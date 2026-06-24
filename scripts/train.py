#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""YOLO training entrypoint: validate dataset, generate dataset.yaml, run Ultralytics."""

import argparse
import csv
import json
import sys
import types
from datetime import datetime
from pathlib import Path


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


def configure_utf8_stdio() -> None:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")


def disable_ultralytics_tensorboard() -> None:
    """Avoid optional TensorBoard imports crashing on broken TensorFlow installs."""
    for module_name in (
        "ultralytics.utils.callbacks.tensorboard",
        "ultralytics.yolo.utils.callbacks.tensorboard",
    ):
        module = types.ModuleType(module_name)
        module.callbacks = {}
        sys.modules[module_name] = module


def resolve_device(requested_device: str) -> str:
    requested = (requested_device or "auto").strip()
    if requested.lower() not in {"", "auto"}:
        log(f"DEVICE_REQUESTED: {requested}")
        return requested

    try:
        import torch
    except ImportError:
        log("WARN: PyTorch is not installed; Ultralytics will choose a device after import.")
        return ""

    log(f"TORCH_VERSION: {torch.__version__}")
    if torch.cuda.is_available():
        device_name = torch.cuda.get_device_name(0)
        cuda_version = torch.version.cuda or "unknown"
        log(f"CUDA_AVAILABLE: True")
        log(f"CUDA_VERSION: {cuda_version}")
        log(f"CUDA_DEVICE_0: {device_name}")
        return "0"

    log("CUDA_AVAILABLE: False")
    log("WARN: CUDA is not available in this Python environment; training will use CPU.")
    log("WARN: Install CUDA-enabled PyTorch if you want to use an NVIDIA GPU.")
    return "cpu"


def log(message: str) -> None:
    print(message, flush=True)


def fail(message: str, code: int = 1) -> None:
    log("ERROR: " + message)
    sys.exit(code)


def load_names_from_yaml(yaml_path: Path):
    if not yaml_path.exists():
        return []
    try:
        import yaml
    except ImportError:
        log("WARN: PyYAML is not installed; class names cannot be parsed. Run: pip install PyYAML")
        return []
    with yaml_path.open("r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}
    names = data.get("names", [])
    if isinstance(names, dict):
        return [str(names[k]) for k in sorted(names)]
    if isinstance(names, list):
        return [str(x) for x in names]
    return []


def count_files(root: Path, suffixes):
    return sum(1 for p in root.rglob("*") if p.is_file() and p.suffix.lower() in suffixes)


def check_yolo_dataset(dataset_root: Path):
    train_images = dataset_root / "train" / "images"
    train_labels = dataset_root / "train" / "labels"
    valid_name = "valid" if (dataset_root / "valid" / "images").exists() else "val"
    valid_images = dataset_root / valid_name / "images"
    valid_labels = dataset_root / valid_name / "labels"
    test_images = dataset_root / "test" / "images"

    image_count = count_files(dataset_root, IMAGE_SUFFIXES)
    label_count = count_files(dataset_root, {".txt"})
    log(f"DATASET_IMAGES: {image_count}")
    log(f"DATASET_LABELS: {label_count}")

    if not train_images.exists() or not train_labels.exists():
        fail("Dataset is missing train/images or train/labels. Please annotate or convert to YOLO format first.")
    if not valid_images.exists() or not valid_labels.exists():
        fail("Dataset is missing valid/images (or val/images) or validation labels. Please check the split.")
    if label_count == 0:
        fail("No YOLO txt labels were found. Please annotate the dataset before training.")

    yaml_path = dataset_root / "data.yaml"
    if not yaml_path.exists():
        yaml_path = dataset_root / "dataset.yaml"
    names = load_names_from_yaml(yaml_path)
    if not names:
        log("WARN: No names field was found in data.yaml; using class_0 by default.")
        names = ["class_0"]

    generated = dataset_root / "dataset.generated.yaml"
    try:
        import yaml
    except ImportError:
        fail("PyYAML is required to generate dataset.generated.yaml. Install it with: pip install PyYAML", 2)

    yaml_data = {
        "path": str(dataset_root.resolve()),
        "train": "train/images",
        "val": f"{valid_name}/images",
        "test": "test/images" if test_images.exists() else "",
        "nc": len(names),
        "names": names,
    }
    with generated.open("w", encoding="utf-8") as f:
        yaml.safe_dump(yaml_data, f, allow_unicode=True, sort_keys=False)
    log(f"DATASET_YAML: {generated}")
    log("CLASSES: " + ", ".join(names))
    return generated


def labels_dir_for_images(images_dir: Path) -> Path:
    if images_dir.name.lower() == "images":
        return images_dir.parent / "labels"
    return images_dir.parent / "labels"


def build_yaml_from_splits(dataset_root: Path, train_images: Path, val_images: Path) -> Path:
    train_images = train_images.resolve()
    val_images = val_images.resolve()
    train_labels = labels_dir_for_images(train_images)
    val_labels = labels_dir_for_images(val_images)

    if not train_images.exists():
        fail(f"Training images directory does not exist: {train_images}")
    if not val_images.exists():
        fail(f"Validation images directory does not exist: {val_images}")
    if not train_labels.exists():
        fail(f"Training labels directory does not exist: {train_labels}")
    if not val_labels.exists():
        fail(f"Validation labels directory does not exist: {val_labels}")

    yaml_path = dataset_root / "data.yaml"
    if not yaml_path.exists():
        yaml_path = dataset_root / "dataset.yaml"
    names = load_names_from_yaml(yaml_path)
    if not names:
        log("WARN: No names field was found in data.yaml; using class_0 by default.")
        names = ["class_0"]

    try:
        import yaml
    except ImportError:
        fail("PyYAML is required to generate dataset.generated.yaml. Install it with: pip install PyYAML", 2)

    generated = dataset_root / "dataset.generated.yaml"
    yaml_data = {
        "path": str(dataset_root.resolve()),
        "train": str(train_images),
        "val": str(val_images),
        "nc": len(names),
        "names": names,
    }
    with generated.open("w", encoding="utf-8") as f:
        yaml.safe_dump(yaml_data, f, allow_unicode=True, sort_keys=False)

    log(f"TRAIN_IMAGES: {train_images}")
    log(f"VAL_IMAGES: {val_images}")
    log(f"DATASET_YAML: {generated}")
    log("CLASSES: " + ", ".join(names))
    return generated


def read_final_metrics(save_dir: Path):
    results_csv = save_dir / "results.csv"
    metrics = {}
    if not results_csv.exists():
        return metrics

    try:
        with results_csv.open("r", encoding="utf-8-sig", newline="") as f:
            rows = list(csv.DictReader(f, skipinitialspace=True))
    except OSError as exc:
        log(f"WARN: Failed to read metrics csv: {exc}")
        return metrics

    if not rows:
        return metrics

    last = rows[-1]
    for key, value in last.items():
        if key is None:
            continue
        clean_key = key.strip()
        clean_value = str(value).strip()
        if not clean_key:
            continue
        try:
            metrics[clean_key] = float(clean_value)
        except ValueError:
            metrics[clean_key] = clean_value
    metrics["results_csv"] = str(results_csv)
    results_png = save_dir / "results.png"
    if results_png.exists():
        metrics["results_png"] = str(results_png)
    return metrics


def main():
    configure_utf8_stdio()
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", required=True, help="Dataset root directory or data.yaml")
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--batch", type=int, default=4)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--model", default="yolov8n.pt")
    parser.add_argument("--lr", type=float, default=0.001)
    parser.add_argument("--device", default="auto", help="Training device: auto, cpu, 0, 0,1, etc.")
    parser.add_argument("--project", default="runs/train")
    parser.add_argument("--train", default="", help="Optional train/images directory selected in GUI")
    parser.add_argument("--val", default="", help="Optional valid/images or val/images directory selected in GUI")
    args = parser.parse_args()

    data_path = Path(args.data)
    dataset_root = data_path.parent if data_path.suffix.lower() in {".yaml", ".yml"} else data_path
    dataset_root = dataset_root.resolve()
    if not dataset_root.exists():
        fail(f"Dataset directory does not exist: {dataset_root}")

    log("=== Vehicle detection training started ===")
    log(f"DATASET_ROOT: {dataset_root}")
    if args.train and args.val:
        yaml_path = build_yaml_from_splits(dataset_root, Path(args.train), Path(args.val))
    else:
        yaml_path = check_yolo_dataset(dataset_root)
    device = resolve_device(args.device)

    disable_ultralytics_tensorboard()
    log("INFO: TensorBoard callback disabled; YOLO training does not require TensorFlow.")

    try:
        from ultralytics import YOLO
    except ImportError:
        fail("Ultralytics is not installed. Run: pip install ultralytics", 2)

    project_dir = Path(args.project).resolve()
    project_dir.mkdir(parents=True, exist_ok=True)
    run_name = datetime.now().strftime("cardet_%Y%m%d_%H%M%S")

    log("Calling ultralytics YOLO.train ...")
    try:
        model = YOLO(args.model)
        results = model.train(
            data=str(yaml_path),
            epochs=args.epochs,
            batch=args.batch,
            imgsz=args.imgsz,
            lr0=args.lr,
            device=device,
            project=str(project_dir),
            name=run_name,
            exist_ok=True,
        )
    except Exception as exc:
        fail(f"Training failed: {exc}")

    save_dir = Path(getattr(results, "save_dir", project_dir / run_name))
    best_model = save_dir / "weights" / "best.pt"
    if not best_model.exists():
        candidates = list(project_dir.rglob("best.pt"))
        best_model = candidates[-1] if candidates else best_model

    summary = {
        "status": "ok",
        "save_dir": str(save_dir),
        "best_model": str(best_model),
        "epochs": args.epochs,
        "batch": args.batch,
        "imgsz": args.imgsz,
        "model": args.model,
        "train_path": args.train,
        "val_path": args.val,
        "metrics": read_final_metrics(save_dir),
    }
    log("BEST_MODEL: " + str(best_model))
    log("FINAL_RESULT: " + json.dumps(summary, ensure_ascii=False))


if __name__ == "__main__":
    main()
