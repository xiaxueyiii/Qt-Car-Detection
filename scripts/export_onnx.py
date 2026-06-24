#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Export an Ultralytics .pt model to ONNX."""

import argparse
import shutil
import sys
from pathlib import Path


def log(message: str) -> None:
    print(message, flush=True)


def fail(message: str, code: int = 1) -> None:
    log("ERROR: " + message)
    sys.exit(code)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--weights", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--imgsz", type=int, default=640)
    args = parser.parse_args()

    weights = Path(args.weights)
    output = Path(args.output)

    if weights.suffix.lower() != ".pt":
        fail(
            "ONNX export requires a PyTorch .pt weights file as input. "
            f"Received: {weights}. Use runs/train/.../weights/best.pt, then quantize the exported .onnx file."
        )
    if output.suffix.lower() != ".onnx":
        fail(f"ONNX output path must end with .onnx. Received: {output}")
    if not weights.exists() and (weights.is_absolute() or len(weights.parts) > 1):
        fail(f"Model file does not exist: {weights}")
    output.parent.mkdir(parents=True, exist_ok=True)

    try:
        from ultralytics import YOLO
    except ImportError:
        fail("Ultralytics is not installed, so ONNX export cannot run. Install it with: pip install ultralytics", 2)

    log("=== ONNX export started ===")
    log(f"WEIGHTS: {weights}")
    log(f"OUTPUT: {output}")

    try:
        model = YOLO(str(weights))
        exported_path = Path(model.export(format="onnx", imgsz=args.imgsz, opset=12, simplify=False))
    except Exception as exc:
        fail(f"ONNX export failed: {exc}")

    if exported_path.resolve() != output.resolve():
        if output.exists():
            output.unlink()
        shutil.copy2(exported_path, output)

    log("ONNX_MODEL: " + str(output.resolve()))


if __name__ == "__main__":
    main()
