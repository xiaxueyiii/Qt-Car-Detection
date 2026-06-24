#ifndef TRAININGVIEWMODEL_H
#define TRAININGVIEWMODEL_H

#include "../model/DataTypes.h"

#include <QObject>
#include <QProcess>

class TrainingViewModel : public QObject
{
    Q_OBJECT

public:
    explicit TrainingViewModel(QObject *parent = nullptr);
    bool isRunning() const;
    QString lastModelPath() const;
    QString lastMetricsJson() const;

public slots:
    void startTraining(const QString &pythonProgram,
                       const QString &scriptPath,
                       const TrainParams &params,
                       const QString &projectDir);
    void stop();

signals:
    void logMessage(const QString &message);
    void finished(bool success, int exitCode, const QString &modelPath, const QString &metricsJson);

private:
    void handleChunk(QByteArray bytes);
    void handleLine(const QString &line);

private:
    QProcess *m_process = nullptr;
    QString m_buffer;
    QString m_lastModelPath;
    QString m_lastMetricsJson;
};

#endif // TRAININGVIEWMODEL_H
