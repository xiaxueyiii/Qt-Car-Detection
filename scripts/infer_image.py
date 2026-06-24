#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Run inference on one image and write result.jpg plus result.json."""

import argparse
import json
import sys
from pathlib import Path


def log(message: str) -> None:
    print(message, flush=True)


def fail(message: str, code: int = 1) -> None:
    log("ERROR: " + message)
    sys.exit(code)


def imread_unicode(path: Path):
    import cv2
    import numpy as np

    data = np.fromfile(str(path), dtype=np.uint8)
    if data.size == 0:
        return None
    return cv2.imdecode(data, cv2.IMREAD_COLOR)


def imwrite_unicode(path: Path, image):
    import cv2

    ok, buffer = cv2.imencode(".jpg", image)
    if not ok:
        return False
    buffer.tofile(str(path))
    return True


def resolve_device(weights: Path, requested: str) -> str:
    requested = (requested or "auto").strip()
    if requested.lower() not in {"auto", ""}:
        return requested

    if weights.suffix.lower() == ".onnx":
        return "cpu"

    try:
        import torch

        if torch.cuda.is_available():
            return "0"
    except Exception:
        pass
    return "cpu"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--weights", required=True)
    parser.add_argument("--image", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--device", default="auto", help="Ultralytics device, for example auto, cpu, 0, cuda:0")
    args = parser.parse_args()

    image_path = Path(args.image)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    result_image = output_dir / "result.jpg"
    result_json = output_dir / "result.json"

    if not image_path.exists():
        fail(f"Image file does not exist: {image_path}")

    try:
        import cv2
    except ImportError:
        fail("OpenCV is not installed. Install it with: pip install opencv-python", 2)

    try:
        from ultralytics import YOLO
    except ImportError:
        fail("Ultralytics is not installed. Install it with: pip install ultralytics", 2)

    weights = Path(args.weights)
    if not weights.exists() and (weights.is_absolute() or len(weights.parts) > 1):
        fail(f"Model file does not exist: {weights}")
    device = resolve_device(weights, args.device)
    if weights.suffix.lower() == ".onnx":
        try:
            import onnxruntime as ort

            log("ONNXRUNTIME_VERSION: " + getattr(ort, "__version__", "unknown"))
            log("ONNXRUNTIME_PROVIDERS: " + ", ".join(ort.get_available_providers()))
            if device != "cpu" and "CUDAExecutionProvider" not in ort.get_available_providers():
                log("WARN: ONNX Runtime CUDAExecutionProvider is not available; ONNX inference will use CPU.")
                device = "cpu"
        except Exception as exc:
            fail(
                "ONNX Runtime cannot be loaded for ONNX inference. "
                "Use the latest runs/train/.../weights/best.pt for stable demo inference, "
                "or repair the environment with: pip uninstall -y onnxruntime-gpu && "
                "pip install --force-reinstall onnxruntime==1.20.1. "
                f"Original error: {exc}"
            )

    log("=== Single-image vehicle detection started ===")
    log(f"IMAGE: {image_path}")
    log(f"WEIGHTS: {args.weights}")
    log(f"DEVICE: {device}")

    image = imread_unicode(image_path)
    if image is None:
        fail(f"Failed to read image: {image_path}")

    try:
        model = YOLO(str(args.weights))
        predictions = model(str(image_path), conf=args.conf, verbose=True, device=device)
    except Exception as exc:
        fail(f"Inference failed: {exc}")

    names = getattr(model, "names", {})
    annotated = image.copy()
    results = []

    if predictions:
        pred = predictions[0]
        if getattr(pred, "boxes", None) is not None:
            for box in pred.boxes:
                xyxy = box.xyxy[0].detach().cpu().tolist()
                cls_id = int(box.cls[0].detach().cpu().item())
                conf = float(box.conf[0].detach().cpu().item())
                class_name = str(names.get(cls_id, f"class_{cls_id}") if isinstance(names, dict) else names[cls_id])
                x1, y1, x2, y2 = [float(v) for v in xyxy]
                results.append(
                    {
                        "class_id": cls_id,
                        "class_name": class_name,
                        "confidence": conf,
                        "bbox": [x1, y1, x2, y2],
                    }
                )

                p1 = (int(round(x1)), int(round(y1)))
                p2 = (int(round(x2)), int(round(y2)))
                cv2.rectangle(annotated, p1, p2, (0, 210, 255), 2)
                label = f"{class_name} {conf:.2f}"
                cv2.putText(
                    annotated,
                    label,
                    (p1[0], max(18, p1[1] - 6)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.6,
                    (0, 255, 170),
                    2,
                    cv2.LINE_AA,
                )

    if not imwrite_unicode(result_image, annotated):
        fail(f"Failed to save result image: {result_image}")

    payload = {"image": str(image_path), "results": results}
    result_json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

    log(f"DETECTED_COUNT: {len(results)}")
    log("RESULT_IMAGE: " + str(result_image.resolve()))
    log("RESULT_JSON: " + str(result_json.resolve()))


if __name__ == "__main__":
    main()
