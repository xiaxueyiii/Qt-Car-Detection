#include "DatasetPage.h"

#include "../model/DatabaseManager.h"
#include "../utils/AppConfig.h"
#include "../viewmodel/DatasetViewModel.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
const QStringList kImageNameFilters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp"};

QString labelPathForImage(const QString &imagePath)
{
    QFileInfo imageInfo(imagePath);
    QDir imageDir = imageInfo.absoluteDir();
    if (imageDir.dirName().compare("images", Qt::CaseInsensitive) == 0) {
        imageDir.cdUp();
        return imageDir.filePath("labels/" + imageInfo.completeBaseName() + ".txt");
    }
    return imageInfo.absolutePath() + "/" + imageInfo.completeBaseName() + ".txt";
}

QString splitNameForImage(const QString &datasetPath, const QString &imagePath)
{
    const QString relative = QDir(datasetPath).relativeFilePath(imagePath).replace("\\", "/").toLower();
    if (relative.startsWith("train/images/")) {
        return "train";
    }
    if (relative.startsWith("valid/images/")) {
        return "valid";
    }
    if (relative.startsWith("val/images/")) {
        return "val";
    }
    if (relative.startsWith("test/images/")) {
        return "test";
    }
    return "images";
}

QPair<int, QString> labelSummary(const QString &labelPath)
{
    QFile file(labelPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {0, QString()};
    }
    int count = 0;
    QStringList classIds;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        ++count;
        const QString classId = line.section(' ', 0, 0);
        if (!classIds.contains(classId)) {
            classIds << classId;
        }
    }
    return {count, classIds.join(",")};
}
}

DatasetPage::DatasetPage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database),
      m_viewModel(new DatasetViewModel(database, this))
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(12);

    auto *topRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit(AppConfig::defaultDatasetPath(), this);
    auto *createButton = new QPushButton("创建数据集", this);
    auto *browseButton = new QPushButton("打开数据集", this);
    auto *importButton = new QPushButton("导入图片文件夹", this);
    auto *scanButton = new QPushButton("扫描并保存", this);
    topRow->addWidget(new QLabel("数据集路径", this));
    topRow->addWidget(m_pathEdit, 1);
    topRow->addWidget(createButton);
    topRow->addWidget(browseButton);
    topRow->addWidget(importButton);
    topRow->addWidget(scanButton);
    rootLayout->addLayout(topRow);

    auto *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);

    auto *statsPanel = new QWidget(splitter);
    auto *statsLayout = new QVBoxLayout(statsPanel);
    statsLayout->setContentsMargins(0, 0, 8, 0);
    m_table = new QTableWidget(0, 2, statsPanel);
    m_table->setHorizontalHeaderLabels({"项目", "结果"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statsLayout->addWidget(new QLabel("数据集统计", statsPanel));
    statsLayout->addWidget(m_table, 1);

    m_detailEdit = new QTextEdit(statsPanel);
    m_detailEdit->setReadOnly(true);
    m_detailEdit->setMinimumHeight(180);
    statsLayout->addWidget(m_detailEdit, 1);

    auto *imagePanel = new QWidget(splitter);
    auto *imageLayout = new QVBoxLayout(imagePanel);
    imageLayout->setContentsMargins(8, 0, 0, 0);
    m_imageTable = new QTableWidget(0, 5, imagePanel);
    m_imageTable->setHorizontalHeaderLabels({"图片", "分组", "标注数", "类别ID", "标签文件"});
    m_imageTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_imageTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_imageTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_imageTable->verticalHeader()->setVisible(false);
    m_imageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    imageLayout->addWidget(new QLabel("图像与标注列表", imagePanel));
    imageLayout->addWidget(m_imageTable, 1);

    splitter->addWidget(statsPanel);
    splitter->addWidget(imagePanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

    connect(createButton, &QPushButton::clicked, this, &DatasetPage::createDataset);
    connect(browseButton, &QPushButton::clicked, this, &DatasetPage::chooseDataset);
    connect(importButton, &QPushButton::clicked, this, &DatasetPage::importImageFolder);
    connect(scanButton, &QPushButton::clicked, this, &DatasetPage::scanDataset);
    connect(m_viewModel, &DatasetViewModel::logMessage, this, &DatasetPage::appendLog);

    scanDataset();
}

QStringList DatasetPage::classNames() const
{
    return m_lastStats.classNames;
}

void DatasetPage::createDataset()
{
    const QString parentDir = QFileDialog::getExistingDirectory(this, "选择新数据集保存位置", AppConfig::projectRoot());
    if (parentDir.isEmpty()) {
        return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(this, "创建数据集", "数据集名称：", QLineEdit::Normal, "CarDataset", &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }

    QDir parent(parentDir);
    const QString datasetPath = parent.filePath(name);
    QDir dataset(datasetPath);
    dataset.mkpath("train/images");
    dataset.mkpath("train/labels");
    dataset.mkpath("valid/images");
    dataset.mkpath("valid/labels");
    dataset.mkpath("test/images");
    dataset.mkpath("test/labels");

    QFile yaml(dataset.filePath("data.yaml"));
    if (!yaml.exists() && yaml.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&yaml);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#else
        out.setEncoding(QStringConverter::Utf8);
#endif
        out << "path: " << QDir::fromNativeSeparators(dataset.absolutePath()) << "\n";
        out << "train: train/images\n";
        out << "val: valid/images\n";
        out << "test: test/images\n";
        out << "nc: 1\n";
        out << "names:\n";
        out << "  - Car\n";
    }

    m_pathEdit->setText(QDir::fromNativeSeparators(dataset.absolutePath()));
    appendLog("已创建 YOLO 数据集结构：" + m_pathEdit->text());
    scanDataset();
}

void DatasetPage::chooseDataset()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择数据集目录", m_pathEdit->text());
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
        scanDataset();
    }
}

void DatasetPage::importImageFolder()
{
    const QString sourceDir = QFileDialog::getExistingDirectory(this, "选择要导入的图片文件夹", QString());
    if (sourceDir.isEmpty()) {
        return;
    }

    QDir dataset(m_pathEdit->text().trimmed());
    if (!dataset.exists()) {
        appendLog("当前数据集目录不存在，请先创建或打开数据集。");
        return;
    }
    dataset.mkpath("train/images");
    const QString targetDir = dataset.filePath("train/images");

    QDir source(sourceDir);
    const QFileInfoList files = source.entryInfoList(kImageNameFilters, QDir::Files, QDir::Name);
    int copied = 0;
    int skipped = 0;
    for (const QFileInfo &file : files) {
        const QString targetPath = QDir(targetDir).filePath(file.fileName());
        if (QFileInfo::exists(targetPath)) {
            ++skipped;
            continue;
        }
        if (QFile::copy(file.absoluteFilePath(), targetPath)) {
            ++copied;
        }
    }

    appendLog(QString("导入完成：复制 %1 张，跳过重名 %2 张。").arg(copied).arg(skipped));
    scanDataset();
}

void DatasetPage::scanDataset()
{
    m_detailEdit->clear();
    m_lastStats = m_viewModel->scan(m_pathEdit->text().trimmed());
    showStats(m_lastStats);
    m_viewModel->saveStats(m_lastStats);
    if (m_database) {
        m_database->saveDatasetCatalog(m_lastStats);
    }
    refreshImageTable(m_lastStats.datasetPath);
    emit classNamesUpdated(m_lastStats.classNames);
}

void DatasetPage::showStats(const DatasetStats &stats)
{
    m_table->setRowCount(0);
    const QList<QPair<QString, QString>> rows = {
        {"数据集路径", stats.datasetPath},
        {"图片数量", QString::number(stats.imageCount)},
        {"标注文件数量", QString::number(stats.labelCount)},
        {"类别数量", QString::number(stats.classCount)},
        {"识别格式", stats.isYoloDetection ? "YOLO 检测数据集" : "非标准 YOLO/待标注"},
        {"扫描时间", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")}
    };

    for (const auto &row : rows) {
        const int index = m_table->rowCount();
        m_table->insertRow(index);
        m_table->setItem(index, 0, new QTableWidgetItem(row.first));
        m_table->setItem(index, 1, new QTableWidgetItem(row.second));
    }

    QString detail;
    detail += stats.formatSummary + "\n\n";
    detail += "类别列表：\n";
    for (int i = 0; i < stats.classNames.size(); ++i) {
        detail += QString("%1  %2\n").arg(i).arg(stats.classNames.at(i));
    }
    if (stats.classNames.isEmpty()) {
        detail += "未从 data.yaml 中读取到类别名称。\n";
    }
    detail += "\n数据集管理：可创建 YOLO 目录结构、打开数据集、导入图片文件夹，并在右侧查看每张图片的标注状态。\n";
    m_detailEdit->setPlainText(detail);
}

void DatasetPage::refreshImageTable(const QString &datasetPath)
{
    m_imageTable->setRowCount(0);
    QDir root(datasetPath);
    if (!root.exists()) {
        return;
    }

    const QFileInfoList images = root.entryInfoList(kImageNameFilters, QDir::Files, QDir::Name)
                                  + QDir(root.filePath("train/images")).entryInfoList(kImageNameFilters, QDir::Files, QDir::Name)
                                  + QDir(root.filePath("valid/images")).entryInfoList(kImageNameFilters, QDir::Files, QDir::Name)
                                  + QDir(root.filePath("val/images")).entryInfoList(kImageNameFilters, QDir::Files, QDir::Name)
                                  + QDir(root.filePath("test/images")).entryInfoList(kImageNameFilters, QDir::Files, QDir::Name);

    for (const QFileInfo &image : images) {
        const QString imagePath = QDir::fromNativeSeparators(image.absoluteFilePath());
        const QString labelPath = QDir::fromNativeSeparators(labelPathForImage(imagePath));
        const auto summary = labelSummary(labelPath);
        const int row = m_imageTable->rowCount();
        m_imageTable->insertRow(row);
        m_imageTable->setItem(row, 0, new QTableWidgetItem(image.fileName()));
        m_imageTable->setItem(row, 1, new QTableWidgetItem(splitNameForImage(datasetPath, imagePath)));
        m_imageTable->setItem(row, 2, new QTableWidgetItem(QString::number(summary.first)));
        m_imageTable->setItem(row, 3, new QTableWidgetItem(summary.second));
        m_imageTable->setItem(row, 4, new QTableWidgetItem(labelPath));
    }
}

void DatasetPage::appendLog(const QString &message)
{
    m_detailEdit->append(message);
}
