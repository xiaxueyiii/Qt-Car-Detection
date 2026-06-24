# 车辆识别图形界面应用程序

这是一个基于 Qt 6 Widgets + C++ 的课程作业版车辆检测桌面应用，包含数据集统计、简化 YOLO 标注、训练、模型导出/量化、单图推理和结果可视化。

## 目录说明

- `CarDet.pro`：Qt Creator 可直接打开的 qmake 工程文件。
- `src/`：Qt/C++ 源码，按 View、ViewModel、Model 分层组织。
- `scripts/`：Python 训练、ONNX 导出、INT8 量化、单图推理脚本。
- `Vehicles-dataset/`：当前车辆数据集。已识别为 YOLO 检测格式。
- `database/cardet.db`：SQLite 数据库，程序启动后自动创建。
- `models/`：建议保存训练模型、ONNX、量化模型。
- `runs/`：训练和推理输出目录。
- `backup_old_qt_*`：重构前旧 Qt 源码备份。

## 环境依赖

Qt/C++：

- Qt 6
- Qt Widgets
- Qt Sql / SQLite 驱动
- 支持 C++17 的编译器，例如 MinGW 64-bit

Python：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
python -m pip install -r requirements.txt
```

如果只演示界面、数据集扫描、标注和数据库，不安装 Python 依赖也可以运行 Qt 程序；训练、导出、量化、推理会在日志中提示缺失依赖。

## 用 Qt Creator 编译运行

1. 打开 Qt Creator。
2. 选择 `E:\Desktop\FriD\suanfa\CarDet\CarDet.pro`。
3. 选择 Qt 6 MinGW 64-bit Kit。
4. Configure Project。
5. Build。
6. Run。

程序默认项目根目录为 `E:\Desktop\FriD\suanfa\CarDet`。如需放到其他目录，可设置环境变量：

```powershell
$env:CARDET_ROOT='E:\Desktop\FriD\suanfa\CarDet'
```

如需指定 Python：

```powershell
$env:CARDET_PYTHON='C:\Path\To\python.exe'
```

## 数据集要求

推荐 YOLO 检测格式：

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

当前数据集扫描结果：

- train：878 张图片，878 个标签
- valid：250 张图片，250 个标签
- test：126 张图片，126 个标签
- 类别：Ambulance、Bus、Car、Motorcycle、Truck

训练脚本会自动生成 `Vehicles-dataset/dataset.generated.yaml`，修正 Roboflow `data.yaml` 里可能存在的相对路径问题。

## 功能测试

数据集/统计：

1. 打开“数据集/统计”页。
2. 默认路径为 `Vehicles-dataset`。
3. 点击“扫描并保存”。
4. 查看图片数、标签数、类别列表。
5. 切到“结果可视化”可看到 `dataset_stats` 记录。

标注：

1. 打开“标注”页。
2. 默认加载 `Vehicles-dataset/train/images`。
3. 选择图片，已有 YOLO 标签会自动回显。
4. 左侧设置类别编号和名称。
5. 在图片上鼠标拖拽绘制矩形框。
6. 点击“保存 YOLO 标签”，会保存到同 split 的 `labels/同名.txt`。

训练：

```powershell
python scripts/train.py --data E:\Desktop\FriD\suanfa\CarDet\Vehicles-dataset --epochs 10 --batch 4 --imgsz 640 --model yolov8n.pt
```

也可以在“训练”页点击“开始训练”。训练结束后会向日志输出 `BEST_MODEL:`，并保存训练记录到 SQLite。

ONNX 导出：

```powershell
python scripts/export_onnx.py --weights models\best.pt --output models\best.onnx
```

INT8 动态量化：

```powershell
python scripts/quantize_onnx.py --input models\best.onnx --output models\best_int8.onnx
```

单图推理：

```powershell
python scripts/infer_image.py --weights models\best.pt --image Vehicles-dataset\test\images\xxx.jpg --output runs\infer
```

脚本会输出：

- `runs/infer/.../result.jpg`
- `runs/infer/.../result.json`

Qt “图像推理”页会读取并展示这两个文件。

## 常见错误

- `No module named ultralytics`：执行 `python -m pip install ultralytics`。
- `No module named onnxruntime`：执行 `python -m pip install onnxruntime`。
- `No module named cv2`：执行 `python -m pip install opencv-python`。
- 训练提示缺少 `train/images` 或 `train/labels`：数据集不是 YOLO 检测格式，先在“标注”页补齐标签或转换数据集。
- Qt 数据库初始化失败：确认 Qt 安装包含 SQLite 驱动插件。

## 当前限制

- 标注工具实现的是课程演示版：支持绘框、选择、删除、保存和读取 YOLO 标签，不包含多边形、自动吸附、批量类别编辑。
- C++ 端主要负责 GUI、数据库和流程编排；训练、导出、量化、推理由 Python 脚本完成。
- 推理默认使用 ultralytics，模型文件可以是 `.pt`，部分 `.onnx` 是否可推理由 ultralytics 当前环境决定。
- INT8 量化采用 ONNX Runtime 动态量化，未实现带校准集的静态量化。
