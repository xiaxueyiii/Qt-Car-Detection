# CarDet 车辆识别检测桌面应用

中文 | [English](README_EN.md)

CarDet 是一个基于 Qt Widgets + C++ 的车辆识别检测桌面应用，用于课程项目展示和车辆目标检测流程演示。项目将数据集管理、图像标注、模型训练、ONNX 导出、INT8 动态量化、单图像推理、结果可视化和 SQLite 数据库存储整合到一个图形化软件中。

本仓库只上传项目代码和说明文档，不包含本地数据集、训练权重、ONNX 模型、Python 虚拟环境、构建目录和训练输出结果。

## 项目功能

- 数据集管理：支持 YOLO 格式数据集扫描、图片数量统计、标签数量统计和类别读取。
- 图像标注：支持矩形框创建、删除、移动、缩放、类别修改、撤销和重做。
- 类别管理：支持类别新增、删除确认和删除后撤回。
- 模型训练：通过 Qt/C++ 使用 `QProcess` 调用 Python YOLO 训练脚本，并在界面实时显示训练日志。
- 模型部署：支持将 `.pt` 模型导出为 ONNX 模型。
- 模型量化：支持基于 ONNX Runtime 的 INT8 动态量化。
- 图像推理：支持选择图片和模型进行单图像车辆检测，并在 GUI 中显示检测结果。
- C++ ONNX Runtime 推理：提供 C++ 端加载 ONNX/INT8 ONNX 模型的推理入口。
- 数据库存储：使用 SQLite 保存数据集、标注、训练、模型和推理结果记录。
- 结果可视化：在软件界面中查看训练记录、模型文件和推理结果。

## 技术栈

- 桌面界面：Qt Widgets
- 开发语言：C++20、Python
- 构建方式：qmake
- 数据库：SQLite / Qt SQL
- 深度学习框架：Ultralytics YOLO
- 模型格式：PyTorch `.pt`、ONNX `.onnx`
- 量化方式：ONNX Runtime 动态量化
- 推理方式：Python YOLO、Python ONNX Runtime、C++ ONNX Runtime

## 目录结构

```text
CarDet/
  CarDet.pro                 qmake 工程文件
  README.md                  中文说明文档
  README_EN.md               英文说明文档
  requirements.txt           Python 基础依赖
  requirements-gpu-cu124.txt GPU 训练依赖示例
  src/                       Qt/C++ 主程序源码
    model/                   数据库、数据结构、C++ ONNX Runtime 推理
    view/                    各个 GUI 页面
    viewmodel/               业务调度和 QProcess 调用逻辑
    utils/                   路径和配置工具
  scripts/                   Python 训练、推理、导出和量化脚本
  docs/                      项目说明、验收报告、性能文档
  tools/                     辅助测试工具
```

以下目录属于本地运行资源或生成结果，默认不上传到 GitHub：

```text
Vehicles-dataset/  数据集
models/            模型权重和导出模型
runs/              训练、推理和性能测试输出
database/*.db      本地 SQLite 数据库
.venv/             Python 虚拟环境
build*/            Qt 构建目录
third_party/       第三方二进制库
```

## 核心模块说明

`src/MainWindow.cpp` 负责创建主窗口、菜单栏、状态栏和功能页面。主界面通过标签页组织数据集管理、标注、训练、模型部署/量化、图像推理和结果可视化等功能。

`src/view/DatasetPage.cpp` 和 `src/viewmodel/DatasetViewModel.cpp` 实现数据集扫描和统计，能够读取 YOLO 数据集目录，统计图片、标签和类别信息。

`src/view/AnnotationPage.cpp`、`src/view/AnnotationCanvas.cpp` 和 `src/viewmodel/AnnotationViewModel.cpp` 实现图像标注功能。用户可以在图片上绘制矩形框，修改类别，保存为 YOLO 标签文件，并将标注信息写入数据库。

`src/view/TrainingPage.cpp` 和 `src/viewmodel/TrainingViewModel.cpp` 实现训练界面和训练流程调度。C++ 端通过 `QProcess` 调用 `scripts/train.py`，实时读取 Python 输出并显示在 GUI 日志窗口中。

`src/view/DeployPage.cpp` 和 `src/viewmodel/DeployViewModel.cpp` 实现模型部署和量化入口。`scripts/export_onnx.py` 用于 ONNX 导出，`scripts/quantize.py` 和 `scripts/quantize_onnx.py` 用于 INT8 动态量化。

`src/view/InferencePage.cpp`、`src/viewmodel/InferenceViewModel.cpp` 和 `scripts/infer_image.py` 实现单图像推理流程。用户可以选择图片、模型和置信度阈值，推理完成后界面显示结果图和检测对象列表。

`src/model/DatabaseManager.cpp` 负责 SQLite 数据库初始化和读写，保存数据集、图片、类别、标注、训练记录、模型文件和推理结果。

`src/model/QuantizedOnnxRunner.cpp` 封装 C++ ONNX Runtime 推理逻辑，用于展示 C++ 端加载 ONNX 或 INT8 ONNX 模型进行推理的能力。

## 环境准备

Qt/C++ 环境：

- Qt 6
- Qt Widgets
- Qt SQL
- MinGW 64-bit 或其他兼容 Qt 的 C++ 编译器
- 支持 C++20 的编译环境

Python 环境：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python -m venv .venv
.\.venv\Scripts\activate
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

如果需要使用 NVIDIA GPU 训练，可根据本机 CUDA 版本安装对应的 PyTorch。项目中提供了 `requirements-gpu-cu124.txt` 作为 CUDA 12.4 环境示例。

## 编译运行

使用 Qt Creator：

1. 打开 Qt Creator。
2. 选择 `CarDet.pro`。
3. 选择 Qt 6 MinGW 64-bit Kit。
4. 点击 Configure Project。
5. 点击 Build。
6. 点击 Run。

如果项目放在其他路径，可以设置项目根目录环境变量：

```powershell
$env:CARDET_ROOT="E:\Desktop\FriD\suanfa\CarDet"
```

如果需要指定 Python 解释器：

```powershell
$env:CARDET_PYTHON="E:\Desktop\FriD\suanfa\CarDet\.venv\Scripts\python.exe"
```

## 数据集格式

项目推荐使用 YOLO 检测数据集格式：

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

YOLO 标签文件与图片同名，扩展名为 `.txt`。每一行表示一个目标框：

```text
class_id center_x center_y width height
```

其中 `center_x`、`center_y`、`width`、`height` 是归一化坐标，取值通常在 0 到 1 之间。

## Python 脚本示例

训练模型：

```powershell
python scripts/train.py --data Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt
```

导出 ONNX：

```powershell
python scripts/export_onnx.py --weights runs\train\your_run\weights\best.pt --output models\best.onnx
```

INT8 动态量化：

```powershell
python scripts/quantize_onnx.py --input models\best.onnx --output models\best_int8.onnx
```

单图像推理：

```powershell
python scripts/infer_image.py --weights models\best.onnx --image Vehicles-dataset\test\images\demo.jpg --output runs\infer
```

## GitHub 提交说明

本仓库用于提交课程项目代码和说明文档。数据集、训练权重、ONNX 模型、量化模型和运行输出不随仓库提交。

如果需要复现完整运行效果，请自行准备：

- YOLO 格式车辆数据集
- 训练得到的 `.pt` 权重文件
- 导出的 `.onnx` 模型文件
- 可选的 ONNX Runtime C++ 开发库

## 课程展示建议

建议按以下顺序展示项目：

1. 打开 Qt 主界面，说明项目是 Qt/C++ 桌面应用。
2. 展示数据集管理页面，说明 YOLO 数据集结构和统计信息。
3. 展示标注页面，演示矩形框创建、类别修改、保存和撤销。
4. 展示训练页面，说明训练参数和实时日志输出。
5. 展示模型部署页面，演示 ONNX 导出和 INT8 动态量化。
6. 展示图像推理页面，选择图片和模型并显示检测结果。
7. 展示结果可视化页面或数据库记录，说明训练和推理结果已经保存。
