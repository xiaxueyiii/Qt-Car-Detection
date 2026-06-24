#include "TrainingViewModel.h"

#include <QFileInfo>

TrainingViewModel::TrainingViewModel(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        handleChunk(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit logMessage("训练进程错误：" + m_process->errorString());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                if (!m_buffer.isEmpty()) {
                    handleLine(m_buffer);
                    m_buffer.clear();
                }
                const bool ok = (status == QProcess::NormalExit && exitCode == 0);
                emit finished(ok, exitCode, m_lastModelPath, m_lastMetricsJson);
            });
}

bool TrainingViewModel::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString TrainingViewModel::lastModelPath() const
{
    return m_lastModelPath;
}

QString TrainingViewModel::lastMetricsJson() const
{
    return m_lastMetricsJson;
}

void TrainingViewModel::startTraining(const QString &pythonProgram,
                                      const QString &scriptPath,
                                      const TrainParams &params,
                                      const QString &projectDir)
{
    if (isRunning()) {
        emit logMessage("训练任务已在运行。");
        return;
    }
    if (!QFileInfo::exists(scriptPath)) {
        emit logMessage("训练脚本不存在：" + scriptPath);
        emit finished(false, -1, QString(), QString());
        return;
    }

    m_lastModelPath.clear();
    m_lastMetricsJson.clear();
    m_buffer.clear();

    QStringList args;
    args << "-u" << scriptPath
         << "--data" << params.datasetPath
         << "--epochs" << QString::number(params.epochs)
         << "--batch" << QString::number(params.batchSize)
         << "--imgsz" << QString::number(params.imageSize)
         << "--model" << params.modelType
         << "--lr" << QString::number(params.learningRate, 'g', 8)
         << "--device" << (params.device.trimmed().isEmpty() ? QStringLiteral("auto") : params.device.trimmed())
         << "--project" << projectDir;
    if (!params.trainPath.trimmed().isEmpty()) {
        args << "--train" << params.trainPath.trimmed();
    }
    if (!params.valPath.trimmed().isEmpty()) {
        args << "--val" << params.valPath.trimmed();
    }

    emit logMessage("启动训练脚本：" + scriptPath);
    m_process->setProgram(pythonProgram.isEmpty() ? QStringLiteral("python") : pythonProgram);
    m_process->setArguments(args);
    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        emit logMessage("训练进程启动失败：" + m_process->errorString());
        emit finished(false, -1, QString(), QString());
    }
}

void TrainingViewModel::stop()
{
    if (!isRunning()) {
        return;
    }
    emit logMessage("正在停止训练进程。");
    m_process->terminate();
    if (!m_process->waitForFinished(2000)) {
        m_process->kill();
    }
}

void TrainingViewModel::handleChunk(QByteArray bytes)
{
    m_buffer += QString::fromUtf8(bytes);
    while (true) {
        int pos = m_buffer.indexOf('\n');
        if (pos < 0) {
            break;
        }
        QString line = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        handleLine(line);
    }
}

void TrainingViewModel::handleLine(const QString &line)
{
    if (line.startsWith("BEST_MODEL:")) {
        m_lastModelPath = line.mid(QString("BEST_MODEL:").size()).trimmed();
    } else if (line.startsWith("FINAL_RESULT:")) {
        m_lastMetricsJson = line.mid(QString("FINAL_RESULT:").size()).trimmed();
    }
    emit logMessage(line);
}
