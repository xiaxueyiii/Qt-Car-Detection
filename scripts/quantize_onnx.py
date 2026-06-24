#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Apply ONNX Runtime dynamic INT8 quantization."""

import argparse
import logging
import sys
from pathlib import Path


def log(message: str) -> None:
    print(message, flush=True)


def fail(message: str, code: int = 1) -> None:
    log("ERROR: " + message)
    sys.exit(code)


def install_onnx_reference_guard() -> None:
    """Avoid a native ONNX reference import crash on some Windows setups.

    onnxruntime.quantization imports onnx.reference while loading. Certain
    ONNX wheel / Windows combinations can crash in the ONNX C++ schema
    initializer before Python can raise an exception. Dynamic INT8
    quantization does not need ReferenceEvaluator, so this guard provides the
    small imported symbols and fails clearly only if a reference-evaluator code
    path is actually used.
    """
    import os
    import types

    if os.environ.get("CARDET_DISABLE_ONNX_REFERENCE_GUARD") == "1":
        return

    if "onnx.reference" not in sys.modules:
        reference_module = types.ModuleType("onnx.reference")

        class ReferenceEvaluator:
            def __init__(self, *args, **kwargs):
                raise RuntimeError(
                    "ONNX ReferenceEvaluator is unavailable in this environment. "
                    "Dynamic INT8 quantization does not require it."
                )

        reference_module.ReferenceEvaluator = ReferenceEvaluator
        sys.modules["onnx.reference"] = reference_module

    if "onnx.reference.custom_element_types" not in sys.modules:
        custom_types_module = types.ModuleType("onnx.reference.custom_element_types")
        custom_types_module.float8e4m3fn = None
        custom_types_module.int4 = None
        custom_types_module.uint4 = None
        sys.modules["onnx.reference.custom_element_types"] = custom_types_module

    if "onnx.reference.op_run" not in sys.modules:
        op_run_module = types.ModuleType("onnx.reference.op_run")
        op_run_module.to_array_extended = None
        sys.modules["onnx.reference.op_run"] = op_run_module


def install_shape_inference_guard() -> None:
    """Use plain ONNX loading when native ONNX shape inference is unstable."""
    import importlib
    import tempfile

    import onnx

    quant_utils = importlib.import_module("onnxruntime.quantization.quant_utils")
    quantize_module = importlib.import_module("onnxruntime.quantization.quantize")

    def load_model_without_shape_infer(model_path):
        model = onnx.load(str(model_path))
        try:
            quant_utils.add_infer_metadata(model)
        except Exception:
            pass
        return model

    def save_and_reload_without_shape_infer(model):
        with tempfile.TemporaryDirectory(prefix="cardet.quant.") as quant_tmp_dir:
            model_path = Path(quant_tmp_dir) / "model.onnx"
            onnx.save_model(model, str(model_path), save_as_external_data=True)
            return load_model_without_shape_infer(model_path)

    quant_utils.load_model_with_shape_infer = load_model_without_shape_infer
    quant_utils.save_and_reload_model_with_shape_infer = save_and_reload_without_shape_infer
    quantize_module.load_model_with_shape_infer = load_model_without_shape_infer
    quantize_module.save_and_reload_model_with_shape_infer = save_and_reload_without_shape_infer


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument(
        "--op-types",
        default="Conv,MatMul,Gemm",
        help=(
            "Comma-separated ONNX op types to quantize. Default is Conv,MatMul,Gemm. "
            "The script uses QUInt8 weights because signed QInt8 ConvInteger is not implemented "
            "by common ONNX Runtime CPU builds."
        ),
    )
    parser.add_argument("--verbose", action="store_true", help="Show ONNX Runtime quantization warnings.")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if not input_path.exists():
        fail(f"ONNX file does not exist: {input_path}")

    install_onnx_reference_guard()
    try:
        import onnx
        from onnxruntime.quantization import QuantType, quantize_dynamic
        install_shape_inference_guard()
    except ImportError:
        fail("onnxruntime is not installed. Install it with: pip install onnxruntime", 2)

    log("=== ONNX INT8 dynamic quantization started ===")
    log(f"INPUT: {input_path}")
    log(f"OUTPUT: {output_path}")
    op_types = [item.strip() for item in args.op_types.split(",") if item.strip()]
    log("OP_TYPES: " + (", ".join(op_types) if op_types else "all supported dynamic quantization ops"))
    log("WEIGHT_TYPE: QUInt8")
    if not args.verbose:
        logging.getLogger().setLevel(logging.ERROR)

    try:
        quantize_dynamic(
            str(input_path),
            str(output_path),
            weight_type=QuantType.QUInt8,
            op_types_to_quantize=op_types,
            extra_options={"DefaultTensorType": onnx.TensorProto.FLOAT},
        )
    except Exception as exc:
        fail(f"Quantization failed: {exc}")

    try:
        import onnxruntime as ort

        model = onnx.load(str(output_path))
        quantized_nodes = sum(
            1
            for node in model.graph.node
            if "Quant" in node.op_type or "Integer" in node.op_type or node.op_type.startswith("QLinear")
        )
        ort.InferenceSession(str(output_path), providers=["CPUExecutionProvider"])
    except Exception as exc:
        fail(f"Quantized model validation failed: {exc}")

    log(f"QUANTIZED_NODES: {quantized_nodes}")
    if quantized_nodes == 0:
        log(
            "WARNING: No supported Conv/MatMul/Gemm nodes were quantized. "
            "YOLOv8 is convolution-heavy; static QDQ PTQ with calibration is needed for Conv INT8."
        )
    log("QUANTIZED_MODEL: " + str(output_path.resolve()))


if __name__ == "__main__":
    main()
