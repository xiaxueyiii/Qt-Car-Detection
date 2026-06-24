#ifndef DATATYPES_H
#define DATATYPES_H

#include <QRectF>
#include <QString>
#include <QStringList>

struct DatasetStats
{
    QString datasetPath;
    int imageCount = 0;
    int labelCount = 0;
    int classCount = 0;
    QStringList classNames;
    bool isYoloDetection = false;
    QString formatSummary;
};

struct AnnotationBox
{
    int classId = 0;
    QString className;
    QRectF normalizedRect; // YOLO 标注使用 0-1 归一化坐标。
    double confidence = 0.0;
};

struct TrainParams
{
    QString datasetPath;
    QString trainPath;
    QString valPath;
    QString modelType;
    int epochs = 10;
    int batchSize = 4;
    int imageSize = 640;
    double learningRate = 0.001;
    QString device = "auto";
    QString outputDir;
};

struct InferenceObject
{
    int classId = 0;
    QString className;
    double confidence = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

#endif // DATATYPES_H
