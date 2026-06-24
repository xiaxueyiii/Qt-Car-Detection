# CarDet 车辆检测桌面应用使用说明

更新时间：2026-06-04

## 1. 项目简介

CarDet 是一个基于 Qt 6 Widgets + C++ 的车辆识别/车辆检测桌面应用。项目把界面、数据库和流程控制放在 C++ 端，把 YOLO 训练、模型导出、ONNX 量化、单图推理放在 Python 脚本端执行。

应用主要功能包括：

- 数据集扫描与统计保存
- 简易 YOLO 检测框标注
- YOLOv8 模型训练
- `.pt` 模型导出为 `.onnx`
- ONNX Runtime INT8 动态量化
- 单张图片车辆检测推理
- 训练、模型、推理、数据集统计结果可视化
- SQLite 本地记录管理

默认项目路径：

```text
E:\Desktop\FriD\suanfa\CarDet
```

## 2. 项目目录说明

```text
CarDet/
  CarDet.pro                     Qt qmake 工程文件
  README.md                      原始说明文档
  USAGE.md                       本使用说明文档
  requirements.txt               Python 依赖列表
  yolov8n.pt                     默认 YOLOv8n 预训练权重

  src/                           当前 Qt/C++ 主体代码
    main.cpp
    MainWindow.cpp/.h
    model/
      DatabaseManager.cpp/.h     SQLite 数据库读写
      DataTypes.h                数据结构定义
    utils/
      AppConfig.cpp/.h           项目路径、Python 路径等配置
    view/
      DatasetPage.*              数据集统计页
      AnnotationPage.*           标注页
      TrainingPage.*             训练页
      DeployPage.*               模型部署/量化页
      InferencePage.*            图像推理页
      VisualizationPage.*        结果可视化页
      AnnotationCanvas.*         标注画布
    viewmodel/
      DatasetViewModel.*         数据集扫描逻辑
      AnnotationViewModel.*      标注文件读写逻辑
      TrainingViewModel.*        训练进程管理
      DeployViewModel.*          导出/量化进程管理
      InferenceViewModel.*       推理进程管理

  scripts/
    train.py                     YOLO 训练入口
    export_onnx.py               .pt 导出 ONNX
    quantize_onnx.py             ONNX INT8 动态量化
    infer_image.py               单张图片推理

  Vehicles-dataset/              示例车辆数据集
    data.yaml
    dataset.generated.yaml       训练脚本自动生成
    train/images/
    train/labels/
    valid/images/
    valid/labels/
    test/images/
    test/labels/

  database/
    cardet.db                    SQLite 数据库

  models/                        建议保存导出模型、量化模型
  runs/                          训练和推理输出目录
  build*/                        Qt 构建输出目录
  yolov9-main/                   保留的 YOLOv9 代码目录，当前主流程不依赖它
```

## 3. 运行环境准备

### 3.1 Qt/C++ 环境

需要安装：

- Qt 6
- Qt Widgets 模块
- Qt Sql 模块
- SQLite 驱动插件
- 支持 C++17 的编译器，例如 MinGW 64-bit

工程文件是：

```text
E:\Desktop\FriD\suanfa\CarDet\CarDet.pro
```

Qt 工程已经在 `CarDet.pro` 中声明：

```text
QT += widgets sql
CONFIG += c++17
```

### 3.2 Python 环境

训练、导出、量化、推理需要 Python 环境。推荐使用当前项目可访问的 Python 解释器，例如：

```powershell
python --version
```

当前项目推荐使用根目录下的虚拟环境：

```text
E:\Desktop\FriD\suanfa\CarDet\.venv\Scripts\python.exe
```

Qt 程序会优先自动使用这个 `.venv` 解释器；如果设置了 `CARDET_PYTHON`，则以环境变量为准。

安装 Python 依赖：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python -m pip install -r requirements.txt
```

如果需要重新创建 GPU 训练环境，推荐顺序是先安装 CUDA 版 PyTorch，再安装项目依赖：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r requirements-gpu-cu124.txt
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

验证 RTX 3060 是否可用：

```powershell
.\.venv\Scripts\python.exe -c "import torch; print(torch.__version__); print(torch.cuda.is_available()); print(torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'NO CUDA')"
```

正常应看到：

```text
True
NVIDIA GeForce RTX 3060 Laptop GPU
```

`requirements.txt` 当前固定为已验证通过的版本组合：

```text
ultralytics==8.4.60
onnx==1.21.0
onnxruntime==1.18.1
opencv-python==4.10.0.84
PyYAML>=6.0
numpy==1.26.4
```

说明：

- YOLO 训练和推理依赖 `ultralytics`
- 单图推理画框依赖 `opencv-python`
- ONNX 导出依赖 `onnx`
- INT8 动态量化依赖 `onnxruntime`
- 数据集 yaml 解析和生成依赖 `PyYAML`
- 项目训练 YOLO 不需要 TensorFlow

### 3.3 环境变量

项目会自动查找默认根目录。若移动了项目目录，可以设置：

```powershell
$env:CARDET_ROOT='E:\Desktop\FriD\suanfa\CarDet'
```

如果系统里有多个 Python，建议指定解释器：

```powershell
$env:CARDET_PYTHON='E:\anaconda\python.exe'
```

程序内默认使用：

- 项目根目录：`CARDET_ROOT`，未设置时优先尝试 `E:/Desktop/FriD/suanfa/CarDet`
- Python：`CARDET_PYTHON`，未设置时使用 `python`
- 数据集：`项目根目录/Vehicles-dataset`
- 脚本：`项目根目录/scripts`
- 模型目录：`项目根目录/models`
- 输出目录：`项目根目录/runs`
- 数据库：`项目根目录/database/cardet.db`

## 4. 编译与运行

### 4.1 使用 Qt Creator

1. 打开 Qt Creator。
2. 选择 `E:\Desktop\FriD\suanfa\CarDet\CarDet.pro`。
3. 选择 Qt 6 MinGW 64-bit Kit。
4. 点击 Configure Project。
5. 点击 Build。
6. 点击 Run。

启动后主窗口包含 6 个标签页：

- 数据集/统计
- 标注
- 训练
- 模型部署/量化
- 图像推理
- 结果可视化

### 4.2 使用命令行构建

如果已经配置好 Qt 命令行环境，也可以使用 qmake 构建：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
qmake CarDet.pro
mingw32-make
```

不同 Qt Kit 的构建目录可能不同，最终程序一般位于 `build.../debug/CarDet.exe` 或 `build.../release/CarDet.exe`。

## 5. 推荐使用流程

建议按以下顺序使用：

1. 打开程序，进入“数据集/统计”页，确认数据集格式正确。
2. 如需修改标签，进入“标注”页，绘制或修正 YOLO 检测框。
3. 进入“训练”页，选择数据集、权重和训练参数，开始训练。
4. 训练完成后记录 `BEST_MODEL` 路径。
5. 进入“模型部署/量化”页，将 `.pt` 导出为 `.onnx`，再按需做 INT8 量化。
6. 进入“图像推理”页，选择图片和模型，运行检测。
7. 进入“结果可视化”页查看历史记录和统计结果。

## 6. 数据集要求

项目推荐使用 YOLO 检测数据集结构：

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

也支持把验证集目录命名为 `val`：

```text
val/
  images/
  labels/
```

### 6.1 当前示例数据集

当前 `Vehicles-dataset/data.yaml` 中的类别为：

```text
0 Ambulance
1 Bus
2 Car
3 Motorcycle
4 Truck
```

示例数据集大致结构：

```text
train: 878 images
valid: 250 images
test: 126 images
classes: Ambulance, Bus, Car, Motorcycle, Truck
```

### 6.2 YOLO 标签格式

每张图片对应一个同名 `.txt` 标签文件。每行一个目标：

```text
class_id center_x center_y width height
```

坐标使用 0 到 1 的归一化比例，例如：

```text
2 0.512000 0.438000 0.210000 0.180000
```

含义：

- `class_id`：类别编号
- `center_x`：框中心点 x 坐标
- `center_y`：框中心点 y 坐标
- `width`：框宽
- `height`：框高

如果图片在：

```text
train/images/example.jpg
```

标签应保存到：

```text
train/labels/example.txt
```

### 6.3 dataset.generated.yaml

训练脚本会自动生成：

```text
Vehicles-dataset/dataset.generated.yaml
```

它会把 Roboflow `data.yaml` 中可能不适合本机路径的相对路径修正为稳定路径。训练时实际传给 Ultralytics 的是这个自动生成的 yaml。

## 7. 功能页使用说明

### 7.1 数据集/统计

用途：

- 选择数据集目录
- 扫描图片数量、标签数量、类别数量
- 判断是否为 YOLO 检测数据集
- 将统计信息写入 SQLite 数据库
- 将类别名同步给标注页

操作步骤：

1. 在“数据集路径”输入框中确认路径，默认是：

   ```text
   E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset
   ```

2. 点击“选择数据集”可以切换目录。
3. 点击“扫描并保存”。
4. 查看表格中的图片数量、标签数量、类别数量和识别格式。
5. 下方详情区域会显示类别列表和扫描日志。

保存到数据库的表：

```text
dataset_stats
```

### 7.2 标注

用途：

- 加载图片文件夹
- 查看已有 YOLO 标签
- 鼠标拖拽绘制检测框
- 删除选中的检测框
- 保存 YOLO 标签

默认图片目录：

```text
E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset\train\images
```

操作步骤：

1. 在“图片文件夹”处选择图片目录。
2. 点击“加载”。
3. 左侧图片列表选择一张图片。
4. 在类别编号处选择类别，例如：

   ```text
   0 Ambulance
   1 Bus
   2 Car
   3 Motorcycle
   4 Truck
   ```

5. 在右侧图片画布上拖拽绘制矩形框。
6. 如需删除，选中框后点击“删除选中框”。
7. 点击“保存 YOLO 标签”。

保存规则：

- 如果图片位于 `images` 文件夹下，标签会写入同级 `labels` 文件夹。
- 例如 `train/images/a.jpg` 会保存到 `train/labels/a.txt`。
- 标签保存为 YOLO 归一化格式。

注意事项：

- 标注页是课程演示版，支持基本绘框、选择、删除、保存和读取。
- 不包含多边形标注、自动吸附、批量类别编辑等高级功能。
- 建议标注后回到“数据集/统计”页重新扫描一次。

### 7.3 训练

用途：

- 调用 `scripts/train.py`
- 自动检查数据集结构
- 自动生成 `dataset.generated.yaml`
- 使用 Ultralytics YOLO 训练模型
- 输出 `best.pt`
- 保存训练记录到 SQLite

主要参数：

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| 数据集 | YOLO 数据集根目录 | `Vehicles-dataset` |
| 模型类型/权重 | 预训练权重或模型路径 | `yolov8n.pt` |
| epochs | 训练轮数 | `10` |
| batch size | 批大小 | `4` |
| image size | 输入图片尺寸 | `640` |
| learning rate | 初始学习率 | `0.001` |
| device | 训练设备，`auto` 自动优先使用 GPU | `auto` |
| Python | Python 解释器 | `python` 或 `CARDET_PYTHON` |
| 训练脚本 | Python 训练脚本 | `scripts/train.py` |

操作步骤：

1. 确认数据集路径。
2. 确认模型权重，默认可用项目根目录下的：

   ```text
   yolov8n.pt
   ```

3. 设置训练参数。
4. `device` 默认填写 `auto`，检测到 CUDA 版 PyTorch 时会自动使用第 0 张 NVIDIA 显卡；也可以手动填写 `0` 强制使用第一张显卡，填写 `cpu` 强制使用 CPU。
5. 点击“开始训练”。
6. 右侧日志会实时输出训练过程。
7. 训练完成后日志中会出现：

   ```text
   BEST_MODEL: 路径\weights\best.pt
   FINAL_RESULT: {...}
   ```

8. 训练记录会保存到 SQLite 的：

   ```text
   train_records
   ```

训练输出目录：

```text
runs/train/cardet_YYYYMMDD_HHMMSS/
  weights/
    best.pt
    last.pt
  labels.jpg
  results.csv
  results.png
  args.yaml
```

说明：

- `best.pt` 是后续导出和推理推荐使用的模型。
- CPU 训练会比较慢，显存或内存不足时可以降低 `batch size` 或 `image size`。
- 当前训练脚本已经禁用 Ultralytics TensorBoard 回调，YOLO 训练不依赖 TensorFlow。

### 7.4 模型部署/量化

用途：

- 将 `.pt` 模型导出为 `.onnx`
- 对 `.onnx` 模型做 INT8 动态量化
- 将模型记录保存到 SQLite

相关脚本：

```text
scripts/export_onnx.py
scripts/quantize_onnx.py
```

操作步骤：

1. 在 `.pt 模型` 输入框选择训练得到的 `best.pt`。
2. 确认 ONNX 输出路径，默认类似：

   ```text
   models/best.onnx
   ```

3. 点击“导出 ONNX”。
4. 导出成功后日志会出现：

   ```text
   ONNX_MODEL: 路径\best.onnx
   ```

5. 如果需要量化，确认 INT8 输出路径，默认类似：

   ```text
   models/best_int8.onnx
   ```

6. 点击“动态量化 INT8”。
7. 量化成功后日志会出现：

   ```text
   QUANTIZED_MODEL: 路径\best_int8.onnx
   ```

保存到数据库的表：

```text
model_records
```

说明：

- ONNX 导出使用 Ultralytics 的 `model.export(format="onnx")`。
- INT8 动态量化使用 ONNX Runtime 的 `quantize_dynamic`。
- 动态量化不需要校准数据集。

### 7.5 图像推理

用途：

- 选择单张图片
- 选择 `.pt` 或部分 Ultralytics 支持的 `.onnx` 模型
- 运行车辆检测
- 展示原图、结果图和检测表格
- 保存推理记录到 SQLite

默认图片来源：

```text
Vehicles-dataset/test/images
```

默认模型：

```text
yolov8n.pt
```

默认输出目录：

```text
runs/infer
```

操作步骤：

1. 在“图片”处选择待检测图片。
2. 在“模型”处选择 `.pt` 或可用 `.onnx` 模型。
3. 设置置信度阈值 `conf`，默认 `0.25`。
4. 点击“开始识别/检测”。
5. 程序会在输出目录下创建时间戳子目录。
6. 推理完成后显示结果图和检测表格。
7. 点击“保存结果图”可另存当前结果图。

输出文件：

```text
runs/infer/YYYYMMDD_HHMMSS/
  result.jpg
  result.json
```

`result.json` 格式示例：

```json
{
  "image": "path/to/image.jpg",
  "results": [
    {
      "class_id": 2,
      "class_name": "Car",
      "confidence": 0.876,
      "bbox": [100.0, 80.0, 300.0, 220.0]
    }
  ]
}
```

保存到数据库的表：

```text
inference_records
inference_objects
```

### 7.6 结果可视化

用途：

- 查看总推理次数
- 查看最近一次训练模型路径
- 查看最近一次推理时间
- 查看各类别检测数量
- 浏览数据集、训练、模型、推理历史表

页面包含以下记录表：

```text
train_records
model_records
inference_records
dataset_stats
```

操作：

1. 切换到“结果可视化”页会自动刷新。
2. 点击“刷新统计”可以手动刷新。
3. 查看上方摘要和下方表格。

## 8. 命令行脚本使用

除 GUI 外，也可以直接运行 Python 脚本。

### 8.1 训练

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python scripts\train.py --data E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt --lr 0.001 --project E:\Desktop\FriD\suanfa\CarDet\runs\train
```

使用自动 GPU/CPU 选择：

```powershell
python scripts\train.py --data E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt --lr 0.001 --device auto --project E:\Desktop\FriD\suanfa\CarDet\runs\train
```

强制使用第一张 NVIDIA 显卡：

```powershell
python scripts\train.py --data E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt --lr 0.001 --device 0 --project E:\Desktop\FriD\suanfa\CarDet\runs\train
```

关键输出：

```text
DATASET_ROOT: ...
DATASET_IMAGES: ...
DATASET_LABELS: ...
DATASET_YAML: ...
CLASSES: ...
CUDA_AVAILABLE: True
CUDA_DEVICE_0: NVIDIA GeForce RTX 3060 Laptop GPU
BEST_MODEL: ...
FINAL_RESULT: ...
```

### 8.2 导出 ONNX

```powershell
python scripts\export_onnx.py --weights runs\train\cardet_xxx\weights\best.pt --output models\best.onnx
```

关键输出：

```text
ONNX_MODEL: ...
```

### 8.3 INT8 动态量化

```powershell
python scripts\quantize_onnx.py --input models\best.onnx --output models\best_int8.onnx
```

关键输出：

```text
QUANTIZED_MODEL: ...
```

### 8.4 单张图片推理

```powershell
python scripts\infer_image.py --weights runs\train\cardet_xxx\weights\best.pt --image Vehicles-dataset\test\images\example.jpg --output runs\infer\manual_test --conf 0.25
```

关键输出：

```text
DETECTED_COUNT: ...
RESULT_IMAGE: ...
RESULT_JSON: ...
```

## 9. SQLite 数据库说明

数据库路径：

```text
database/cardet.db
```

程序启动时会自动创建以下表：

### 9.1 dataset_stats

记录数据集扫描结果：

```text
id
dataset_path
image_count
label_count
class_count
created_at
```

### 9.2 train_records

记录训练任务：

```text
id
dataset_path
model_type
epochs
batch_size
image_size
result_model_path
status
created_at
```

### 9.3 model_records

记录导出和量化模型：

```text
id
model_path
model_type
quantized_path
quantization_type
created_at
```

### 9.4 inference_records

记录每次图片推理：

```text
id
image_path
model_path
result_image_path
result_json_path
detected_count
created_at
```

### 9.5 inference_objects

记录每个检测目标：

```text
id
inference_id
class_id
class_name
confidence
x1
y1
x2
y2
```

## 10. 常见问题与处理

### 10.1 No module named ultralytics

原因：Python 环境未安装 Ultralytics。

处理：

```powershell
python -m pip install ultralytics
```

或重新安装全部依赖：

```powershell
python -m pip install -r requirements.txt
```

### 10.2 No module named cv2

原因：缺少 OpenCV。

处理：

```powershell
python -m pip install opencv-python
```

### 10.3 No module named onnxruntime

原因：缺少 ONNX Runtime。

处理：

```powershell
python -m pip install onnxruntime
```

### 10.4 No module named yaml

原因：缺少 PyYAML，训练脚本无法读取或生成 yaml。

处理：

```powershell
python -m pip install PyYAML
```

### 10.5 Dataset is missing train/images or train/labels

原因：数据集不是标准 YOLO 检测格式，或路径选错。

检查：

```text
dataset/
  train/images
  train/labels
  valid/images 或 val/images
  valid/labels 或 val/labels
```

处理：

- 回到“数据集/统计”页确认数据集根目录
- 确认不要选到 `train/images`，训练页应选择数据集根目录
- 如无标签，先进入“标注”页生成 YOLO `.txt` 标签

### 10.6 module 'tensorflow' has no attribute 'io'

原因：Python 环境中的 TensorFlow 安装残缺，TensorBoard 回调导入时访问 `tf.io` 失败。

当前项目处理方式：

- `scripts/train.py` 已禁用 Ultralytics 的 TensorBoard 回调
- YOLO 训练不依赖 TensorFlow
- 正常训练不需要安装或修复 TensorFlow

如果仍然出现该错误，请确认 GUI 训练页使用的是当前项目的：

```text
scripts/train.py
```

### 10.7 Qt 数据库初始化失败

可能原因：

- Qt Kit 缺少 SQLite 驱动插件
- `database/` 目录无写入权限
- 数据库文件被其他程序占用

处理：

- 确认 Qt 安装包含 SQLite driver
- 删除或备份 `database/cardet.db` 后重启程序
- 确认项目目录可写

### 10.8 训练速度慢

可能原因：

- 当前使用 CPU 训练
- `image size` 或 `batch size` 较大
- 数据集图片数量较多

处理建议：

- 降低 `batch size`
- 降低 `image size`，例如从 `640` 降到 `320`
- 测试流程时先把 `epochs` 设置为 `1`
- 有 NVIDIA GPU 时安装合适的 CUDA 版 PyTorch

### 10.9 有 RTX 3060 但日志仍显示 CPU

如果训练日志显示类似：

```text
torch-2.0.1+cpu CPU
CUDA_AVAILABLE: False
```

说明当前 Qt 训练页使用的 Python 环境安装的是 CPU 版 PyTorch，不是程序没有识别到显卡。

处理步骤：

1. 确认训练页 `Python` 输入框使用的解释器。
2. 在同一个解释器环境里安装 CUDA 版 PyTorch。
3. 安装后验证：

   ```powershell
   python -c "import torch; print(torch.__version__); print(torch.cuda.is_available()); print(torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'NO CUDA')"
   ```

4. 如果输出 `True` 和 `NVIDIA GeForce RTX 3060 Laptop GPU`，训练页 `device=auto` 或 `device=0` 就会使用显卡。

### 10.10 模型路径找不到

处理：

- 如果使用默认 `yolov8n.pt`，确认项目根目录存在该文件
- 如果选择训练结果，推荐选择：

  ```text
  runs/train/cardet_xxx/weights/best.pt
  ```

- 如果移动模型文件，重新在界面中选择新路径

## 11. 交付与演示建议

推荐演示顺序：

1. 打开程序，展示主界面和 6 个功能页。
2. 在“数据集/统计”页扫描 `Vehicles-dataset`。
3. 在“标注”页加载 `train/images`，展示已有标签回显和绘框保存。
4. 在“训练”页用 `epochs=1` 演示训练流程，正式训练可使用更多轮数。
5. 选择训练得到的 `best.pt`，导出 `models/best.onnx`。
6. 对 `models/best.onnx` 做 `models/best_int8.onnx` 量化。
7. 在“图像推理”页选择测试图片和模型，运行检测。
8. 在“结果可视化”页展示数据库记录和统计结果。

演示用快速训练参数建议：

```text
epochs: 1
batch size: 4 或 8
image size: 320 或 640
model: yolov8n.pt
learning rate: 0.001
```

正式训练可以提高：

```text
epochs: 50 或 100
batch size: 根据显存/内存调整
image size: 640
```

## 12. 维护建议

- 训练得到的重要模型建议复制到 `models/` 目录统一管理。
- 每次改动数据集后，先重新扫描数据集再训练。
- 标注时保持类别编号与 `data.yaml` 中 `names` 顺序一致。
- 推理输出目录 `runs/infer` 会不断累积结果，定期清理无用结果。
- 数据库 `database/cardet.db` 记录演示过程，交付前可按需要保留或清空。
- 如果移动项目目录，优先设置 `CARDET_ROOT`，避免默认路径失效。
- 如果 Python 环境变更，优先设置 `CARDET_PYTHON`，避免 Qt 调错解释器。
