#include "TrainingPage.h"

#include "../model/DatabaseManager.h"
#include "../utils/AppConfig.h"
#include "../viewmodel/TrainingViewModel.h"

#include <QAbstractSpinBox>
#include <QAbstractItemView>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QVBoxLayout>

namespace {
QWidget *pathRow(QWidget *parent, QLineEdit *edit, QPushButton *button)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit, 1);
    layout->addWidget(button);
    return row;
}

void prepareStepper(QAbstractSpinBox *spinBox)
{
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setAccelerated(true);
    spinBox->setMinimumHeight(34);
    spinBox->setMinimumWidth(130);
}

QWidget *stepperRow(QWidget *parent, QAbstractSpinBox *spinBox)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *minusButton = new QPushButton("-", row);
    auto *plusButton = new QPushButton("+", row);
    minusButton->setFixedSize(32, 32);
    plusButton->setFixedSize(32, 32);
    minusButton->setToolTip("减少");
    plusButton->setToolTip("增加");
    minusButton->setFocusPolicy(Qt::NoFocus);
    plusButton->setFocusPolicy(Qt::NoFocus);

    QObject::connect(minusButton, &QPushButton::clicked, spinBox, &QAbstractSpinBox::stepDown);
    QObject::connect(plusButton, &QPushButton::clicked, spinBox, &QAbstractSpinBox::stepUp);

    layout->addWidget(spinBox, 1);
    layout->addWidget(minusButton);
    layout->addWidget(plusButton);
    return row;
}
}

TrainingPage::TrainingPage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database),
      m_viewModel(new TrainingViewModel(this))
{
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(14);

    auto *paramBox = new QGroupBox("训练参数", this);
    auto *form = new QFormLayout(paramBox);

    const QString datasetRoot = AppConfig::defaultDatasetPath();
    m_datasetEdit = new QLineEdit(datasetRoot, paramBox);
    auto *datasetButton = new QPushButton("选择", paramBox);
    m_trainEdit = new QLineEdit(datasetRoot + "/train/images", paramBox);
    auto *trainButton = new QPushButton("选择", paramBox);
    m_valEdit = new QLineEdit(datasetRoot + "/valid/images", paramBox);
    auto *valButton = new QPushButton("选择", paramBox);

    m_modelEdit = new QLineEdit("yolov8n.pt", paramBox);
    m_epochsSpin = new QSpinBox(paramBox);
    m_epochsSpin->setRange(1, 10000);
    m_epochsSpin->setValue(10);
    prepareStepper(m_epochsSpin);
    m_batchSpin = new QSpinBox(paramBox);
    m_batchSpin->setRange(1, 512);
    m_batchSpin->setValue(4);
    prepareStepper(m_batchSpin);
    m_imgszSpin = new QSpinBox(paramBox);
    m_imgszSpin->setRange(64, 4096);
    m_imgszSpin->setValue(640);
    prepareStepper(m_imgszSpin);
    m_lrSpin = new QDoubleSpinBox(paramBox);
    m_lrSpin->setRange(0.000001, 1.0);
    m_lrSpin->setDecimals(6);
    m_lrSpin->setSingleStep(0.0005);
    m_lrSpin->setValue(0.001);
    prepareStepper(m_lrSpin);
    m_deviceEdit = new QLineEdit("0", paramBox);
    m_outputEdit = new QLineEdit(AppConfig::runsDir() + "/train", paramBox);
    auto *outputButton = new QPushButton("选择", paramBox);

    m_pythonEdit = new QLineEdit(AppConfig::pythonProgram(), paramBox);
    auto *pythonButton = new QPushButton("选择", paramBox);

    m_scriptEdit = new QLineEdit(AppConfig::scriptsDir() + "/train.py", paramBox);
    auto *scriptButton = new QPushButton("选择", paramBox);

    form->addRow("数据集根目录", pathRow(paramBox, m_datasetEdit, datasetButton));
    form->addRow("训练集 images", pathRow(paramBox, m_trainEdit, trainButton));
    form->addRow("验证集 images", pathRow(paramBox, m_valEdit, valButton));
    form->addRow("模型类型/权重", m_modelEdit);
    form->addRow("epochs", stepperRow(paramBox, m_epochsSpin));
    form->addRow("batch size", stepperRow(paramBox, m_batchSpin));
    form->addRow("image size", stepperRow(paramBox, m_imgszSpin));
    form->addRow("learning rate", stepperRow(paramBox, m_lrSpin));
    form->addRow("device", m_deviceEdit);
    form->addRow("输出目录", pathRow(paramBox, m_outputEdit, outputButton));
    form->addRow("Python", pathRow(paramBox, m_pythonEdit, pythonButton));
    form->addRow("训练脚本", pathRow(paramBox, m_scriptEdit, scriptButton));

    auto *buttonRow = new QHBoxLayout;
    m_startButton = new QPushButton("开始训练", paramBox);
    m_stopButton = new QPushButton("停止", paramBox);
    m_stopButton->setEnabled(false);
    buttonRow->addWidget(m_startButton);
    buttonRow->addWidget(m_stopButton);
    buttonRow->addStretch();
    form->addRow(buttonRow);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_metricsTable = new QTableWidget(0, 2, rightPanel);
    m_metricsTable->setHorizontalHeaderLabels({"指标", "值"});
    m_metricsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_metricsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_metricsTable->verticalHeader()->setVisible(false);
    m_metricsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_metricChartLabel = new QLabel(rightPanel);
    m_metricChartLabel->setWordWrap(true);
    m_metricChartLabel->setMinimumHeight(120);
    m_metricChartLabel->setTextFormat(Qt::RichText);
    m_metricChartLabel->setStyleSheet("QLabel{background:#ffffff;border:1px solid #cfd7e3;border-radius:5px;padding:8px;}");
    m_metricChartLabel->setText("训练完成后显示 mAP / precision / recall 指标条形图。");

    m_logEdit = new QPlainTextEdit(rightPanel);
    m_logEdit->setReadOnly(true);
    m_logEdit->setPlaceholderText("训练日志会实时显示在这里。");

    rightLayout->addWidget(new QLabel("训练结果指标", rightPanel));
    rightLayout->addWidget(m_metricsTable, 1);
    rightLayout->addWidget(new QLabel("指标可视化", rightPanel));
    rightLayout->addWidget(m_metricChartLabel);
    rightLayout->addWidget(new QLabel("训练日志", rightPanel));
    rightLayout->addWidget(m_logEdit, 3);

    rootLayout->addWidget(paramBox, 1);
    rootLayout->addWidget(rightPanel, 2);

    connect(datasetButton, &QPushButton::clicked, this, &TrainingPage::chooseDataset);
    connect(trainButton, &QPushButton::clicked, this, &TrainingPage::chooseTrainPath);
    connect(valButton, &QPushButton::clicked, this, &TrainingPage::chooseValPath);
    connect(outputButton, &QPushButton::clicked, this, &TrainingPage::chooseOutputDir);
    connect(pythonButton, &QPushButton::clicked, this, &TrainingPage::choosePython);
    connect(scriptButton, &QPushButton::clicked, this, &TrainingPage::chooseScript);
    connect(m_startButton, &QPushButton::clicked, this, &TrainingPage::startTraining);
    connect(m_stopButton, &QPushButton::clicked, m_viewModel, &TrainingViewModel::stop);
    connect(m_viewModel, &TrainingViewModel::logMessage, this, &TrainingPage::appendLog);
    connect(m_viewModel, &TrainingViewModel::finished, this, &TrainingPage::onTrainingFinished);
}

void TrainingPage::chooseDataset()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择训练数据集", m_datasetEdit->text());
    if (!dir.isEmpty()) {
        m_datasetEdit->setText(dir);
        m_trainEdit->setText(dir + "/train/images");
        const QString valid = QFileInfo::exists(dir + "/valid/images") ? dir + "/valid/images" : dir + "/val/images";
        m_valEdit->setText(valid);
    }
}

void TrainingPage::chooseTrainPath()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择训练集 images 目录", m_trainEdit->text());
    if (!dir.isEmpty()) {
        m_trainEdit->setText(dir);
    }
}

void TrainingPage::chooseValPath()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择验证集 images 目录", m_valEdit->text());
    if (!dir.isEmpty()) {
        m_valEdit->setText(dir);
    }
}

void TrainingPage::chooseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择训练输出目录", m_outputEdit->text());
    if (!dir.isEmpty()) {
        m_outputEdit->setText(dir);
    }
}

void TrainingPage::choosePython()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择 Python 解释器", QString(), "Executable (*.exe *.bat *.cmd);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_pythonEdit->setText(file);
    }
}

void TrainingPage::chooseScript()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择训练脚本", AppConfig::scriptsDir(), "Python Files (*.py)");
    if (!file.isEmpty()) {
        m_scriptEdit->setText(file);
    }
}

void TrainingPage::startTraining()
{
    if (m_viewModel->isRunning()) {
        appendLog("训练任务已经在运行。");
        return;
    }

    m_logEdit->clear();
    m_metricsTable->setRowCount(0);
    m_metricChartLabel->setText("训练进行中，完成后显示指标条形图。");
    m_currentParams.datasetPath = m_datasetEdit->text().trimmed();
    m_currentParams.trainPath = m_trainEdit->text().trimmed();
    m_currentParams.valPath = m_valEdit->text().trimmed();
    m_currentParams.modelType = m_modelEdit->text().trimmed();
    m_currentParams.epochs = m_epochsSpin->value();
    m_currentParams.batchSize = m_batchSpin->value();
    m_currentParams.imageSize = m_imgszSpin->value();
    m_currentParams.learningRate = m_lrSpin->value();
    m_currentParams.device = m_deviceEdit->text().trimmed();
    m_currentParams.outputDir = m_outputEdit->text().trimmed();

    if (m_database) {
        m_recordId = m_database->insertTrainRecord(m_currentParams, "运行中");
        m_trainingRunId = m_database->insertTrainingRun(m_currentParams, "运行中");
    }

    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_viewModel->startTraining(m_pythonEdit->text().trimmed(),
                               m_scriptEdit->text().trimmed(),
                               m_currentParams,
                               m_currentParams.outputDir.isEmpty() ? AppConfig::runsDir() + "/train" : m_currentParams.outputDir);
}

void TrainingPage::onTrainingFinished(bool success, int exitCode, const QString &modelPath, const QString &metricsJson)
{
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    const QString status = success ? "完成" : QString("失败 exitCode=%1").arg(exitCode);
    appendLog("训练任务结束：" + status);
    if (!modelPath.isEmpty()) {
        appendLog("模型路径：" + modelPath);
    }
    showMetrics(metricsJson);
    if (m_database && m_recordId > 0) {
        m_database->updateTrainRecord(m_recordId, status, modelPath);
    }
    if (m_database && m_trainingRunId > 0) {
        m_database->finishTrainingRun(m_trainingRunId, status, modelPath, metricsJson);
    }
}

void TrainingPage::appendLog(const QString &message)
{
    m_logEdit->appendPlainText(message);
}

void TrainingPage::showMetrics(const QString &metricsJson)
{
    m_metricsTable->setRowCount(0);
    QList<QPair<QString, double>> metricBars;
    auto addRow = [this](const QString &name, const QString &value) {
        const int row = m_metricsTable->rowCount();
        m_metricsTable->insertRow(row);
        m_metricsTable->setItem(row, 0, new QTableWidgetItem(name));
        m_metricsTable->setItem(row, 1, new QTableWidgetItem(value));
    };
    auto maybeAddMetricBar = [&metricBars](const QString &name, const QJsonValue &value) {
        const QString lower = name.toLower();
        if (!lower.contains("map") && !lower.contains("precision") && !lower.contains("recall")) {
            return;
        }
        bool ok = false;
        const double number = value.toVariant().toDouble(&ok);
        if (ok && number >= 0.0 && number <= 1.0) {
            metricBars.append({name, number});
        }
    };

    if (metricsJson.trimmed().isEmpty()) {
        addRow("metrics_json", "训练脚本未返回结构化指标。");
        showMetricBars(metricBars);
        return;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(metricsJson.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        addRow("raw", metricsJson);
        showMetricBars(metricBars);
        return;
    }

    const QJsonObject object = doc.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.value().isObject()) {
            const QJsonObject nested = it.value().toObject();
            for (auto nestedIt = nested.constBegin(); nestedIt != nested.constEnd(); ++nestedIt) {
                const QString name = it.key() + "." + nestedIt.key();
                addRow(name, nestedIt.value().toVariant().toString());
                maybeAddMetricBar(name, nestedIt.value());
            }
        } else {
            addRow(it.key(), it.value().toVariant().toString());
            maybeAddMetricBar(it.key(), it.value());
        }
    }
    showMetricBars(metricBars);
}

void TrainingPage::showMetricBars(const QList<QPair<QString, double>> &bars)
{
    if (bars.isEmpty()) {
        m_metricChartLabel->setText("暂无可视化指标。训练脚本返回 mAP / precision / recall 后会显示条形图。");
        return;
    }

    QString html;
    html += "<div style='font-family:Microsoft YaHei;font-size:13px;'>";
    const int maxBars = qMin(8, bars.size());
    for (int i = 0; i < maxBars; ++i) {
        const QString name = bars.at(i).first.toHtmlEscaped();
        const double value = qBound(0.0, bars.at(i).second, 1.0);
        const int percent = qRound(value * 100.0);
        html += QString("<div style='margin-bottom:6px;'>"
                        "<div>%1&nbsp;&nbsp;%2%</div>"
                        "<div style='height:10px;background:#e8eef7;border-radius:5px;'>"
                        "<div style='height:10px;width:%2%;background:#1f6feb;border-radius:5px;'></div>"
                        "</div></div>")
                    .arg(name)
                    .arg(percent);
    }
    html += "</div>";
    m_metricChartLabel->setText(html);
}
