#include "InferencePage.h"

#include "../model/DatabaseManager.h"
#include "../model/QuantizedOnnxRunner.h"
#include "../utils/AppConfig.h"
#include "../viewmodel/InferenceViewModel.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>

InferencePage::InferencePage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database),
      m_viewModel(new InferenceViewModel(this))
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(12);

    auto *controlBox = new QGroupBox("图像推理", this);
    auto *form = new QFormLayout(controlBox);

    m_imageEdit = new QLineEdit(findDefaultImage(), controlBox);
    auto *imageButton = new QPushButton("选择", controlBox);
    auto *imageRow = new QWidget(controlBox);
    auto *imageLayout = new QHBoxLayout(imageRow);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->addWidget(m_imageEdit, 1);
    imageLayout->addWidget(imageButton);

    m_modelEdit = new QLineEdit(findDefaultModel(), controlBox);
    auto *modelButton = new QPushButton("选择", controlBox);
    auto *modelRow = new QWidget(controlBox);
    auto *modelLayout = new QHBoxLayout(modelRow);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->addWidget(m_modelEdit, 1);
    modelLayout->addWidget(modelButton);

    m_outputEdit = new QLineEdit(AppConfig::runsDir() + "/infer", controlBox);
    auto *outputButton = new QPushButton("选择", controlBox);
    auto *outputRow = new QWidget(controlBox);
    auto *outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputLayout->addWidget(m_outputEdit, 1);
    outputLayout->addWidget(outputButton);

    m_confSpin = new QDoubleSpinBox(controlBox);
    m_confSpin->setRange(0.01, 0.99);
    m_confSpin->setSingleStep(0.05);
    m_confSpin->setValue(0.25);
    m_deviceEdit = new QLineEdit("0", controlBox);
    m_deviceEdit->setPlaceholderText("0 / cpu / auto");
    m_cppOnnxCheck = new QCheckBox("ONNX 量化模型优先使用 C++ Runtime", controlBox);
    if (!QuantizedOnnxRunner::isAvailable()) {
        m_cppOnnxCheck->setChecked(false);
        m_cppOnnxCheck->setEnabled(false);
        m_cppOnnxCheck->setText("C++ ONNX Runtime 未启用，ONNX 使用 Python Runtime");
        m_cppOnnxCheck->setToolTip("当前程序没有链接 ONNX Runtime C++ 开发库；"
                                   "选择 .onnx 模型时会使用 Python ONNX Runtime 执行推理。");
    }

    auto *buttonRow = new QHBoxLayout;
    m_startButton = new QPushButton("开始识别/检测", controlBox);
    m_saveButton = new QPushButton("保存结果图", controlBox);
    m_saveButton->setEnabled(false);
    buttonRow->addWidget(m_startButton);
    buttonRow->addWidget(m_saveButton);
    buttonRow->addStretch();

    form->addRow("图片", imageRow);
    form->addRow("模型", modelRow);
    form->addRow("输出目录", outputRow);
    form->addRow("conf", m_confSpin);
    form->addRow("device", m_deviceEdit);
    form->addRow("量化推理", m_cppOnnxCheck);
    form->addRow(buttonRow);
    rootLayout->addWidget(controlBox);

    auto *splitter = new QSplitter(this);
    auto *imagePanel = new QWidget(splitter);
    auto *imagePanelLayout = new QHBoxLayout(imagePanel);
    m_originalLabel = new QLabel("原图", imagePanel);
    m_resultLabel = new QLabel("结果图", imagePanel);
    for (QLabel *label : {m_originalLabel, m_resultLabel}) {
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(360, 260);
        label->setStyleSheet("QLabel{background:#222832;color:#e9edf3;border:1px solid #3a4250;}");
        imagePanelLayout->addWidget(label, 1);
    }

    auto *rightPanel = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    m_resultTable = new QTableWidget(0, 4, rightPanel);
    m_resultTable->setHorizontalHeaderLabels({"类别", "置信度", "bbox", "class id"});
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logEdit = new QPlainTextEdit(rightPanel);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(180);
    rightLayout->addWidget(m_resultTable, 1);
    rightLayout->addWidget(m_logEdit);

    splitter->addWidget(imagePanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

    connect(imageButton, &QPushButton::clicked, this, &InferencePage::chooseImage);
    connect(modelButton, &QPushButton::clicked, this, &InferencePage::chooseModel);
    connect(outputButton, &QPushButton::clicked, this, &InferencePage::chooseOutputDir);
    connect(m_startButton, &QPushButton::clicked, this, &InferencePage::startInference);
    connect(m_saveButton, &QPushButton::clicked, this, &InferencePage::saveResultImage);
    connect(m_viewModel, &InferenceViewModel::logMessage, this, &InferencePage::appendLog);
    connect(m_viewModel, &InferenceViewModel::finished, this, &InferencePage::onInferenceFinished);

    if (!m_imageEdit->text().isEmpty()) {
        showImage(m_originalLabel, m_imageEdit->text());
    }
}

void InferencePage::chooseImage()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择图片", AppConfig::defaultDatasetPath(), "Images (*.jpg *.jpeg *.png *.bmp *.webp)");
    if (!file.isEmpty()) {
        m_imageEdit->setText(file);
        showImage(m_originalLabel, file);
    }
}

void InferencePage::chooseModel()
{
    const QString file = QFileDialog::getOpenFileName(this, "选择模型", AppConfig::modelsDir(), "Model Files (*.pt *.onnx);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_modelEdit->setText(file);
    }
}

void InferencePage::chooseOutputDir()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择输出目录", m_outputEdit->text());
    if (!dir.isEmpty()) {
        m_outputEdit->setText(dir);
    }
}

void InferencePage::startInference()
{
    if (m_viewModel->isRunning()) {
        appendLog("推理任务已经在运行。");
        return;
    }
    m_logEdit->clear();
    m_resultTable->setRowCount(0);
    m_saveButton->setEnabled(false);
    showImage(m_originalLabel, m_imageEdit->text().trimmed());
    m_startButton->setEnabled(false);
    const QString runDir = m_outputEdit->text().trimmed() + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString modelPath = m_modelEdit->text().trimmed();
    const bool usingOnnx = QFileInfo(modelPath).suffix().compare("onnx", Qt::CaseInsensitive) == 0;
    if (usingOnnx && !m_cppOnnxCheck->isChecked()) {
        const QString defaultPt = findDefaultModel();
        if (QFileInfo(defaultPt).suffix().compare("pt", Qt::CaseInsensitive) == 0
            && QFileInfo::exists(defaultPt)) {
            const QMessageBox::StandardButton choice =
                QMessageBox::question(this,
                                      "ONNX 推理提示",
                                      "当前选择的是 ONNX 模型，Python 推理需要 onnxruntime 环境。\n\n"
                                      "课堂演示建议优先使用最新训练的 best.pt，速度和稳定性更好。\n\n"
                                      "是否自动切换到最新 best.pt？",
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes);
            if (choice == QMessageBox::Yes) {
                modelPath = defaultPt;
                m_modelEdit->setText(defaultPt);
                appendLog("已自动切换到最新训练模型：" + defaultPt);
            } else {
                appendLog("继续使用 ONNX 模型执行 Python 推理，需要 onnxruntime 可正常加载。");
            }
        } else {
            appendLog("当前选择 ONNX 模型，Python 推理需要 onnxruntime 可正常加载。");
        }
    }

    if (m_cppOnnxCheck->isChecked() && QFileInfo(modelPath).suffix().compare("onnx", Qt::CaseInsensitive) == 0) {
        appendLog("尝试使用 C++ ONNX Runtime 执行量化模型推理。");
        const QuantizedRunOutput cppOutput = QuantizedOnnxRunner::run(modelPath,
                                                                      m_imageEdit->text().trimmed(),
                                                                      runDir,
                                                                      m_confSpin->value());
        appendLog(cppOutput.message);
        if (cppOutput.success) {
            m_startButton->setEnabled(true);
            m_lastResultImage = cppOutput.resultImagePath;
            m_lastResultJson = cppOutput.resultJsonPath;
            showImage(m_resultLabel, m_lastResultImage);
            m_saveButton->setEnabled(QFileInfo::exists(m_lastResultImage));
            const QVector<InferenceObject> objects = loadResults(m_lastResultJson);
            if (m_database) {
                const int inferenceId = m_database->insertInferenceRecord(m_imageEdit->text().trimmed(),
                                                                          modelPath,
                                                                          m_lastResultImage,
                                                                          m_lastResultJson,
                                                                          objects.size());
                if (inferenceId > 0) {
                    for (const InferenceObject &object : objects) {
                        m_database->insertInferenceObject(inferenceId, object);
                    }
                }
            }
            return;
        }
        appendLog("C++ 量化推理不可用，继续使用 Python ONNX Runtime 推理。");
    }
    m_viewModel->runInference(AppConfig::pythonProgram(),
                              AppConfig::scriptsDir() + "/infer_image.py",
                              modelPath,
                              m_imageEdit->text().trimmed(),
                              runDir,
                              m_deviceEdit->text().trimmed(),
                              m_confSpin->value());
}

void InferencePage::onInferenceFinished(bool success, int exitCode, const QString &resultImagePath, const QString &resultJsonPath)
{
    m_startButton->setEnabled(true);
    appendLog(QString("推理结束：%1，exitCode=%2").arg(success ? "成功" : "失败").arg(exitCode));
    if (!success) {
        return;
    }

    m_lastResultImage = resultImagePath;
    m_lastResultJson = resultJsonPath;
    showImage(m_resultLabel, m_lastResultImage);
    m_saveButton->setEnabled(QFileInfo::exists(m_lastResultImage));

    const QVector<InferenceObject> objects = loadResults(m_lastResultJson);
    if (m_database) {
        const int inferenceId = m_database->insertInferenceRecord(m_imageEdit->text().trimmed(),
                                                                  m_modelEdit->text().trimmed(),
                                                                  m_lastResultImage,
                                                                  m_lastResultJson,
                                                                  objects.size());
        if (inferenceId > 0) {
            for (const InferenceObject &object : objects) {
                m_database->insertInferenceObject(inferenceId, object);
            }
        }
    }
}

void InferencePage::saveResultImage()
{
    if (!QFileInfo::exists(m_lastResultImage)) {
        appendLog("没有可保存的结果图。");
        return;
    }
    const QString target = QFileDialog::getSaveFileName(this, "保存结果图", AppConfig::runsDir() + "/result.jpg", "JPEG Image (*.jpg)");
    if (!target.isEmpty()) {
        QFile::remove(target);
        if (QFile::copy(m_lastResultImage, target)) {
            appendLog("结果图已保存：" + target);
        } else {
            appendLog("结果图保存失败。");
        }
    }
}

QString InferencePage::findDefaultImage() const
{
    QDir dir(AppConfig::defaultDatasetPath() + "/test/images");
    const QFileInfoList files = dir.entryInfoList({"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp"}, QDir::Files, QDir::Name);
    if (!files.isEmpty()) {
        return QDir::fromNativeSeparators(files.first().absoluteFilePath());
    }
    return QString();
}

QString InferencePage::findDefaultModel() const
{
    QDir trainDir(AppConfig::runsDir() + "/train");
    const QFileInfoList runDirs = trainDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const QFileInfo &runDir : runDirs) {
        const QString best = runDir.absoluteFilePath() + "/weights/best.pt";
        if (QFileInfo::exists(best)) {
            return QDir::fromNativeSeparators(QFileInfo(best).absoluteFilePath());
        }
    }

    const QString modelsBest = AppConfig::modelsDir() + "/best.pt";
    if (QFileInfo::exists(modelsBest)) {
        return QDir::fromNativeSeparators(QFileInfo(modelsBest).absoluteFilePath());
    }

    return QStringLiteral("yolov8n.pt");
}

void InferencePage::showImage(QLabel *label, const QString &path)
{
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        label->setText("图片不可用");
        return;
    }
    label->setPixmap(pixmap.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QVector<InferenceObject> InferencePage::loadResults(const QString &jsonPath)
{
    QVector<InferenceObject> objects;
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        appendLog("结果 JSON 读取失败：" + jsonPath);
        return objects;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        appendLog("结果 JSON 解析失败：" + error.errorString());
        return objects;
    }

    const QJsonArray arr = doc.object().value("results").toArray();
    m_resultTable->setRowCount(0);
    for (const QJsonValue &value : arr) {
        const QJsonObject obj = value.toObject();
        const QJsonArray bbox = obj.value("bbox").toArray();
        InferenceObject item;
        item.classId = obj.value("class_id").toInt();
        item.className = obj.value("class_name").toString();
        item.confidence = obj.value("confidence").toDouble();
        if (bbox.size() >= 4) {
            item.x1 = bbox.at(0).toDouble();
            item.y1 = bbox.at(1).toDouble();
            item.x2 = bbox.at(2).toDouble();
            item.y2 = bbox.at(3).toDouble();
        }
        objects << item;

        const int row = m_resultTable->rowCount();
        m_resultTable->insertRow(row);
        m_resultTable->setItem(row, 0, new QTableWidgetItem(item.className));
        m_resultTable->setItem(row, 1, new QTableWidgetItem(QString::number(item.confidence, 'f', 3)));
        m_resultTable->setItem(row, 2, new QTableWidgetItem(QString("[%1,%2,%3,%4]")
                                                            .arg(item.x1, 0, 'f', 1)
                                                            .arg(item.y1, 0, 'f', 1)
                                                            .arg(item.x2, 0, 'f', 1)
                                                            .arg(item.y2, 0, 'f', 1)));
        m_resultTable->setItem(row, 3, new QTableWidgetItem(QString::number(item.classId)));
    }
    appendLog(QString("检测对象数量：%1").arg(objects.size()));
    return objects;
}

void InferencePage::appendLog(const QString &message)
{
    m_logEdit->appendPlainText(message);
}
