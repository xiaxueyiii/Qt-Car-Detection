#ifndef QUANTIZEDONNXRUNNER_H
#define QUANTIZEDONNXRUNNER_H

#include "DataTypes.h"

#include <QString>
#include <QVector>

struct QuantizedRunOutput
{
    bool success = false;
    QString message;
    QString resultImagePath;
    QString resultJsonPath;
    QVector<InferenceObject> objects;
};

class QuantizedOnnxRunner
{
public:
    static bool isAvailable();
    static QuantizedRunOutput run(const QString &modelPath,
                                  const QString &imagePath,
                                  const QString &outputDir,
                                  double confidence);
};

#endif // QUANTIZEDONNXRUNNER_H
