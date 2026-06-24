#include "InferenceViewModel.h"

#include <QFileInfo>

InferenceViewModel::InferenceViewModel(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        handleChunk(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit logMessage("推理进程错误：" + m_process->errorString());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                if (!m_buffer.isEmpty()) {
                    handleLine(m_buffer);
                    m_buffer.clear();
                }
                emit finished(status == QProcess::NormalExit && exitCode == 0,
                              exitCode,
                              m_resultImagePath,
                              m_resultJsonPath);
            });
}

bool InferenceViewModel::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString InferenceViewModel::resultImagePath() const
{
    return m_resultImagePath;
}

QString InferenceViewModel::resultJsonPath() const
{
    return m_resultJsonPath;
}

void InferenceViewModel::runInference(const QString &pythonProgram,
                                      const QString &scriptPath,
                                      const QString &weights,
                                      const QString &image,
                                      const QString &outputDir,
                                      const QString &device,
                                      double confidence)
{
    if (isRunning()) {
        emit logMessage("推理任务已在运行。");
        return;
    }
    if (!QFileInfo::exists(scriptPath)) {
        emit logMessage("推理脚本不存在：" + scriptPath);
        emit finished(false, -1, QString(), QString());
        return;
    }

    m_resultImagePath.clear();
    m_resultJsonPath.clear();
    m_buffer.clear();

    QStringList args;
    args << "-u" << scriptPath
         << "--weights" << weights
         << "--image" << image
         << "--output" << outputDir
         << "--device" << (device.trimmed().isEmpty() ? QStringLiteral("auto") : device.trimmed())
         << "--conf" << QString::number(confidence, 'g', 4);

    emit logMessage("启动推理脚本：" + scriptPath);
    m_process->setProgram(pythonProgram.isEmpty() ? QStringLiteral("python") : pythonProgram);
    m_process->setArguments(args);
    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        emit logMessage("推理进程启动失败：" + m_process->errorString());
        emit finished(false, -1, QString(), QString());
    }
}

void InferenceViewModel::stop()
{
    if (!isRunning()) {
        return;
    }
    m_process->terminate();
    if (!m_process->waitForFinished(2000)) {
        m_process->kill();
    }
}

void InferenceViewModel::handleChunk(QByteArray bytes)
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

void InferenceViewModel::handleLine(const QString &line)
{
    if (line.startsWith("RESULT_IMAGE:")) {
        m_resultImagePath = line.mid(QString("RESULT_IMAGE:").size()).trimmed();
    } else if (line.startsWith("RESULT_JSON:")) {
        m_resultJsonPath = line.mid(QString("RESULT_JSON:").size()).trimmed();
    }
    emit logMessage(line);
}
