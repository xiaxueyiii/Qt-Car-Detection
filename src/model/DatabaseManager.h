#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "DataTypes.h"

#include <QMap>
#include <QObject>
#include <QSize>
#include <QSqlDatabase>
#include <QStringList>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool open(const QString &databasePath);
    bool initialize();
    QString lastError() const;

    bool insertDatasetStats(const DatasetStats &stats);
    bool saveDatasetCatalog(const DatasetStats &stats);
    bool saveCategories(const QString &datasetPath, const QStringList &classNames);
    bool replaceImageAnnotations(const QString &datasetPath,
                                 const QString &imagePath,
                                 const QString &labelPath,
                                 const QVector<AnnotationBox> &boxes,
                                 const QStringList &classNames);
    int insertTrainRecord(const TrainParams &params, const QString &status, const QString &modelPath = QString());
    bool updateTrainRecord(int id, const QString &status, const QString &modelPath);
    int insertTrainingRun(const TrainParams &params, const QString &status);
    bool finishTrainingRun(int id,
                           const QString &status,
                           const QString &modelPath,
                           const QString &metricsJson);
    bool insertModelRecord(const QString &modelPath,
                           const QString &modelType,
                           const QString &quantizedPath,
                           const QString &quantizationType);
    bool insertModelFile(const QString &sourceModelPath,
                         const QString &modelPath,
                         const QString &modelType,
                         const QString &quantizationType,
                         const QString &backend);
    int insertInferenceRecord(const QString &imagePath,
                              const QString &modelPath,
                              const QString &resultImagePath,
                              const QString &resultJsonPath,
                              int detectedCount);
    bool insertInferenceObject(int inferenceId, const InferenceObject &object);

    QList<QStringList> queryRows(const QString &tableName, const QStringList &columns, int limit = 200) const;
    int totalInferenceCount() const;
    QMap<QString, int> classDetectionCounts() const;
    QString latestTrainingModel() const;
    QString latestInferenceTime() const;

private:
    bool execSql(const QString &sql);
    int ensureDatasetRecord(const QString &datasetPath, const QString &name = QString());
    QString now() const;

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    QString m_lastError;
};

#endif // DATABASEMANAGER_H
