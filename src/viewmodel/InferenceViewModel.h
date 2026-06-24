#ifndef INFERENCEVIEWMODEL_H
#define INFERENCEVIEWMODEL_H

#include <QObject>
#include <QProcess>

class InferenceViewModel : public QObject
{
    Q_OBJECT

public:
    explicit InferenceViewModel(QObject *parent = nullptr);
    bool isRunning() const;
    QString resultImagePath() const;
    QString resultJsonPath() const;

public slots:
    void runInference(const QString &pythonProgram,
                      const QString &scriptPath,
                      const QString &weights,
                      const QString &image,
                      const QString &outputDir,
                      const QString &device,
                      double confidence);
    void stop();

signals:
    void logMessage(const QString &message);
    void finished(bool success, int exitCode, const QString &resultImagePath, const QString &resultJsonPath);

private:
    void handleChunk(QByteArray bytes);
    void handleLine(const QString &line);

private:
    QProcess *m_process = nullptr;
    QString m_buffer;
    QString m_resultImagePath;
    QString m_resultJsonPath;
};

#endif // INFERENCEVIEWMODEL_H
