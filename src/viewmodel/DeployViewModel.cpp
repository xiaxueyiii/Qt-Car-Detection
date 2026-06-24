#include "DeployViewModel.h"

#include <QFileInfo>

DeployViewModel::DeployViewModel(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        handleChunk(m_process->readAllStandardOutput());
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit logMessage("部署进程错误：" + m_process->errorString());
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                if (!m_buffer.isEmpty()) {
                    handleLine(m_buffer);
                    m_buffer.clear();
                }
                emit finished(status == QProcess::NormalExit && exitCode == 0, exitCode, m_lastOutputPath);
            });
}

bool DeployViewModel::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString DeployViewModel::lastOutputPath() const
{
    return m_lastOutputPath;
}

void DeployViewModel::exportOnnx(const QString &pythonProgram, const QString &scriptPath, const QString &weights, const QString &output)
{
    startProcess(pythonProgram, scriptPath, {"--weights", weights, "--output", output});
}

void DeployViewModel::quantizeOnnx(const QString &pythonProgram, const QString &scriptPath, const QString &input, const QString &output)
{
    startProcess(pythonProgram, scriptPath, {"--input", input, "--output", output});
}

void DeployViewModel::stop()
{
    if (!isRunning()) {
        return;
    }
    m_process->terminate();
    if (!m_process->waitForFinished(2000)) {
        m_process->kill();
    }
}

void DeployViewModel::startProcess(const QString &pythonProgram, const QString &scriptPath, const QStringList &args)
{
    if (isRunning()) {
        emit logMessage("已有部署任务在运行。");
        return;
    }
    if (!QFileInfo::exists(scriptPath)) {
        emit logMessage("脚本不存在：" + scriptPath);
        emit finished(false, -1, QString());
        return;
    }

    m_lastOutputPath.clear();
    m_buffer.clear();
    QStringList fullArgs;
    fullArgs << "-u" << scriptPath;
    fullArgs << args;
    m_process->setProgram(pythonProgram.isEmpty() ? QStringLiteral("python") : pythonProgram);
    m_process->setArguments(fullArgs);
    emit logMessage("启动脚本：" + scriptPath);
    m_process->start();
    if (!m_process->waitForStarted(3000)) {
        emit logMessage("部署进程启动失败：" + m_process->errorString());
        emit finished(false, -1, QString());
    }
}

void DeployViewModel::handleChunk(QByteArray bytes)
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

void DeployViewModel::handleLine(const QString &line)
{
    if (line.startsWith("ONNX_MODEL:")) {
        m_lastOutputPath = line.mid(QString("ONNX_MODEL:").size()).trimmed();
    } else if (line.startsWith("QUANTIZED_MODEL:")) {
        m_lastOutputPath = line.mid(QString("QUANTIZED_MODEL:").size()).trimmed();
    }
    emit logMessage(line);
}
