# CarDet Vehicle Detection Desktop Application

[中文](README.md) | English

CarDet is a Qt Widgets + C++ desktop application for vehicle detection and course project demonstration. It integrates dataset management, image annotation, model training, ONNX export, INT8 dynamic quantization, single-image inference, result visualization, and SQLite-based result storage into one graphical application.

This repository only contains source code and documentation. Local datasets, trained weights, ONNX models, Python virtual environments, build directories, and runtime outputs are not included.

## Features

- Dataset management: scan YOLO-format datasets and count images, labels, and classes.
- Image annotation: create, delete, move, resize, edit, undo, and redo rectangular bounding boxes.
- Category management: add categories, confirm category deletion, and restore deleted categories.
- Model training: start Python YOLO training scripts from Qt/C++ through `QProcess` and display real-time logs in the GUI.
- Model deployment: export `.pt` models to ONNX.
- Model quantization: perform INT8 dynamic quantization with ONNX Runtime.
- Image inference: select an image and model from the GUI and display detection results.
- C++ ONNX Runtime inference: provide a C++ inference path for ONNX and INT8 ONNX models.
- Database storage: store datasets, annotations, training records, model files, and inference results in SQLite.
- Result visualization: view training records, model records, and inference results in the application.

## Technology Stack

- Desktop UI: Qt Widgets
- Languages: C++20 and Python
- Build system: qmake
- Database: SQLite / Qt SQL
- Detection framework: Ultralytics YOLO
- Model formats: PyTorch `.pt` and ONNX `.onnx`
- Quantization: ONNX Runtime dynamic quantization
- Inference paths: Python YOLO, Python ONNX Runtime, and C++ ONNX Runtime

## Project Structure

```text
CarDet/
  CarDet.pro                 qmake project file
  README.md                  Chinese documentation
  README_EN.md               English documentation
  requirements.txt           Basic Python dependencies
  requirements-gpu-cu124.txt Example GPU dependencies for CUDA 12.4
  src/                       Qt/C++ application source code
    model/                   Database, data types, and C++ ONNX Runtime inference
    view/                    GUI pages
    viewmodel/               Business logic and QProcess orchestration
    utils/                   Path and configuration utilities
  scripts/                   Python training, inference, export, and quantization scripts
  docs/                      Project documents and course reports
  tools/                     Auxiliary benchmark tools
```

The following local resources and generated outputs are intentionally excluded from GitHub:

```text
Vehicles-dataset/  Dataset files
models/            Model weights and exported models
runs/              Training, inference, and benchmark outputs
database/*.db      Local SQLite database files
.venv/             Python virtual environment
build*/            Qt build directories
third_party/       Third-party binary libraries
```

## Core Modules

`src/MainWindow.cpp` creates the main window, menu bar, status bar, and functional pages. The GUI is organized with tabs for dataset management, annotation, training, model deployment, image inference, and result visualization.

`src/view/DatasetPage.cpp` and `src/viewmodel/DatasetViewModel.cpp` implement dataset scanning and statistics. They read YOLO-format dataset directories and collect image, label, and class information.

`src/view/AnnotationPage.cpp`, `src/view/AnnotationCanvas.cpp`, and `src/viewmodel/AnnotationViewModel.cpp` implement image annotation. Users can draw bounding boxes, edit categories, save YOLO label files, and write annotation records to SQLite.

`src/view/TrainingPage.cpp` and `src/viewmodel/TrainingViewModel.cpp` implement the training page and training workflow. The C++ application launches `scripts/train.py` through `QProcess`, reads Python output in real time, and displays logs in the GUI.

`src/view/DeployPage.cpp` and `src/viewmodel/DeployViewModel.cpp` implement model deployment and quantization. `scripts/export_onnx.py` exports ONNX models, while `scripts/quantize.py` and `scripts/quantize_onnx.py` perform INT8 dynamic quantization.

`src/view/InferencePage.cpp`, `src/viewmodel/InferenceViewModel.cpp`, and `scripts/infer_image.py` implement the single-image inference workflow. Users can select an image, model, and confidence threshold. The result image and detection table are displayed in the GUI.

`src/model/DatabaseManager.cpp` manages SQLite initialization and data access. It stores datasets, images, categories, annotations, training records, model files, and inference results.

`src/model/QuantizedOnnxRunner.cpp` wraps C++ ONNX Runtime inference and demonstrates how the C++ side can load ONNX or INT8 ONNX models for deployment.

## Environment Setup

Qt/C++ environment:

- Qt 6
- Qt Widgets
- Qt SQL
- MinGW 64-bit or another Qt-compatible C++ compiler
- C++20 support

Python environment:

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python -m venv .venv
.\.venv\Scripts\activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

For NVIDIA GPU training, install a PyTorch build that matches your CUDA version. `requirements-gpu-cu124.txt` is provided as an example for CUDA 12.4 environments.

## Build and Run

With Qt Creator:

1. Open Qt Creator.
2. Open `CarDet.pro`.
3. Select a Qt 6 MinGW 64-bit kit.
4. Click Configure Project.
5. Click Build.
6. Click Run.

If the project is moved to another directory, set the project root environment variable:

```powershell
$env:CARDET_ROOT="E:\Desktop\FriD\suanfa\CarDet"
```

To specify a Python interpreter:

```powershell
$env:CARDET_PYTHON="E:\Desktop\FriD\suanfa\CarDet\.venv\Scripts\python.exe"
```

## Dataset Format

The recommended dataset format is YOLO detection format:

```text
Vehicles-dataset/
  data.yaml
  train/
    images/
    labels/
  valid/
    images/
    labels/
  test/
    images/
    labels/
```

Each YOLO label file has the same base name as its image and uses the `.txt` extension. Each line represents one bounding box:

```text
class_id center_x center_y width height
```

`center_x`, `center_y`, `width`, and `height` are normalized values, usually between 0 and 1.

## Python Script Examples

Train a model:

```powershell
python scripts/train.py --data Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt
```

Export ONNX:

```powershell
python scripts/export_onnx.py --weights runs\train\your_run\weights\best.pt --output models\best.onnx
```

Run INT8 dynamic quantization:

```powershell
python scripts/quantize_onnx.py --input models\best.onnx --output models\best_int8.onnx
```

Run single-image inference:

```powershell
python scripts/infer_image.py --weights models\best.onnx --image Vehicles-dataset\test\images\demo.jpg --output runs\infer
```

## GitHub Submission Notes

This repository is intended to submit the project source code and documentation for course review. Datasets, trained weights, ONNX models, quantized models, and runtime outputs are not committed.

To reproduce the complete workflow, prepare the following local resources:

- A YOLO-format vehicle dataset
- A trained `.pt` weight file
- An exported `.onnx` model file
- Optional ONNX Runtime C++ development package

## Course Demonstration Flow

Recommended presentation order:

1. Open the Qt main window and explain that it is a Qt/C++ desktop application.
2. Show the dataset management page and explain the YOLO dataset structure.
3. Show the annotation page and demonstrate bounding-box creation, category editing, saving, and undo.
4. Show the training page and explain training parameters and real-time logs.
5. Show the deployment page and demonstrate ONNX export and INT8 dynamic quantization.
6. Show the inference page, select an image and model, and display detection results.
7. Show the visualization page or database records to prove that training and inference results are stored.
