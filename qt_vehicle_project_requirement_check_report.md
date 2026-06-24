# Qt 车辆识别项目课程验收检查报告

生成时间：2026-06-23  
项目路径：`E:\Desktop\FriD\suanfa\CarDet`

## 1. 项目概况

### 总体结论

项目主体符合“Qt + C++ 桌面应用程序”资格要求。当前实际构建入口是 `CarDet.pro`，使用 Qt Widgets 和 Qt Sql，主入口为 `src/main.cpp`，主窗口为 `src/MainWindow.cpp` 中的 `MainWindow : QMainWindow`。

目前项目已经覆盖课程验收的大部分功能：数据集管理、矩形框标注、类别管理、撤销重做、训练参数设置、训练日志、训练结果入库、ONNX 导出、ONNX Runtime 动态量化、Python 推理、结果展示和数据库保存均有真实代码入口。

仍需人工确认的主要风险是：默认构建未启用 ONNX Runtime C++ SDK。代码已经补全 `CARDET_USE_ONNXRUNTIME` 条件编译下的 C++ ONNX Runtime 推理流程，但需要本机安装并配置 ONNX Runtime C++ 开发包后才能现场演示“C++ 加载量化 ONNX 模型推理”。

### 项目结构

| 路径 | 作用 |
|---|---|
| `CarDet.pro` | qmake 构建文件，当前有效工程入口 |
| `src/main.cpp` | Qt 应用入口，创建 `QApplication` 和 `MainWindow` |
| `src/MainWindow.cpp/.h` | 主窗口、菜单栏、图标工具栏、状态栏、页面 Tab |
| `src/view/DatasetPage.*` | 数据集创建、打开、导入图片、扫描统计、图像/标注列表 |
| `src/view/AnnotationPage.*` | 标注页面、类别管理、标注表格、图片导航 |
| `src/view/AnnotationCanvas.*` | 标注画布，矩形框创建、移动、缩放、删除、撤销/重做 |
| `src/view/TrainingPage.*` | 训练参数 GUI、训练日志、指标表格和条形可视化 |
| `src/view/DeployPage.*` | ONNX 导出、动态 INT8 量化 GUI |
| `src/view/InferencePage.*` | 图片选择、模型选择、推理执行、结果图和检测表格 |
| `src/view/VisualizationPage.*` | 数据库记录、训练、模型、推理、标注等结果可视化 |
| `src/model/DatabaseManager.*` | SQLite 初始化、数据集/标注/训练/模型/推理入库 |
| `src/model/QuantizedOnnxRunner.*` | C++ ONNX Runtime 条件编译推理实现 |
| `src/viewmodel/*.cpp` | QProcess 调用训练、导出、量化、推理脚本 |
| `scripts/train.py` | Ultralytics YOLO 训练入口，输出 `BEST_MODEL` 和 `FINAL_RESULT` |
| `scripts/export_onnx.py` | `.pt` 导出 `.onnx` |
| `scripts/quantize.py`、`scripts/quantize_onnx.py` | ONNX Runtime 动态 INT8/PTQ 量化 |
| `scripts/infer_image.py` | Python 加载模型进行单图推理并输出结果图/JSON |
| `database/cardet.db` | SQLite 数据库，程序自动创建 |
| `Vehicles-dataset/` | 示例 YOLO 检测数据集 |

根目录下的 `mainwindow.cpp/.h/.ui`、`trainpage.cpp/.h` 等是旧版文件，未被 `CarDet.pro` 编译；当前验收应以 `src/` 下代码为准。

### 构建方式与依赖

- 构建方式：qmake
- Qt 模块：`QT += widgets sql`
- C++ 标准：`CONFIG += c++20`
- 本次验证 Qt：`C:\Qt\6.11.0\mingw_64`
- 本次验证编译器：`C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe`
- Python 依赖：`ultralytics==8.4.60`、`onnx==1.19.1`、`onnxruntime==1.20.1`、`opencv-python==4.10.0.84`、`PyYAML`、`numpy==1.26.4`、`protobuf==4.25.3`

## 2. 逐项功能核查

| 模块 | 验收要求 | 当前状态 | 证据文件/函数 | 存在问题 | 处理方式 |
|---|---|---|---|---|---|
| 资格条件 | 必须使用 Qt、C++ 开发桌面应用程序 | 已实现 | `CarDet.pro` 使用 `QT += widgets sql`；`src/main.cpp` 创建 `QApplication`；`MainWindow : QMainWindow` | Python 负责算法脚本，但由 Qt/C++ 调度 | 保持 Qt/C++ 桌面主体，通过 `QProcess` 调 Python |
| 标注工具 | 数据集管理功能 | 已实现 | `DatasetPage::createDataset()`、`chooseDataset()`、`importImageFolder()`、`scanDataset()` | 导入图片默认进入 `train/images`，没有自动划分训练/验证 | 已具备课程演示需要；演示时说明数据集划分由目录决定 |
| 标注工具 | 矩形框创建、删除、移动、编辑 | 已实现 | `AnnotationCanvas::mouseReleaseEvent()` 创建；`deleteSelected()` 删除；`mouseMoveEvent()` 的 `MovingBox` 和 `ResizingBox`；`AnnotationPage::onBoxTableItemChanged()` 表格编辑坐标 | 复杂旋转视图下建议演示常规 0 度标注 | 已实现 |
| 标注工具 | 类别新建、删除 | 已实现 | `AnnotationPage::addCategory()`、`deleteCategory()`、`saveClassNamesToDatasetYaml()`、`DatabaseManager::saveCategories()` | 删除类别不会自动重编号历史标注框 | 保留现状，避免破坏已有 YOLO class id |
| 标注工具 | 标注框类别信息更改 | 已实现 | `AnnotationPage::applyClassToSelectedBox()`；`AnnotationCanvas::updateSelectedClass()`；标注表格双击编辑类别列 | 无明显问题 | 已修复当前类别 ID 与类别名同步 |
| 标注工具 | 撤销与重做 | 已实现 | `AnnotationCanvas::undo()`、`redo()`、`pushHistory()`、`commitDragHistoryIfNeeded()` | 撤销栈限制 80 步 | 满足课程演示 |
| 标注工具 | 已标注数据集管理 | 已实现 | `DatasetPage::refreshImageTable()` 显示每张图标注数和类别 ID；`AnnotationPage::imageListText()` 显示 `[n框/类别]`；`DatabaseManager::replaceImageAnnotations()` 入库 | 没有高级筛选/批量操作 | 现有列表和数据库记录满足基础管理 |
| 训练功能 | GUI 设置训练集与验证集 | 已实现 | `TrainingPage` 的 `m_trainEdit`、`m_valEdit`；`chooseTrainPath()`、`chooseValPath()`；`TrainingViewModel::startTraining()` 传 `--train`、`--val` | 需要选择 images 目录 | 已实现 |
| 训练功能 | GUI 设置训练超参数 | 已实现 | `TrainingPage` 中 epochs、batch size、image size、learning rate、device、model、output dir 控件 | 未暴露全部 YOLO 高级参数 | 满足课程常规超参数要求 |
| 训练功能 | 实时显示训练过程信息 | 已实现 | `TrainingViewModel` 用 `QProcess::readyReadStandardOutput` 读取日志；`TrainingPage::appendLog()` 显示 | 无明显问题 | 已实现 |
| 训练功能 | 显示训练后的性能指标与模型文件 | 已实现 | `scripts/train.py` 输出 `BEST_MODEL:`、`FINAL_RESULT:`；`TrainingViewModel::handleLine()` 解析；`TrainingPage::showMetrics()` 展示 | 依赖 Ultralytics 生成 `results.csv` | 已实现 |
| 训练功能 | 性能指标界面可视化 | 已实现 | `TrainingPage::showMetricBars()` 以条形图展示 mAP、precision、recall；`VisualizationPage` 展示训练记录表 | 曲线图仍使用 Ultralytics 输出 `results.png`，GUI 当前展示为条形图/表格 | 本次已补强 |
| 训练功能 | 训练结果保存到数据库 | 已实现 | `DatabaseManager::insertTrainRecord()`、`insertTrainingRun()`、`finishTrainingRun()` | 无明显问题 | 已实现 |
| 训练功能 | C++ 调用 Python 训练程序 | 已实现 | `TrainingViewModel::startTraining()` 使用 `QProcess` 调 `scripts/train.py` | 无明显问题 | 已实现 |
| GUI 功能 | 菜单、工具栏、状态栏、视图布局合理 | 已实现 | `MainWindow::setupChrome()` 创建菜单、图标工具栏、状态栏；`QTabWidget` 组织六个页面 | 工具栏是图标模式，需要鼠标悬停看提示 | 本次已恢复紧凑图标工具栏 |
| GUI 功能 | 训练和推理参数通过 GUI 设置 | 已实现 | `TrainingPage`、`InferencePage` 的参数控件和 `QFileDialog` | 无明显问题 | 已实现 |
| GUI 功能 | 图像放大、缩小、平移、旋转 | 已实现 | `AnnotationCanvas::zoomIn()`、`zoomOut()`、`fitToWindow()`、`rotateLeft()`、`rotateRight()`；右键/中键平移 | 推理结果图是静态预览，交互主要在标注画布 | 满足图像操作演示 |
| GUI 功能 | 列表显示图像、标注信息 | 已实现 | `DatasetPage::refreshImageTable()`；`AnnotationPage` 图片列表和标注表格；`VisualizationPage` 图像/标注数据库表 | 无明显问题 | 已实现 |
| 量化功能 | 支持训练后量化或 QAT | 已实现 | `DeployPage::quantizeOnnx()`；`scripts/quantize.py`；`scripts/quantize_onnx.py` | 当前为 PTQ 动态量化，不是 QAT | 满足“至少一种”量化类型 |
| 量化功能 | 支持 ONNX Runtime/TensorRT/PyTorch 量化路径 | 已实现 | `scripts/quantize_onnx.py` 使用 `onnxruntime.quantization.quantize_dynamic` | 依赖 Python onnxruntime | 已实现 |
| 推理功能 | 支持多种图像文件 | 已实现 | `InferencePage::chooseImage()` 支持 jpg/jpeg/png/bmp/webp；`DatasetViewModel` 和 `AnnotationViewModel` 同样支持多格式 | 无明显问题 | 已实现 |
| 推理功能 | 通过 GUI 对话框选择图像 | 已实现 | `InferencePage::chooseImage()` 使用 `QFileDialog::getOpenFileName` | 无明显问题 | 已实现 |
| 推理功能 | 推理结果 GUI 展示 | 已实现 | `InferencePage::showImage()` 显示原图/结果图；`loadResults()` 填充检测表格 | QLabel 预览不支持缩放平移 | 已实现基础展示 |
| 推理功能 | Python 加载训练后的模型推理 | 已实现 | `InferenceViewModel::runInference()` 调 `scripts/infer_image.py`；脚本 `YOLO(str(args.weights))` | 依赖 ultralytics | 已实现 |
| 推理功能 | C++ 加载量化后的模型推理 | 部分实现 | `QuantizedOnnxRunner::run()` 已补全 `CARDET_USE_ONNXRUNTIME` 下的 C++ 预处理、ORT session、输出解析、NMS、绘图和 JSON；`CarDet.pro` 支持 `ONNXRUNTIME_DIR` | 默认构建未配置 ONNX Runtime C++ SDK，现场不设置依赖时会提示并回退 Python | 代码已补全条件实现；要满分演示需配置 ONNX Runtime C++ |
| 推理功能 | 推理结果保存入数据库 | 已实现 | `DatabaseManager::insertInferenceRecord()`、`insertInferenceObject()`；C++ 成功路径和 Python 路径均调用 | 默认后端记录为 Python Ultralytics；C++ 路径需启用 ORT | 已实现 |

## 3. 本次已补全的功能

1. 恢复并优化主窗口工具栏  
   文件：`src/MainWindow.cpp`  
   说明：新增紧凑图标工具栏，保留菜单和状态栏，工具栏包含页面切换与刷新结果。

2. 增加训练指标条形可视化  
   文件：`src/view/TrainingPage.cpp`、`src/view/TrainingPage.h`  
   说明：训练完成后解析 `FINAL_RESULT` 中的 mAP、precision、recall 等指标，用条形图在 GUI 中展示。

3. 补强 C++ ONNX Runtime 量化模型推理条件实现  
   文件：`src/model/QuantizedOnnxRunner.cpp`、`CarDet.pro`、`src/view/InferencePage.cpp`  
   说明：在定义 `CARDET_USE_ONNXRUNTIME` 并配置 ONNX Runtime C++ SDK 时，C++ 端可读取图片、构造 NCHW 输入、执行 ONNX Runtime Session、解析 YOLO 输出、NMS、绘制结果图、保存 JSON，并写入数据库。

4. 保持默认可编译降级  
   文件：`src/model/QuantizedOnnxRunner.cpp`  
   说明：未配置 ONNX Runtime C++ 时，默认构建仍成功，GUI 会提示 C++ Runtime 未启用并回退 Python 推理。

5. 保留之前已修复功能  
   包括：当前类别 ID 与类别名同步、标注页上一张/下一张单击切换、量化脚本避免 ONNX/ORT Windows 崩溃、量化输出可被 ONNX Runtime 加载。

## 4. 构建与验证结果

### Qt 构建结果

已执行构建：

```powershell
$env:PATH = 'C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cd E:\Desktop\FriD\suanfa\CarDet
mkdir build_requirement_check_final
cd build_requirement_check_final
C:\Qt\6.11.0\mingw_64\bin\qmake.exe ..\CarDet.pro
C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe -j4
```

结果：构建成功。  
生成文件：`E:\Desktop\FriD\suanfa\CarDet\build_requirement_check_final\release\CarDet.exe`

### Python 脚本语法检查

已执行：

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
.\.venv\Scripts\python.exe -m py_compile scripts\train.py scripts\export_onnx.py scripts\quantize.py scripts\quantize_onnx.py scripts\infer_image.py
```

结果：通过。

## 5. 构建运行方法

### 常规 Qt Creator 运行

1. 打开 Qt Creator。
2. 打开 `E:\Desktop\FriD\suanfa\CarDet\CarDet.pro`。
3. 选择 Qt 6 MinGW 64-bit Kit。
4. Configure Project。
5. Build。
6. Run。

### 命令行构建

```powershell
$env:PATH = 'C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
cd E:\Desktop\FriD\suanfa\CarDet
mkdir build
cd build
C:\Qt\6.11.0\mingw_64\bin\qmake.exe ..\CarDet.pro
C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe -j4
```

### Python 依赖安装

```powershell
cd E:\Desktop\FriD\suanfa\CarDet
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

如需 RTX 3060 GPU 训练，先确认 CUDA 版 PyTorch：

```powershell
.\.venv\Scripts\python.exe -c "import torch; print(torch.cuda.is_available()); print(torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'CPU')"
```

### 启用 C++ ONNX Runtime 推理

默认构建不启用 C++ ONNX Runtime。若要演示该项，请安装 ONNX Runtime C++ 包，并设置：

```powershell
$env:ONNXRUNTIME_DIR='D:\onnxruntime-win-x64-gpu-or-cpu'
$env:PATH="$env:ONNXRUNTIME_DIR\lib;$env:PATH"
```

然后重新 qmake 和构建。`ONNXRUNTIME_DIR` 下应包含：

```text
include/onnxruntime_cxx_api.h
lib/onnxruntime.dll 或对应链接库
```

## 6. 手动测试步骤

### 数据集管理

1. 打开“数据集/统计”页。
2. 点击“打开数据集”选择 `Vehicles-dataset`。
3. 点击“扫描并保存”。
4. 检查左侧统计、右侧图像和标注列表、结果可视化页的数据库记录。

### 标注功能

1. 打开“标注”页。
2. 加载 `Vehicles-dataset/train/images`。
3. 点击图片列表中的图片，确认已有框回显。
4. 修改“当前类别”，确认类别名同步。
5. 在图上拖拽创建矩形框。
6. 点击已有框拖动移动，拖四角缩放。
7. 点击“撤销”“重做”。
8. 双击标注表格修改类别或坐标。
9. 点击“保存标签”，再查看数据库记录。

### 训练功能

1. 打开“训练”页。
2. 选择数据集根目录、训练集 images、验证集 images。
3. 设置 epochs、batch size、image size、learning rate、device。
4. 点击“开始训练”。
5. 观察实时日志。
6. 训练结束后展示模型路径、指标表和条形指标可视化。

课堂快速演示建议：`epochs=1`，`batch size=4`，`device=0` 或 `auto`。

### 量化功能

1. 打开“模型部署/量化”页。
2. 选择训练得到的 `best.pt`。
3. 点击“导出 ONNX”。
4. 点击“动态量化 INT8”。
5. 日志中应出现 `ONNX_MODEL:`、`QUANTIZED_MODEL:`。
6. 到“结果可视化”页检查模型记录。

### 推理功能

1. 打开“图像推理”页。
2. 选择 jpg/png/bmp/webp 图片。
3. 选择 `.pt` 或 `.onnx` 模型。
4. 设置置信度 `conf`。
5. 点击“开始识别/检测”。
6. 查看结果图、检测表格和日志。
7. 检查 SQLite 中 `inference_records`、`inference_objects`、`inference_results`。

如配置了 ONNX Runtime C++，勾选“ONNX 量化模型优先使用 C++ Runtime”，选择量化后的 `.onnx` 模型进行演示。

## 7. 课堂验收演示建议

建议按以下顺序演示：

1. 打开主程序，展示菜单栏、图标工具栏、状态栏和六个功能页。
2. 数据集/统计页：扫描 `Vehicles-dataset`，展示图片数量、标签数量、类别列表、图像标注表。
3. 标注页：加载图片，演示画框、移动、缩放、删除、撤销、重做、类别新增/删除、类别修改、保存 YOLO 标签。
4. 训练页：设置训练/验证集和超参数，使用 `epochs=1` 演示 QProcess 实时日志，展示训练指标表和条形图。
5. 模型部署/量化页：导出 ONNX，再执行动态 INT8 量化，展示日志和输出路径。
6. 图像推理页：选择测试图片和模型，运行检测，展示结果图、检测表格和保存结果图。
7. 结果可视化页：刷新统计，展示训练、模型、推理、图像、类别、标注等数据库表。

## 8. 最终判断

当前项目已经基本满足课程验收要求。  

严格按“所有可选项也当作必做项”判断，唯一需要额外环境确认的是“C++ 加载量化后的模型进行推理”：代码层面已经实现条件编译版本，但默认构建没有启用 ONNX Runtime C++ SDK。如果课堂验收要求必须现场跑通 C++ ONNX Runtime 推理，请提前配置 `ONNXRUNTIME_DIR` 并重新构建；否则默认程序会自动回退 Python 推理，这一项可能被老师视为部分实现。

