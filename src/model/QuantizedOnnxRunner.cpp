#include "QuantizedOnnxRunner.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QRectF>

#ifdef CARDET_USE_ONNXRUNTIME
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

#ifndef _Frees_ptr_opt_
#define _Frees_ptr_opt_
#endif

#include <onnxruntime_cxx_api.h>
#endif

#ifdef CARDET_USE_ONNXRUNTIME
namespace {
struct Candidate
{
    int classId = 0;
    double confidence = 0.0;
    QRectF box;
};

double boxIou(const QRectF &a, const QRectF &b)
{
    const QRectF inter = a.intersected(b);
    if (inter.isEmpty()) {
        return 0.0;
    }
    const double interArea = inter.width() * inter.height();
    const double unionArea = a.width() * a.height() + b.width() * b.height() - interArea;
    return unionArea > 0.0 ? interArea / unionArea : 0.0;
}

QVector<Candidate> nonMaximumSuppression(QVector<Candidate> candidates, double iouThreshold = 0.45)
{
    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
        return a.confidence > b.confidence;
    });

    QVector<Candidate> kept;
    for (const Candidate &candidate : candidates) {
        bool overlapped = false;
        for (const Candidate &selected : kept) {
            if (candidate.classId == selected.classId && boxIou(candidate.box, selected.box) > iouThreshold) {
                overlapped = true;
                break;
            }
        }
        if (!overlapped) {
            kept.append(candidate);
        }
        if (kept.size() >= 100) {
            break;
        }
    }
    return kept;
}

std::vector<float> imageToTensor(const QImage &image, int width, int height)
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB888)
                           .scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    std::vector<float> tensor(3 * width * height);
    const int channelSize = width * height;
    for (int y = 0; y < height; ++y) {
        const uchar *line = rgb.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            const int pixel = y * width + x;
            tensor[pixel] = line[x * 3] / 255.0f;
            tensor[channelSize + pixel] = line[x * 3 + 1] / 255.0f;
            tensor[channelSize * 2 + pixel] = line[x * 3 + 2] / 255.0f;
        }
    }
    return tensor;
}

QVector<Candidate> parseYoloOutput(const float *data,
                                   const std::vector<int64_t> &shape,
                                   const QSize &sourceSize,
                                   int inputWidth,
                                   int inputHeight,
                                   double confidence)
{
    QVector<Candidate> candidates;
    if (!data || shape.size() < 3) {
        return candidates;
    }

    int attrs = 0;
    int boxes = 0;
    bool attrsFirst = false;
    if (shape[1] <= 64 && shape[2] > shape[1]) {
        attrs = static_cast<int>(shape[1]);
        boxes = static_cast<int>(shape[2]);
        attrsFirst = true;
    } else {
        boxes = static_cast<int>(shape[1]);
        attrs = static_cast<int>(shape[2]);
    }
    if (attrs < 5 || boxes <= 0) {
        return candidates;
    }

    const int classCount = attrs - 4;
    const double scaleX = static_cast<double>(sourceSize.width()) / qMax(1, inputWidth);
    const double scaleY = static_cast<double>(sourceSize.height()) / qMax(1, inputHeight);

    auto valueAt = [data, attrs, boxes, attrsFirst](int attr, int boxIndex) {
        return attrsFirst ? data[attr * boxes + boxIndex] : data[boxIndex * attrs + attr];
    };

    for (int i = 0; i < boxes; ++i) {
        int classId = 0;
        float bestScore = 0.0f;
        for (int c = 0; c < classCount; ++c) {
            const float score = valueAt(4 + c, i);
            if (score > bestScore) {
                bestScore = score;
                classId = c;
            }
        }
        if (bestScore < confidence) {
            continue;
        }

        const double cx = valueAt(0, i);
        const double cy = valueAt(1, i);
        const double w = valueAt(2, i);
        const double h = valueAt(3, i);
        QRectF rect((cx - w / 2.0) * scaleX,
                    (cy - h / 2.0) * scaleY,
                    w * scaleX,
                    h * scaleY);
        rect = rect.normalized().intersected(QRectF(QPointF(0, 0), QSizeF(sourceSize)));
        if (rect.width() < 1.0 || rect.height() < 1.0) {
            continue;
        }

        Candidate candidate;
        candidate.classId = classId;
        candidate.confidence = bestScore;
        candidate.box = rect;
        candidates.append(candidate);
    }
    return nonMaximumSuppression(candidates);
}

bool writeResultJson(const QString &path, const QString &imagePath, const QVector<InferenceObject> &objects)
{
    QJsonArray results;
    for (const InferenceObject &object : objects) {
        QJsonArray bbox;
        bbox.append(object.x1);
        bbox.append(object.y1);
        bbox.append(object.x2);
        bbox.append(object.y2);

        QJsonObject item;
        item.insert("class_id", object.classId);
        item.insert("class_name", object.className);
        item.insert("confidence", object.confidence);
        item.insert("bbox", bbox);
        results.append(item);
    }

    QJsonObject root;
    root.insert("image", imagePath);
    root.insert("backend", "C++ ONNX Runtime");
    root.insert("results", results);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}
}
#endif

bool QuantizedOnnxRunner::isAvailable()
{
#ifdef CARDET_USE_ONNXRUNTIME
    return true;
#else
    return false;
#endif
}

QuantizedRunOutput QuantizedOnnxRunner::run(const QString &modelPath,
                                            const QString &imagePath,
                                            const QString &outputDir,
                                            double confidence)
{
    Q_UNUSED(confidence)

    QuantizedRunOutput output;
    if (!QFileInfo::exists(modelPath)) {
        output.message = "C++ ONNX Runtime inference failed: model file does not exist.";
        return output;
    }
    if (!QFileInfo::exists(imagePath)) {
        output.message = "C++ ONNX Runtime inference failed: image file does not exist.";
        return output;
    }
    QDir().mkpath(outputDir);

#ifdef CARDET_USE_ONNXRUNTIME
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "CarDet");
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        Ort::Session session(env, modelPath.toStdWString().c_str(), sessionOptions);

        Ort::AllocatorWithDefaultOptions allocator;
        auto inputName = session.GetInputNameAllocated(0, allocator);
        std::vector<Ort::AllocatedStringPtr> outputNameHolders;
        std::vector<const char *> outputNames;
        const size_t outputCount = session.GetOutputCount();
        for (size_t i = 0; i < outputCount; ++i) {
            outputNameHolders.emplace_back(session.GetOutputNameAllocated(i, allocator));
            outputNames.push_back(outputNameHolders.back().get());
        }
        if (outputNames.empty()) {
            output.message = "C++ ONNX Runtime inference failed: model has no output tensors.";
            return output;
        }

        auto inputInfo = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo();
        std::vector<int64_t> inputShape = inputInfo.GetShape();
        if (inputShape.size() != 4) {
            output.message = "C++ ONNX Runtime inference failed: only NCHW image input is supported.";
            return output;
        }
        if (inputShape[0] <= 0) {
            inputShape[0] = 1;
        }
        if (inputShape[1] <= 0) {
            inputShape[1] = 3;
        }
        if (inputShape[2] <= 0) {
            inputShape[2] = 640;
        }
        if (inputShape[3] <= 0) {
            inputShape[3] = 640;
        }
        const int inputHeight = static_cast<int>(inputShape[2]);
        const int inputWidth = static_cast<int>(inputShape[3]);

        QImage source(imagePath);
        if (source.isNull()) {
            output.message = "C++ ONNX Runtime inference failed: image cannot be loaded.";
            return output;
        }
        std::vector<float> inputTensorValues = imageToTensor(source, inputWidth, inputHeight);
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo,
                                                                 inputTensorValues.data(),
                                                                 inputTensorValues.size(),
                                                                 inputShape.data(),
                                                                 inputShape.size());

        const char *inputNames[] = {inputName.get()};
        auto ortOutputs = session.Run(Ort::RunOptions{nullptr},
                                      inputNames,
                                      &inputTensor,
                                      1,
                                      outputNames.data(),
                                      outputNames.size());
        if (ortOutputs.empty() || !ortOutputs.front().IsTensor()) {
            output.message = "C++ ONNX Runtime inference failed: output tensor is empty.";
            return output;
        }

        auto outputInfo = ortOutputs.front().GetTensorTypeAndShapeInfo();
        const std::vector<int64_t> outputShape = outputInfo.GetShape();
        const float *outputData = ortOutputs.front().GetTensorData<float>();
        const QVector<Candidate> detections = parseYoloOutput(outputData,
                                                              outputShape,
                                                              source.size(),
                                                              inputWidth,
                                                              inputHeight,
                                                              confidence);

        QImage annotated = source.convertToFormat(QImage::Format_RGB32);
        QPainter painter(&annotated);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor("#ffcc33"), 3));
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);
        for (const Candidate &detection : detections) {
            InferenceObject object;
            object.classId = detection.classId;
            object.className = QString("class_%1").arg(detection.classId);
            object.confidence = detection.confidence;
            object.x1 = detection.box.left();
            object.y1 = detection.box.top();
            object.x2 = detection.box.right();
            object.y2 = detection.box.bottom();
            output.objects.append(object);

            painter.drawRect(detection.box);
            const QString label = QString("%1 %2").arg(object.className).arg(object.confidence, 0, 'f', 2);
            painter.fillRect(QRectF(detection.box.topLeft() + QPointF(0, -22), QSizeF(110, 20)), QColor("#111820"));
            painter.setPen(Qt::white);
            painter.drawText(QRectF(detection.box.topLeft() + QPointF(4, -22), QSizeF(106, 20)),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             label);
            painter.setPen(QPen(QColor("#ffcc33"), 3));
        }
        painter.end();

        output.resultImagePath = QDir(outputDir).filePath("result_cpp_onnx.jpg");
        output.resultJsonPath = QDir(outputDir).filePath("result_cpp_onnx.json");
        if (!annotated.save(output.resultImagePath, "JPG")) {
            output.message = "C++ ONNX Runtime inference failed: result image cannot be saved.";
            return output;
        }
        if (!writeResultJson(output.resultJsonPath, imagePath, output.objects)) {
            output.message = "C++ ONNX Runtime inference failed: result json cannot be saved.";
            return output;
        }

        output.success = true;
        output.message = QString("C++ ONNX Runtime inference succeeded. Detected objects: %1.").arg(output.objects.size());
        return output;
    } catch (const std::exception &exc) {
        output.message = QString("C++ ONNX Runtime inference failed: %1").arg(exc.what());
        return output;
    }
#else
    output.message = "C++ ONNX Runtime inference is not enabled in this build. Define CARDET_USE_ONNXRUNTIME and configure ONNX Runtime include/lib paths in CarDet.pro; the GUI will fall back to Python inference.";
    return output;
#endif
}
