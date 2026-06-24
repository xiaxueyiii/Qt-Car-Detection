#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Performance benchmark helper for the CarDet Qt/C++ course project.

The script deliberately reports only measured values. It can be used for:

- dataset scan / image-list loading timing
- annotation operation simulation timing
- Python process startup and stdout latency timing
- Python Ultralytics inference end-to-end timing
- Python ONNX Runtime direct session timing
- SQLite insert/select benchmark
- optional C++ ONNX Runtime benchmark executable integration

Example:
    .venv\\Scripts\\python.exe scripts\\performance_benchmark.py --runs 10
"""

from __future__ import annotations

import argparse
import csv
import json
import sqlite3
import statistics
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


IMAGE_SUFFIXES = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}


@dataclass
class Metric:
    name: str
    unit: str
    values: list[float] = field(default_factory=list)
    note: str = ""

    def summary(self) -> dict[str, Any]:
        if not self.values:
            return {
                "name": self.name,
                "unit": self.unit,
                "runs": 0,
                "avg": None,
                "min": None,
                "max": None,
                "median": None,
                "note": self.note,
            }
        return {
            "name": self.name,
            "unit": self.unit,
            "runs": len(self.values),
            "avg": statistics.fmean(self.values),
            "min": min(self.values),
            "max": max(self.values),
            "median": statistics.median(self.values),
            "note": self.note,
        }


def now_ms() -> float:
    return time.perf_counter() * 1000.0


def time_call(fn):
    start = now_ms()
    result = fn()
    return now_ms() - start, result


def project_root_from_script() -> Path:
    return Path(__file__).resolve().parents[1]


def first_file(root: Path, suffixes: set[str]) -> Path | None:
    if not root.exists():
        return None
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix.lower() in suffixes:
            return path
    return None


def latest_best_pt(project_root: Path) -> Path | None:
    train_root = project_root / "runs" / "train"
    if not train_root.exists():
        return None
    candidates = sorted(
        train_root.rglob("weights/best.pt"),
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )
    return candidates[0] if candidates else None


def count_files(root: Path, suffixes: set[str]) -> int:
    if not root.exists():
        return 0
    return sum(1 for p in root.rglob("*") if p.is_file() and p.suffix.lower() in suffixes)


def label_path_for_image(image_path: Path) -> Path:
    if image_path.parent.name.lower() == "images":
        return image_path.parent.parent / "labels" / f"{image_path.stem}.txt"
    return image_path.with_suffix(".txt")


def label_summary(label_path: Path) -> tuple[int, str]:
    if not label_path.exists():
        return 0, ""
    count = 0
    class_ids: list[str] = []
    try:
        for line in label_path.read_text(encoding="utf-8", errors="replace").splitlines():
            line = line.strip()
            if not line:
                continue
            count += 1
            class_id = line.split()[0]
            if class_id not in class_ids:
                class_ids.append(class_id)
    except OSError:
        return 0, ""
    return count, ",".join(class_ids)


def benchmark_dataset_scan(project_root: Path, runs: int) -> tuple[Metric, dict[str, Any]]:
    dataset_root = project_root / "Vehicles-dataset"
    metric = Metric(
        "dataset_scan_and_image_table",
        "ms",
        note="Simulates DatasetPage scan plus image/label summary table population without rendering Qt widgets.",
    )
    last_detail: dict[str, Any] = {}

    def scan_once() -> dict[str, Any]:
        image_count = count_files(dataset_root, IMAGE_SUFFIXES)
        label_count = count_files(dataset_root, {".txt"})
        image_rows = []
        for split in ("train", "valid", "val", "test"):
            image_dir = dataset_root / split / "images"
            if not image_dir.exists():
                continue
            for image in sorted(image_dir.iterdir()):
                if not image.is_file() or image.suffix.lower() not in IMAGE_SUFFIXES:
                    continue
                labels, classes = label_summary(label_path_for_image(image))
                image_rows.append((image.name, split, labels, classes))
        return {
            "dataset_root": str(dataset_root),
            "image_count": image_count,
            "label_count": label_count,
            "image_table_rows": len(image_rows),
        }

    for _ in range(runs):
        elapsed, last_detail = time_call(scan_once)
        metric.values.append(elapsed)
    return metric, last_detail


def benchmark_annotation_simulation(runs: int, operations: int) -> Metric:
    metric = Metric(
        "annotation_box_operation_simulation",
        "ms",
        note=(
            "In-memory simulation for create/move/resize/delete/undo-like list operations. "
            "It does not measure real mouse event latency inside the Qt GUI."
        ),
    )

    def simulate() -> int:
        boxes: list[tuple[int, float, float, float, float]] = []
        undo_stack: list[list[tuple[int, float, float, float, float]]] = []
        for i in range(operations):
            undo_stack.append(list(boxes))
            class_id = i % 5
            cx = 0.1 + (i % 50) * 0.01
            cy = 0.1 + (i % 40) * 0.01
            w = 0.08
            h = 0.06
            boxes.append((class_id, cx, cy, w, h))
            if len(boxes) > 4:
                old = boxes[-3]
                boxes[-3] = (old[0], min(old[1] + 0.005, 0.95), old[2], old[3], old[4])
            if len(boxes) > 8:
                old = boxes[-7]
                boxes[-7] = (old[0], old[1], old[2], min(old[3] * 1.02, 0.5), old[4])
            if len(boxes) > 40 and i % 11 == 0:
                boxes.pop(0)
            if len(undo_stack) > 80:
                undo_stack.pop(0)
        return len(boxes)

    for _ in range(runs):
        elapsed, _ = time_call(simulate)
        metric.values.append(elapsed)
    return metric


def benchmark_python_process_start(python: Path, runs: int) -> Metric:
    metric = Metric(
        "python_process_start_to_first_stdout",
        "ms",
        note="Measures QProcess-like subprocess startup until the first stdout line is readable.",
    )
    code = "print('READY', flush=True)"
    for _ in range(runs):
        start = now_ms()
        proc = subprocess.Popen(
            [str(python), "-u", "-c", code],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        assert proc.stdout is not None
        _ = proc.stdout.readline()
        metric.values.append(now_ms() - start)
        proc.wait(timeout=10)
    return metric


def benchmark_stdout_latency(python: Path, runs: int, lines: int) -> Metric:
    metric = Metric(
        "stdout_realtime_latency",
        "ms",
        note="Measures parent-side read latency for unbuffered child stdout lines; approximates training log responsiveness.",
    )
    code = (
        "import time\n"
        f"for i in range({lines}):\n"
        "    print(f'TICK {i} {time.perf_counter_ns()}', flush=True)\n"
        "    time.sleep(0.02)\n"
    )
    for _ in range(runs):
        proc = subprocess.Popen(
            [str(python), "-u", "-c", code],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            parts = line.strip().split()
            if len(parts) == 3 and parts[0] == "TICK":
                child_ms = int(parts[2]) / 1_000_000.0
                metric.values.append(now_ms() - child_ms)
        proc.wait(timeout=10)
    return metric


def run_infer_script(
    python: Path,
    script: Path,
    weights: Path,
    image: Path,
    output_root: Path,
    device: str,
    conf: float,
    index: int,
) -> tuple[float, dict[str, Any]]:
    output_dir = output_root / f"run_{index:03d}"
    output_dir.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(python),
        "-u",
        str(script),
        "--weights",
        str(weights),
        "--image",
        str(image),
        "--output",
        str(output_dir),
        "--device",
        device,
        "--conf",
        str(conf),
    ]
    start = now_ms()
    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    elapsed = now_ms() - start
    result: dict[str, Any] = {
        "returncode": proc.returncode,
        "stdout_tail": "\n".join(proc.stdout.splitlines()[-20:]),
    }
    for line in proc.stdout.splitlines():
        if line.startswith("RESULT_IMAGE:"):
            result["result_image"] = line.split(":", 1)[1].strip()
        elif line.startswith("RESULT_JSON:"):
            result["result_json"] = line.split(":", 1)[1].strip()
        elif line.startswith("DETECTED_COUNT:"):
            result["detected_count"] = line.split(":", 1)[1].strip()
    return elapsed, result


def benchmark_python_inference(
    python: Path,
    project_root: Path,
    weights: Path | None,
    image: Path,
    runs: int,
    device: str,
    conf: float,
    name: str,
) -> tuple[Metric, list[dict[str, Any]]]:
    metric = Metric(
        name,
        "ms",
        note=(
            "End-to-end subprocess benchmark for scripts/infer_image.py. "
            "Includes Python startup, model loading, preprocessing, inference, drawing, json/image writing."
        ),
    )
    details: list[dict[str, Any]] = []
    if weights is None or not weights.exists():
        metric.note += " Skipped: model file is missing."
        return metric, details

    script = project_root / "scripts" / "infer_image.py"
    output_root = project_root / "runs" / "perf_benchmark" / name
    for i in range(runs):
        elapsed, detail = run_infer_script(python, script, weights, image, output_root, device, conf, i)
        details.append(detail)
        if detail["returncode"] == 0:
            metric.values.append(elapsed)
        else:
            metric.note += f" Run {i} failed; see details."
            break
    return metric, details


def preprocess_image_for_onnx(image: Path, width: int, height: int):
    import cv2
    import numpy as np

    data = np.fromfile(str(image), dtype=np.uint8)
    bgr = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if bgr is None:
        raise RuntimeError(f"Failed to read image: {image}")
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    resized = cv2.resize(rgb, (width, height), interpolation=cv2.INTER_LINEAR)
    tensor = resized.astype("float32") / 255.0
    tensor = tensor.transpose(2, 0, 1)[None, :, :, :]
    return tensor


def benchmark_python_onnxruntime_direct(model: Path | None, image: Path, runs: int, name: str) -> Metric:
    metric = Metric(
        name,
        "ms",
        note=(
            "Direct Python ONNX Runtime Session.run benchmark. "
            "It measures model load once plus repeated tensor inference, not Ultralytics postprocess."
        ),
    )
    if model is None or not model.exists():
        metric.note += " Skipped: model file is missing."
        return metric

    try:
        import onnxruntime as ort
    except Exception as exc:
        metric.note += f" Skipped: onnxruntime import failed: {exc}"
        return metric

    session = ort.InferenceSession(str(model), providers=["CPUExecutionProvider"])
    input_meta = session.get_inputs()[0]
    output_names = [out.name for out in session.get_outputs()]
    shape = list(input_meta.shape)
    height = int(shape[2]) if len(shape) == 4 and isinstance(shape[2], int) else 640
    width = int(shape[3]) if len(shape) == 4 and isinstance(shape[3], int) else 640
    tensor = preprocess_image_for_onnx(image, width, height)

    session.run(output_names, {input_meta.name: tensor})
    for _ in range(runs):
        elapsed, _ = time_call(lambda: session.run(output_names, {input_meta.name: tensor}))
        metric.values.append(elapsed)
    metric.note += f" Providers: {','.join(session.get_providers())}; input={height}x{width}."
    return metric


def benchmark_database(db_path: Path, runs: int, rows: int) -> tuple[Metric, Metric, Metric]:
    insert_metric = Metric(
        "sqlite_insert_transaction",
        "ms",
        note=f"Inserts {rows} rows per run into a temporary benchmark table in one transaction.",
    )
    select_metric = Metric(
        "sqlite_select_count",
        "ms",
        note="Runs SELECT COUNT(*) on the benchmark table.",
    )
    recent_query_metric = Metric(
        "sqlite_recent_inference_query",
        "ms",
        note="Queries recent inference rows from the real inference_records table.",
    )

    con = sqlite3.connect(db_path)
    cur = con.cursor()
    cur.execute(
        "CREATE TABLE IF NOT EXISTS perf_benchmark_tmp ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "created_at TEXT, "
        "payload TEXT)"
    )
    con.commit()

    for run in range(runs):
        payload = json.dumps({"run": run, "source": "performance_benchmark"}, ensure_ascii=False)

        def insert_rows():
            cur.execute("BEGIN")
            for _ in range(rows):
                cur.execute(
                    "INSERT INTO perf_benchmark_tmp (created_at, payload) VALUES (?, ?)",
                    (time.strftime("%Y-%m-%d %H:%M:%S"), payload),
                )
            con.commit()

        elapsed, _ = time_call(insert_rows)
        insert_metric.values.append(elapsed)

        elapsed, _ = time_call(lambda: cur.execute("SELECT COUNT(*) FROM perf_benchmark_tmp").fetchone())
        select_metric.values.append(elapsed)

        elapsed, _ = time_call(
            lambda: cur.execute(
                "SELECT id, image_path, model_path, detected_count, created_at "
                "FROM inference_records ORDER BY id DESC LIMIT 100"
            ).fetchall()
        )
        recent_query_metric.values.append(elapsed)

    cur.execute("DELETE FROM perf_benchmark_tmp")
    con.commit()
    con.close()
    return insert_metric, select_metric, recent_query_metric


def run_cpp_benchmark(exe: Path | None, model: Path, image: Path, runs: int, conf: float) -> tuple[Metric, dict[str, Any] | None]:
    metric = Metric(
        "cpp_onnxruntime_cli",
        "ms",
        note="Optional external C++ chrono benchmark. Build tools/cpp_onnx_benchmark first.",
    )
    if exe is None or not exe.exists():
        metric.note += " Skipped: C++ benchmark executable not provided or not found."
        return metric, None

    output_dir = model.parent.parent / "runs" / "perf_benchmark" / "cpp_onnxruntime"
    cmd = [
        str(exe),
        "--model",
        str(model),
        "--image",
        str(image),
        "--output",
        str(output_dir),
        "--runs",
        str(runs),
        "--conf",
        str(conf),
    ]
    proc = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    detail = {"returncode": proc.returncode, "stdout": proc.stdout}
    try:
        parsed = json.loads(proc.stdout)
        for value in parsed.get("values_ms", []):
            metric.values.append(float(value))
        metric.note += " Parsed JSON from C++ benchmark."
    except Exception:
        metric.note += " Failed to parse C++ benchmark output; see details."
    return metric, detail


def write_reports(output_dir: Path, metrics: list[Metric], details: dict[str, Any]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    data = {
        "generated_at": time.strftime("%Y-%m-%d %H:%M:%S"),
        "metrics": [m.summary() for m in metrics],
        "details": details,
    }
    (output_dir / "performance_results.json").write_text(
        json.dumps(data, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    with (output_dir / "performance_summary.csv").open("w", encoding="utf-8-sig", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["name", "unit", "runs", "avg", "min", "max", "median", "note"])
        writer.writeheader()
        for metric in metrics:
            writer.writerow(metric.summary())


def main() -> int:
    parser = argparse.ArgumentParser(description="CarDet performance benchmark")
    parser.add_argument("--project-root", default=str(project_root_from_script()))
    parser.add_argument("--python", default=sys.executable)
    parser.add_argument("--runs", type=int, default=10, help="Repeated runs for averaged metrics.")
    parser.add_argument("--db-rows", type=int, default=1000, help="Rows inserted per SQLite benchmark run.")
    parser.add_argument("--annotation-ops", type=int, default=5000)
    parser.add_argument("--image", default="")
    parser.add_argument("--pt-model", default="")
    parser.add_argument("--onnx-model", default="")
    parser.add_argument("--int8-model", default="")
    parser.add_argument("--device", default="0", help="Device for scripts/infer_image.py when model is .pt.")
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--skip-python-infer", action="store_true")
    parser.add_argument("--skip-direct-onnx", action="store_true")
    parser.add_argument("--cpp-benchmark-exe", default="")
    args = parser.parse_args()

    project_root = Path(args.project_root).resolve()
    python = Path(args.python).resolve()
    image = Path(args.image).resolve() if args.image else first_file(project_root / "Vehicles-dataset" / "test" / "images", IMAGE_SUFFIXES)
    if image is None:
        raise SystemExit("No test image found. Pass --image explicitly.")

    pt_model = Path(args.pt_model).resolve() if args.pt_model else latest_best_pt(project_root)
    onnx_model = Path(args.onnx_model).resolve() if args.onnx_model else project_root / "models" / "best.onnx"
    int8_model = Path(args.int8_model).resolve() if args.int8_model else project_root / "models" / "best_int8.onnx"
    cpp_exe = Path(args.cpp_benchmark_exe).resolve() if args.cpp_benchmark_exe else None

    output_dir = project_root / "runs" / "perf_benchmark"
    details: dict[str, Any] = {
        "project_root": str(project_root),
        "python": str(python),
        "image": str(image),
        "pt_model": str(pt_model) if pt_model else "",
        "onnx_model": str(onnx_model),
        "int8_model": str(int8_model),
        "runs": args.runs,
    }
    metrics: list[Metric] = []

    dataset_metric, dataset_detail = benchmark_dataset_scan(project_root, args.runs)
    metrics.append(dataset_metric)
    details["dataset"] = dataset_detail

    metrics.append(benchmark_annotation_simulation(args.runs, args.annotation_ops))
    metrics.append(benchmark_python_process_start(python, args.runs))
    metrics.append(benchmark_stdout_latency(python, max(1, min(args.runs, 3)), lines=20))

    db_path = project_root / "database" / "cardet.db"
    if db_path.exists():
        metrics.extend(benchmark_database(db_path, args.runs, args.db_rows))
    else:
        metrics.append(Metric("sqlite_benchmark", "ms", note="Skipped: database/cardet.db is missing."))

    if not args.skip_python_infer:
        py_pt, py_pt_details = benchmark_python_inference(
            python,
            project_root,
            pt_model,
            image,
            args.runs,
            args.device,
            args.conf,
            "python_ultralytics_pt_end_to_end",
        )
        metrics.append(py_pt)
        details["python_ultralytics_pt"] = py_pt_details

        py_onnx, py_onnx_details = benchmark_python_inference(
            python,
            project_root,
            onnx_model,
            image,
            args.runs,
            "cpu",
            args.conf,
            "python_ultralytics_onnx_end_to_end",
        )
        metrics.append(py_onnx)
        details["python_ultralytics_onnx"] = py_onnx_details

        py_int8, py_int8_details = benchmark_python_inference(
            python,
            project_root,
            int8_model,
            image,
            args.runs,
            "cpu",
            args.conf,
            "python_ultralytics_int8_onnx_end_to_end",
        )
        metrics.append(py_int8)
        details["python_ultralytics_int8"] = py_int8_details

    if not args.skip_direct_onnx:
        metrics.append(benchmark_python_onnxruntime_direct(onnx_model, image, args.runs, "python_onnxruntime_direct_fp32_session_run"))
        metrics.append(benchmark_python_onnxruntime_direct(int8_model, image, args.runs, "python_onnxruntime_direct_int8_session_run"))

    cpp_metric, cpp_detail = run_cpp_benchmark(cpp_exe, int8_model, image, args.runs, args.conf)
    metrics.append(cpp_metric)
    if cpp_detail is not None:
        details["cpp_benchmark"] = cpp_detail

    write_reports(output_dir, metrics, details)
    print(json.dumps({"output_dir": str(output_dir), "metrics": [m.summary() for m in metrics]}, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
