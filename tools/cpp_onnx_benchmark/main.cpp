#include "../../src/model/QuantizedOnnxRunner.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace {
QString valueAfter(const QStringList &args, const QString &name, const QString &fallback = QString())
{
    const int index = args.indexOf(name);
    if (index >= 0 && index + 1 < args.size()) {
        return args.at(index + 1);
    }
    return fallback;
}

int intAfter(const QStringList &args, const QString &name, int fallback)
{
    bool ok = false;
    const int value = valueAfter(args, name).toInt(&ok);
    return ok ? value : fallback;
}

double doubleAfter(const QStringList &args, const QString &name, double fallback)
{
    bool ok = false;
    const double value = valueAfter(args, name).toDouble(&ok);
    return ok ? value : fallback;
}

void printUsage()
{
    QTextStream out(stdout);
    out << "Usage: cpp_onnx_benchmark --model best_int8.onnx --image test.jpg "
           "--output runs/perf_benchmark/cpp --runs 10 --conf 0.25\n";
}
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.contains("--help") || !args.contains("--model") || !args.contains("--image")) {
        printUsage();
        return args.contains("--help") ? 0 : 2;
    }

    const QString modelPath = valueAfter(args, "--model");
    const QString imagePath = valueAfter(args, "--image");
    const QString outputDir = valueAfter(args, "--output", QDir::current().filePath("cpp_onnx_benchmark_output"));
    const int runs = qMax(1, intAfter(args, "--runs", 10));
    const double conf = doubleAfter(args, "--conf", 0.25);

    QJsonObject root;
    root.insert("backend", "C++ ONNX Runtime");
    root.insert("model", QDir::fromNativeSeparators(QFileInfo(modelPath).absoluteFilePath()));
    root.insert("image", QDir::fromNativeSeparators(QFileInfo(imagePath).absoluteFilePath()));
    root.insert("output", QDir::fromNativeSeparators(QFileInfo(outputDir).absoluteFilePath()));
    root.insert("runs", runs);
    root.insert("conf", conf);
    root.insert("available", QuantizedOnnxRunner::isAvailable());

    QJsonArray values;
    QJsonArray messages;

    if (!QuantizedOnnxRunner::isAvailable()) {
        root.insert("success", false);
        root.insert("message", "CARDET_USE_ONNXRUNTIME is not enabled. Configure ONNXRUNTIME_DIR and rebuild this benchmark.");
        QTextStream(stdout) << QJsonDocument(root).toJson(QJsonDocument::Indented);
        return 1;
    }

    bool allSuccess = true;
    for (int i = 0; i < runs; ++i) {
        const QString runDir = QDir(outputDir).filePath(QString("run_%1").arg(i, 3, 10, QChar('0')));
        QDir().mkpath(runDir);
        QElapsedTimer timer;
        timer.start();
        const QuantizedRunOutput output = QuantizedOnnxRunner::run(modelPath, imagePath, runDir, conf);
        const double elapsedMs = timer.nsecsElapsed() / 1000000.0;
        if (output.success) {
            values.append(elapsedMs);
        } else {
            allSuccess = false;
        }
        QJsonObject message;
        message.insert("index", i);
        message.insert("success", output.success);
        message.insert("elapsed_ms", elapsedMs);
        message.insert("message", output.message);
        message.insert("detected_count", output.objects.size());
        message.insert("result_image", output.resultImagePath);
        message.insert("result_json", output.resultJsonPath);
        messages.append(message);
        if (!output.success) {
            break;
        }
    }

    root.insert("success", allSuccess && !values.isEmpty());
    root.insert("values_ms", values);
    root.insert("details", messages);

    double sum = 0.0;
    for (const QJsonValue &value : values) {
        sum += value.toDouble();
    }
    if (!values.isEmpty()) {
        root.insert("avg_ms", sum / values.size());
    }

    QTextStream(stdout) << QJsonDocument(root).toJson(QJsonDocument::Indented);
    return root.value("success").toBool() ? 0 : 1;
}
