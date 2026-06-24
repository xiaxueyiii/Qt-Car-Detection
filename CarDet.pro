QT += widgets sql

CONFIG += c++20
TEMPLATE = app
TARGET = CarDet

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/model/DatabaseManager.cpp \
    src/model/QuantizedOnnxRunner.cpp \
    src/utils/AppConfig.cpp \
    src/view/AnnotationCanvas.cpp \
    src/view/AnnotationPage.cpp \
    src/view/DatasetPage.cpp \
    src/view/DeployPage.cpp \
    src/view/InferencePage.cpp \
    src/view/TrainingPage.cpp \
    src/view/VisualizationPage.cpp \
    src/viewmodel/AnnotationViewModel.cpp \
    src/viewmodel/DatasetViewModel.cpp \
    src/viewmodel/DeployViewModel.cpp \
    src/viewmodel/InferenceViewModel.cpp \
    src/viewmodel/TrainingViewModel.cpp

HEADERS += \
    src/MainWindow.h \
    src/model/DataTypes.h \
    src/model/DatabaseManager.h \
    src/model/QuantizedOnnxRunner.h \
    src/utils/AppConfig.h \
    src/view/AnnotationCanvas.h \
    src/view/AnnotationPage.h \
    src/view/DatasetPage.h \
    src/view/DeployPage.h \
    src/view/InferencePage.h \
    src/view/TrainingPage.h \
    src/view/VisualizationPage.h \
    src/viewmodel/AnnotationViewModel.h \
    src/viewmodel/DatasetViewModel.h \
    src/viewmodel/DeployViewModel.h \
    src/viewmodel/InferenceViewModel.h \
    src/viewmodel/TrainingViewModel.h

DISTFILES += \
    README.md \
    USAGE.md \
    requirements.txt \
    requirements-gpu-cu124.txt \
    scripts/train.py \
    scripts/export_onnx.py \
    scripts/quantize.py \
    scripts/quantize_onnx.py \
    scripts/infer_image.py

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

ONNXRUNTIME_DIR = $$(ONNXRUNTIME_DIR)
isEmpty(ONNXRUNTIME_DIR) {
    exists($$PWD/third_party/onnxruntime-win-x64-1.20.1/include/onnxruntime_cxx_api.h) {
        ONNXRUNTIME_DIR = $$PWD/third_party/onnxruntime-win-x64-1.20.1
    }
}
!isEmpty(ONNXRUNTIME_DIR) {
    DEFINES += CARDET_USE_ONNXRUNTIME
    INCLUDEPATH += $$ONNXRUNTIME_DIR/include
    LIBS += -L$$ONNXRUNTIME_DIR/lib -lonnxruntime

    win32 {
        CONFIG(debug, debug|release) {
            ONNXRUNTIME_TARGET_DIR = $$OUT_PWD/debug
        } else {
            ONNXRUNTIME_TARGET_DIR = $$OUT_PWD/release
        }
        QMAKE_POST_LINK += $$quote(cmd /c copy /Y "$$shell_path($$ONNXRUNTIME_DIR/lib/onnxruntime.dll)" "$$shell_path($$ONNXRUNTIME_TARGET_DIR/onnxruntime.dll)") $$escape_expand(\n\t)
        QMAKE_POST_LINK += $$quote(cmd /c copy /Y "$$shell_path($$ONNXRUNTIME_DIR/lib/onnxruntime_providers_shared.dll)" "$$shell_path($$ONNXRUNTIME_TARGET_DIR/onnxruntime_providers_shared.dll)") $$escape_expand(\n\t)
    }
}
