#ifndef DEPLOYVIEWMODEL_H
#define DEPLOYVIEWMODEL_H

#include <QObject>
#include <QProcess>

class DeployViewModel : public QObject
{
    Q_OBJECT

public:
    explicit DeployViewModel(QObject *parent = nullptr);
    bool isRunning() const;
    QString lastOutputPath() const;

public slots:
    void exportOnnx(const QString &pythonProgram, const QString &scriptPath, const QString &weights, const QString &output);
    void quantizeOnnx(const QString &pythonProgram, const QString &scriptPath, const QString &input, const QString &output);
    void stop();

signals:
    void logMessage(const QString &message);
    void finished(bool success, int exitCode, const QString &outputPath);

private:
    void startProcess(const QString &pythonProgram, const QString &scriptPath, const QStringList &args);
    void handleChunk(QByteArray bytes);
    void handleLine(const QString &line);

private:
    QProcess *m_process = nullptr;
    QString m_buffer;
    QString m_lastOutputPath;
};

#endif // DEPLOYVIEWMODEL_H
