#include "DeployPage.h"

#include "../model/DatabaseManager.h"
#include "../utils/AppConfig.h"
#include "../viewmodel/DeployViewModel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

DeployPage::DeployPage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database),
      m_viewModel(new DeployViewModel(this))
{
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(14);

    auto *formBox = new QGroupBox("模型部署/量化", this);
    auto *form = new QFormLayout(formBox);

    m_pythonEdit = new QLineEdit(AppConfig::pythonProgram(), formBox);
    auto *pythonButton = new QPushButton("选择", formBox);
    auto *pythonRow = new QWidget(formBox);
    auto *pythonLayout = new QHBoxLayout(pythonRow);
    pythonLayout->setContentsMargins(0, 0, 0, 0);
    pythonLayout->addWidget(m_pythonEdit, 1);
    pythonLayout->addWidget(pythonButton);

    m_weightsEdit = new QLineEdit(formBox);
    auto *weightsButton = new QPushButton("选择", formBox);
    auto *weightsRow = new QWidget(formBox);
    auto *weightsLayout = new QHBoxLayout(weightsRow);
    weightsLayout->setContentsMargins(0, 0, 0, 0);
    weightsLayout->addWidget(m_weightsEdit, 1);
    weightsLayout->addWidget(weightsButton);

    m_onnxOutputEdit = new QLineEdit(AppConfig::modelsDir() + "/best.onnx", formBox);
    auto *onnxButton = new QPushButton("选择", formBox);
    auto *onnxRow = new QWidget(formBox);
    auto *onnxLayout = new QHBoxLayout(onnxRow);
    onnxLayout->setContentsMargins(0, 0, 0, 0);
    onnxLayout->addWidget(m_onnxOutputEdit, 1);
    onnxLayout->addWidget(onnxButton);

    m_quantOutputEdit = new QLineEdit(AppConfig::modelsDir() + "/best_int8.onnx", formBox);
    auto *quantOutButton = new QPushButton("选择", formBox);
    auto *quantRow = new QWidget(formBox);
    auto *quantLayout = new QHBoxLayout(quantRow);
    quantLayout->setContentsMargins(0, 0, 0, 0);
    quantLayout->addWidget(m_quantOutputEdit, 1);
    quantLayout->addWidget(quantOutButton);

    m_exportButton = new QPushButton("导出 ONNX", formBox);
    m_quantButton = new QPushButton("动态量化 INT8", formBox);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_exportButton);
    buttonRow->addWidget(m_quantButton);
    buttonRow->addStretch();

    form->addRow("Python", pythonRow);
    form->addRow(".pt 模型", weightsRow);
    form->addRow("ONNX 输出", onnxRow);
    form->addRow("INT8 输出", quantRow);
    form->addRow(buttonRow);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    rootLayout->addWidget(formBox, 1);
    rootLayout->addWidget(m_logEdit, 2);

    connect(pythonButton, &QPushButton::clicked, this, &DeployPage::choosePython);
    connect(weightsButton, &QPushButton::clicked, this, &DeployPage::chooseWeights);
    connect(onnxButton, &QPushButton::clicked, this, &DeployPage::chooseOnnx);
    connect(quantOutButton, &QPushButton::clicked, this, &DeployPage::chooseQuantOutput);
    connect(m_exportButton, &QPushButton::clicked, this, &DeployPage::exportOnnx);
    connect(m_quantButton, &QPushButton::clicked, this, &DeployPage::quantizeOnnx);
    connect(m_viewModel, &DeployViewModel::logMessage, this, &DeployPage::appendLog);
    connect(m_viewModel, &DeployViewModel::finished, this, &DeployPage::onTaskFinished);
}

void DeployPage::choosePython()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择 Python 解释器", QString(), "Executable (*.exe *.bat *.cmd);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_pythonEdit->setText(file);
    }
}

void DeployPage::chooseWeights()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择 PyTorch .pt 权重文件", AppConfig::runsDir(), "PyTorch Weights (*.pt);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_weightsEdit->setText(file);
        QFileInfo info(file);
        m_onnxOutputEdit->setText(AppConfig::modelsDir() + "/" + info.completeBaseName() + ".onnx");
        m_quantOutputEdit->setText(AppConfig::modelsDir() + "/" + info.completeBaseName() + "_int8.onnx");
    }
}

void DeployPage::chooseOnnx()
{
    const QString file = QFileDialog::getSaveFileName(this, "选择 ONNX 输出", m_onnxOutputEdit->text(), "ONNX Files (*.onnx)");
    if (!file.isEmpty()) {
        m_onnxOutputEdit->setText(file);
    }
}

void DeployPage::chooseQuantOutput()
{
    const QString file = QFileDialog::getSaveFileName(this, "选择 INT8 输出", m_quantOutputEdit->text(), "ONNX Files (*.onnx)");
    if (!file.isEmpty()) {
        m_quantOutputEdit->setText(file);
    }
}

void DeployPage::exportOnnx()
{
    const QString weights = m_weightsEdit->text().trimmed();
    const QString output = m_onnxOutputEdit->text().trimmed();
    const QFileInfo weightsInfo(weights);
    const QFileInfo outputInfo(output);

    if (weights.isEmpty()) {
        QMessageBox::warning(this, "缺少 .pt 模型", "请先选择训练得到的 PyTorch 权重文件，例如 runs/train/.../weights/best.pt。");
        appendLog("导出 ONNX 已取消：未选择 .pt 权重文件。");
        return;
    }
    if (weightsInfo.suffix().compare("pt", Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this,
                             "模型格式不正确",
                             "导出 ONNX 必须选择 .pt 权重文件，不能选择 .onnx 或 int8.onnx。\n\n"
                             "正确流程：先选择 runs/train/.../weights/best.pt，导出 best.onnx；再用 best.onnx 做动态量化。");
        appendLog("导出 ONNX 已取消：.pt 模型输入不是 PyTorch 权重文件：" + weights);
        return;
    }
    if (output.isEmpty() || outputInfo.suffix().compare("onnx", Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, "ONNX 输出路径错误", "ONNX 输出必须是 .onnx 文件。");
        appendLog("导出 ONNX 已取消：ONNX 输出路径无效。");
        return;
    }

    m_currentTask = "export";
    m_exportButton->setEnabled(false);
    m_quantButton->setEnabled(false);
    m_viewModel->exportOnnx(m_pythonEdit->text().trimmed(),
                            AppConfig::scriptsDir() + "/export_onnx.py",
                            weights,
                            output);
}

void DeployPage::quantizeOnnx()
{
    const QString input = m_onnxOutputEdit->text().trimmed();
    const QString output = m_quantOutputEdit->text().trimmed();
    const QFileInfo inputInfo(input);
    const QFileInfo outputInfo(output);

    if (input.isEmpty() || inputInfo.suffix().compare("onnx", Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, "ONNX 输入路径错误", "动态量化 INT8 的输入必须是已经导出的 .onnx 模型。");
        appendLog("动态量化已取消：ONNX 输入路径无效。");
        return;
    }
    if (!inputInfo.exists()) {
        QMessageBox::warning(this, "ONNX 文件不存在", "请先导出 ONNX，或者选择一个已经存在的 .onnx 文件。");
        appendLog("动态量化已取消：ONNX 文件不存在：" + input);
        return;
    }
    if (output.isEmpty() || outputInfo.suffix().compare("onnx", Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, "INT8 输出路径错误", "INT8 输出必须是另一个 .onnx 文件，例如 best_int8.onnx。");
        appendLog("动态量化已取消：INT8 输出路径无效。");
        return;
    }
    if (QFileInfo(input).absoluteFilePath().compare(QFileInfo(output).absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        QMessageBox::warning(this, "输入输出不能相同", "动态量化会生成新的 INT8 ONNX 文件，输入 ONNX 和 INT8 输出不能是同一个路径。");
        appendLog("动态量化已取消：输入 ONNX 和 INT8 输出路径相同。");
        return;
    }

    m_currentTask = "quantize";
    m_exportButton->setEnabled(false);
    m_quantButton->setEnabled(false);
    m_viewModel->quantizeOnnx(m_pythonEdit->text().trimmed(),
                              AppConfig::scriptsDir() + "/quantize.py",
                              input,
                              output);
}

void DeployPage::onTaskFinished(bool success, int exitCode, const QString &outputPath)
{
    m_exportButton->setEnabled(true);
    m_quantButton->setEnabled(true);
    appendLog(QString("任务结束：%1，exitCode=%2").arg(success ? "成功" : "失败").arg(exitCode));
    if (success && m_database) {
        if (m_currentTask == "export") {
            m_database->insertModelRecord(m_weightsEdit->text().trimmed(), "ONNX", outputPath, QString());
            if (!outputPath.isEmpty()) {
                m_onnxOutputEdit->setText(outputPath);
            }
        } else if (m_currentTask == "quantize") {
            m_database->insertModelRecord(m_onnxOutputEdit->text().trimmed(), "ONNX", outputPath, "INT8 dynamic");
            if (!outputPath.isEmpty()) {
                m_quantOutputEdit->setText(outputPath);
            }
        }
    }
}

void DeployPage::appendLog(const QString &message)
{
    m_logEdit->appendPlainText(message);
}
