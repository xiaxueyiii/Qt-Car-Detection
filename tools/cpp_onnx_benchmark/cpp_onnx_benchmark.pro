QT += core gui

CONFIG += console c++20
CONFIG -= app_bundle
TEMPLATE = app
TARGET = cpp_onnx_benchmark

SOURCES += \
    main.cpp \
    ../../src/model/QuantizedOnnxRunner.cpp

HEADERS += \
    ../../src/model/DataTypes.h \
    ../../src/model/QuantizedOnnxRunner.h

INCLUDEPATH += ../../src

ONNXRUNTIME_DIR = $$(ONNXRUNTIME_DIR)
isEmpty(ONNXRUNTIME_DIR) {
    exists($$PWD/../../third_party/onnxruntime-win-x64-1.20.1/include/onnxruntime_cxx_api.h) {
        ONNXRUNTIME_DIR = $$PWD/../../third_party/onnxruntime-win-x64-1.20.1
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
