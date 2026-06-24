#include "AnnotationPage.h"

#include "AnnotationCanvas.h"
#include "../model/DatabaseManager.h"
#include "../utils/AppConfig.h"
#include "../viewmodel/AnnotationViewModel.h"

#include <QAbstractSpinBox>
#include <QAbstractItemView>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
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
QString cleanClassName(const QString &name, int id)
{
    const QString trimmed = name.trimmed();
    return trimmed.isEmpty() ? QString("class_%1").arg(id) : trimmed;
}
}

AnnotationPage::AnnotationPage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database),
      m_viewModel(new AnnotationViewModel(this)),
      m_canvas(new AnnotationCanvas(this))
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(10);

    auto *topRow = new QHBoxLayout;
    m_folderEdit = new QLineEdit(AppConfig::defaultDatasetPath() + "/train/images", this);
    auto *browseButton = new QPushButton("选择图片文件夹", this);
    auto *loadButton = new QPushButton("加载", this);
    topRow->addWidget(new QLabel("图片文件夹", this));
    topRow->addWidget(m_folderEdit, 1);
    topRow->addWidget(browseButton);
    topRow->addWidget(loadButton);
    rootLayout->addLayout(topRow);

    auto *splitter = new QSplitter(this);

    auto *leftPanel = new QWidget(splitter);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    m_imageList = new QListWidget(leftPanel);
    leftLayout->addWidget(new QLabel("图片列表 / 标注概况", leftPanel));
    leftLayout->addWidget(m_imageList, 1);

    auto *navRow = new QHBoxLayout;
    auto *prevButton = new QPushButton("上一张", leftPanel);
    auto *nextButton = new QPushButton("下一张", leftPanel);
    navRow->addWidget(prevButton);
    navRow->addWidget(nextButton);
    leftLayout->addLayout(navRow);

    m_categoryList = new QListWidget(leftPanel);
    leftLayout->addWidget(new QLabel("类别列表", leftPanel));
    leftLayout->addWidget(m_categoryList, 1);

    auto *classRow = new QHBoxLayout;
    m_classSpin = new QSpinBox(leftPanel);
    m_classSpin->setRange(0, 999);
    m_classSpin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    m_classSpin->setAccelerated(true);
    m_classSpin->setKeyboardTracking(true);
    m_classSpin->setMinimumWidth(96);
    m_classNameEdit = new QLineEdit("Car", leftPanel);
    m_classNameEdit->setMinimumWidth(96);
    classRow->addWidget(new QLabel("当前类别", leftPanel));
    classRow->addWidget(m_classSpin);
    classRow->addWidget(m_classNameEdit, 1);
    leftLayout->addLayout(classRow);

    auto *categoryButtonRow = new QHBoxLayout;
    auto *addClassButton = new QPushButton("新增类别", leftPanel);
    auto *deleteClassButton = new QPushButton("删除类别", leftPanel);
    auto *applyClassButton = new QPushButton("应用到选中框", leftPanel);
    categoryButtonRow->addWidget(addClassButton);
    categoryButtonRow->addWidget(deleteClassButton);
    categoryButtonRow->addWidget(applyClassButton);
    leftLayout->addLayout(categoryButtonRow);

    auto *toolRow = new QHBoxLayout;
    auto *zoomInButton = new QPushButton("放大", leftPanel);
    auto *zoomOutButton = new QPushButton("缩小", leftPanel);
    auto *fitButton = new QPushButton("适应", leftPanel);
    auto *rotateLeftButton = new QPushButton("左旋", leftPanel);
    auto *rotateRightButton = new QPushButton("右旋", leftPanel);
    toolRow->addWidget(zoomInButton);
    toolRow->addWidget(zoomOutButton);
    toolRow->addWidget(fitButton);
    toolRow->addWidget(rotateLeftButton);
    toolRow->addWidget(rotateRightButton);
    leftLayout->addLayout(toolRow);

    auto *editRow = new QHBoxLayout;
    auto *deleteButton = new QPushButton("删除框", leftPanel);
    auto *undoButton = new QPushButton("撤销", leftPanel);
    auto *redoButton = new QPushButton("重做", leftPanel);
    auto *saveButton = new QPushButton("保存标签", leftPanel);
    editRow->addWidget(deleteButton);
    editRow->addWidget(undoButton);
    editRow->addWidget(redoButton);
    editRow->addWidget(saveButton);
    leftLayout->addLayout(editRow);

    m_boxTable = new QTableWidget(0, 6, leftPanel);
    m_boxTable->setHorizontalHeaderLabels({"类别ID", "类别名", "cx", "cy", "w", "h"});
    m_boxTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_boxTable->verticalHeader()->setVisible(false);
    m_boxTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_boxTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    leftLayout->addWidget(new QLabel("标注框列表（可双击编辑）", leftPanel));
    leftLayout->addWidget(m_boxTable, 1);

    m_logEdit = new QTextEdit(leftPanel);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    leftLayout->addWidget(m_logEdit);

    splitter->addWidget(leftPanel);
    splitter->addWidget(m_canvas);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    rootLayout->addWidget(splitter, 1);

    connect(browseButton, &QPushButton::clicked, this, &AnnotationPage::chooseFolder);
    connect(loadButton, &QPushButton::clicked, this, &AnnotationPage::loadFolder);
    connect(prevButton, &QPushButton::clicked, this, &AnnotationPage::showPreviousImage);
    connect(nextButton, &QPushButton::clicked, this, &AnnotationPage::showNextImage);
    connect(addClassButton, &QPushButton::clicked, this, &AnnotationPage::addCategory);
    connect(deleteClassButton, &QPushButton::clicked, this, &AnnotationPage::deleteCategory);
    connect(applyClassButton, &QPushButton::clicked, this, &AnnotationPage::applyClassToSelectedBox);
    connect(zoomInButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::zoomIn);
    connect(zoomOutButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::zoomOut);
    connect(fitButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::fitToWindow);
    connect(rotateLeftButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::rotateLeft);
    connect(rotateRightButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::rotateRight);
    connect(deleteButton, &QPushButton::clicked, m_canvas, &AnnotationCanvas::deleteSelected);
    connect(undoButton, &QPushButton::clicked, this, &AnnotationPage::undoEditAction);
    connect(redoButton, &QPushButton::clicked, this, &AnnotationPage::redoEditAction);
    connect(saveButton, &QPushButton::clicked, this, &AnnotationPage::saveLabels);

    connect(m_imageList, &QListWidget::currentRowChanged, this, &AnnotationPage::showImageAt);
    connect(m_categoryList, &QListWidget::currentRowChanged, this, &AnnotationPage::onCategorySelectionChanged);
    connect(m_classSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnnotationPage::onCurrentClassIdChanged);
    connect(m_classNameEdit, &QLineEdit::textEdited, this, &AnnotationPage::onCurrentClassNameEdited);
    connect(m_boxTable, &QTableWidget::itemChanged, this, &AnnotationPage::onBoxTableItemChanged);
    connect(m_boxTable, &QTableWidget::itemSelectionChanged, this, &AnnotationPage::onBoxTableSelectionChanged);
    connect(m_viewModel, &AnnotationViewModel::currentImageChanged, this, &AnnotationPage::onCurrentImageChanged);
    connect(m_viewModel, &AnnotationViewModel::logMessage, this, &AnnotationPage::appendLog);
    connect(m_canvas, &AnnotationCanvas::boxesChanged, this, &AnnotationPage::updateBoxTable);
    connect(m_canvas, &AnnotationCanvas::selectionChanged, this, &AnnotationPage::onCanvasSelectionChanged);
    connect(m_canvas, &AnnotationCanvas::logMessage, this, &AnnotationPage::appendLog);

    loadFolder();
}

void AnnotationPage::setClassNames(const QStringList &classNames)
{
    m_classNames = classNames;
    if (m_classNames.isEmpty()) {
        m_classNames << "Car";
    }
    m_viewModel->setClassNames(m_classNames);
    m_classSpin->setMaximum(qMax(0, m_classNames.size() - 1));
    refreshCategoryList();
    onCurrentClassIdChanged(qBound(0, m_classSpin->value(), m_classSpin->maximum()));
}

void AnnotationPage::chooseFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, "选择图片文件夹", m_folderEdit->text());
    if (!dir.isEmpty()) {
        m_folderEdit->setText(dir);
        loadFolder();
    }
}

void AnnotationPage::loadFolder()
{
    m_imageList->clear();
    if (!m_viewModel->loadImageFolder(m_folderEdit->text().trimmed())) {
        appendLog("文件夹中没有可用图片。");
        return;
    }
    for (const QString &image : m_viewModel->imagePaths()) {
        m_imageList->addItem(imageListText(image));
    }
    syncImageListSelection();
}

void AnnotationPage::showImageAt(int index)
{
    m_viewModel->setCurrentIndex(index);
}

void AnnotationPage::showPreviousImage()
{
    navigateImage(-1);
}

void AnnotationPage::showNextImage()
{
    navigateImage(1);
}

void AnnotationPage::onCurrentImageChanged(const QString &path)
{
    if (!m_canvas->loadImage(path)) {
        return;
    }
    const QVector<AnnotationBox> boxes = m_viewModel->loadLabels(m_canvas->imageSize());
    m_canvas->setBoxes(boxes);
    syncImageListSelection();
}

void AnnotationPage::saveLabels()
{
    const QVector<AnnotationBox> boxes = m_canvas->boxes();
    if (!m_viewModel->saveLabels(boxes, m_canvas->imageSize())) {
        return;
    }

    saveClassNamesToDatasetYaml();
    if (m_database) {
        const QString imagePath = m_viewModel->currentImagePath();
        const QString labelPath = m_viewModel->labelPathForImage(imagePath);
        if (!m_database->replaceImageAnnotations(datasetRootForCurrentFolder(), imagePath, labelPath, boxes, m_classNames)) {
            appendLog("标注写入数据库失败：" + m_database->lastError());
        } else {
            appendLog("标注已写入 SQLite：annotations/images/categories。");
        }
    }
    refreshImageListSummaries();
}

void AnnotationPage::updateBoxTable(const QVector<AnnotationBox> &boxes)
{
    QSignalBlocker blocker(m_boxTable);
    m_updatingBoxTable = true;
    m_boxTable->setRowCount(0);
    for (const AnnotationBox &box : boxes) {
        const QRectF rect = box.normalizedRect;
        const double cx = rect.x() + rect.width() / 2.0;
        const double cy = rect.y() + rect.height() / 2.0;
        const int row = m_boxTable->rowCount();
        m_boxTable->insertRow(row);
        m_boxTable->setItem(row, 0, new QTableWidgetItem(QString::number(box.classId)));
        m_boxTable->setItem(row, 1, new QTableWidgetItem(box.className));
        m_boxTable->setItem(row, 2, new QTableWidgetItem(QString::number(cx, 'f', 4)));
        m_boxTable->setItem(row, 3, new QTableWidgetItem(QString::number(cy, 'f', 4)));
        m_boxTable->setItem(row, 4, new QTableWidgetItem(QString::number(rect.width(), 'f', 4)));
        m_boxTable->setItem(row, 5, new QTableWidgetItem(QString::number(rect.height(), 'f', 4)));
    }
    m_updatingBoxTable = false;
}

void AnnotationPage::updateCurrentClass()
{
    const int id = m_classSpin->value();
    const QString name = m_classNameEdit->text().trimmed();
    m_canvas->setCurrentClass(id, cleanClassName(name, id));
}

void AnnotationPage::onCurrentClassIdChanged(int id)
{
    const int safeId = qBound(0, id, m_classSpin->maximum());
    const QString name = classNameForId(safeId);
    {
        QSignalBlocker blocker(m_classNameEdit);
        m_classNameEdit->setText(name);
    }
    if (safeId >= 0 && safeId < m_categoryList->count() && m_categoryList->currentRow() != safeId) {
        QSignalBlocker blocker(m_categoryList);
        m_categoryList->setCurrentRow(safeId);
    }
    updateCurrentClass();
}

void AnnotationPage::onCurrentClassNameEdited(const QString &)
{
    updateCurrentClass();
}

void AnnotationPage::addCategory()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, "新增类别", "类别名称：", QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    if (m_classNames.contains(name)) {
        appendLog("类别已存在：" + name);
        return;
    }

    m_classNames << name;
    m_categoryUndoStack.clear();
    m_categoryRedoStack.clear();
    m_viewModel->setClassNames(m_classNames);
    m_classSpin->setMaximum(qMax(0, m_classNames.size() - 1));
    m_classSpin->setValue(m_classNames.size() - 1);
    refreshCategoryList();
    persistClassNames();
    appendLog("新增类别：" + name);
}

void AnnotationPage::deleteCategory()
{
    const int row = m_categoryList->currentRow();
    if (row < 0 || row >= m_classNames.size()) {
        appendLog("请先在类别列表中选择要删除的类别。");
        return;
    }
    if (m_classNames.size() <= 1) {
        QMessageBox::information(this, "不能删除类别", "至少需要保留一个类别。");
        return;
    }

    const QString removed = m_classNames.at(row);
    const int usageCount = labelUsageCountForClassId(row);
    QString message = QString("确定要删除类别 %1：%2 吗？\n\n"
                              "删除后会立即更新 data.yaml 和 SQLite 数据库，后面的类别编号会前移。")
                          .arg(row)
                          .arg(removed);
    if (usageCount > 0) {
        message += QString("\n\n检测到当前数据集中有 %1 个标注使用这个类别 ID，删除后这些标注的类别解释可能发生变化。")
                       .arg(usageCount);
    }
    message += "\n\n删除后可以点击“撤销”恢复本次删除。";

    const QMessageBox::StandardButton result =
        QMessageBox::question(this,
                              "确认删除类别",
                              message,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);
    if (result != QMessageBox::Yes) {
        appendLog("已取消删除类别：" + removed);
        return;
    }

    CategoryDeleteHistory history;
    history.beforeNames = m_classNames;
    history.beforeClassId = m_classSpin->value();
    history.removedClassId = row;
    history.removedClassName = removed;

    QStringList afterNames = m_classNames;
    afterNames.removeAt(row);
    history.afterNames = afterNames;
    history.afterClassId = qBound(0,
                                  row >= afterNames.size() ? afterNames.size() - 1 : row,
                                  qMax(0, afterNames.size() - 1));

    m_categoryUndoStack.append(history);
    if (m_categoryUndoStack.size() > 50) {
        m_categoryUndoStack.removeFirst();
    }
    m_categoryRedoStack.clear();

    applyCategoryState(afterNames,
                       history.afterClassId,
                       QString("已删除类别：%1 %2。可点击“撤销”恢复。")
                           .arg(row)
                           .arg(removed));
}

void AnnotationPage::undoEditAction()
{
    if (!m_categoryUndoStack.isEmpty()) {
        const CategoryDeleteHistory history = m_categoryUndoStack.takeLast();
        m_categoryRedoStack.append(history);
        applyCategoryState(history.beforeNames,
                           history.beforeClassId,
                           QString("已撤销删除类别：%1 %2。")
                               .arg(history.removedClassId)
                               .arg(history.removedClassName));
        return;
    }

    m_canvas->undo();
}

void AnnotationPage::redoEditAction()
{
    if (!m_categoryRedoStack.isEmpty()) {
        const CategoryDeleteHistory history = m_categoryRedoStack.takeLast();
        m_categoryUndoStack.append(history);
        applyCategoryState(history.afterNames,
                           history.afterClassId,
                           QString("已重做删除类别：%1 %2。")
                               .arg(history.removedClassId)
                               .arg(history.removedClassName));
        return;
    }

    m_canvas->redo();
}

void AnnotationPage::applyClassToSelectedBox()
{
    const int id = m_classSpin->value();
    const QString name = cleanClassName(m_classNameEdit->text(), id);
    bool categoriesChanged = false;
    if (id >= m_classNames.size()) {
        while (m_classNames.size() < id) {
            m_classNames << QString("class_%1").arg(m_classNames.size());
        }
        m_classNames << name;
        refreshCategoryList();
        m_viewModel->setClassNames(m_classNames);
        categoriesChanged = true;
    } else if (m_classNames.value(id) != name) {
        m_classNames[id] = name;
        refreshCategoryList();
        m_viewModel->setClassNames(m_classNames);
        categoriesChanged = true;
    }
    if (categoriesChanged) {
        m_categoryUndoStack.clear();
        m_categoryRedoStack.clear();
        persistClassNames();
    }
    m_canvas->updateSelectedClass(id, name);
}

void AnnotationPage::onCategorySelectionChanged()
{
    const int row = m_categoryList->currentRow();
    if (row < 0 || row >= m_classNames.size()) {
        return;
    }
    {
        QSignalBlocker spinBlocker(m_classSpin);
        QSignalBlocker nameBlocker(m_classNameEdit);
        m_classSpin->setValue(row);
        m_classNameEdit->setText(m_classNames.at(row));
    }
    updateCurrentClass();
}

void AnnotationPage::onBoxTableItemChanged(QTableWidgetItem *item)
{
    if (m_updatingBoxTable || !item) {
        return;
    }

    const int row = item->row();
    const QVector<AnnotationBox> boxes = m_canvas->boxes();
    if (row < 0 || row >= boxes.size()) {
        return;
    }

    bool okId = false;
    bool okCx = false;
    bool okCy = false;
    bool okW = false;
    bool okH = false;
    const int classId = m_boxTable->item(row, 0)->text().toInt(&okId);
    const QString className = cleanClassName(m_boxTable->item(row, 1)->text(), classId);
    const double cx = m_boxTable->item(row, 2)->text().toDouble(&okCx);
    const double cy = m_boxTable->item(row, 3)->text().toDouble(&okCy);
    const double w = m_boxTable->item(row, 4)->text().toDouble(&okW);
    const double h = m_boxTable->item(row, 5)->text().toDouble(&okH);
    if (!okId || !okCx || !okCy || !okW || !okH) {
        appendLog("标注表格包含非法数字，已忽略。");
        return;
    }

    AnnotationBox box = boxes.at(row);
    box.classId = qMax(0, classId);
    box.className = className;
    box.normalizedRect = QRectF(cx - w / 2.0, cy - h / 2.0, w, h);
    m_canvas->updateBox(row, box);
}

void AnnotationPage::onBoxTableSelectionChanged()
{
    if (m_updatingBoxTable) {
        return;
    }
    const int row = m_boxTable->currentRow();
    if (row >= 0) {
        m_canvas->selectBox(row);
    }
}

void AnnotationPage::onCanvasSelectionChanged(int index)
{
    if (index < 0 || index >= m_boxTable->rowCount()) {
        m_boxTable->clearSelection();
        return;
    }
    QSignalBlocker blocker(m_boxTable);
    m_boxTable->selectRow(index);
}

void AnnotationPage::appendLog(const QString &message)
{
    m_logEdit->append(message);
}

void AnnotationPage::navigateImage(int offset)
{
    if (m_imageList->count() <= 0) {
        return;
    }

    int baseRow = m_imageList->currentRow();
    if (baseRow < 0) {
        baseRow = m_viewModel->currentIndex();
    }
    if (baseRow < 0) {
        baseRow = 0;
    }

    const int targetRow = qBound(0, baseRow + offset, m_imageList->count() - 1);
    selectImageRow(targetRow);
}

void AnnotationPage::selectImageRow(int row)
{
    if (m_imageList->count() <= 0) {
        return;
    }

    const int targetRow = qBound(0, row, m_imageList->count() - 1);
    if (m_imageList->currentRow() != targetRow) {
        m_imageList->setCurrentRow(targetRow);
        return;
    }

    if (m_viewModel->currentIndex() != targetRow) {
        showImageAt(targetRow);
    }
}

void AnnotationPage::syncImageListSelection()
{
    const int index = m_viewModel->currentIndex();
    if (index >= 0 && index < m_imageList->count() && m_imageList->currentRow() != index) {
        QSignalBlocker blocker(m_imageList);
        m_imageList->setCurrentRow(index);
    }
}

QString AnnotationPage::classNameForId(int id) const
{
    if (id >= 0 && id < m_classNames.size()) {
        return m_classNames.at(id);
    }
    return QString("class_%1").arg(qMax(0, id));
}

void AnnotationPage::refreshCategoryList()
{
    const int currentId = m_classSpin ? m_classSpin->value() : -1;
    QSignalBlocker blocker(m_categoryList);
    m_categoryList->clear();
    for (int i = 0; i < m_classNames.size(); ++i) {
        m_categoryList->addItem(QString("%1  %2").arg(i).arg(m_classNames.at(i)));
    }
    if (currentId >= 0 && currentId < m_categoryList->count()) {
        m_categoryList->setCurrentRow(currentId);
    }
}

void AnnotationPage::persistClassNames()
{
    if (!saveClassNamesToDatasetYaml()) {
        appendLog("类别保存到 data.yaml 失败，请检查数据集目录权限。");
    }
    if (m_database && !m_database->saveCategories(datasetRootForCurrentFolder(), m_classNames)) {
        appendLog("类别保存到数据库失败：" + m_database->lastError());
    }
}

void AnnotationPage::applyCategoryState(const QStringList &classNames, int currentClassId, const QString &logMessage)
{
    m_classNames = classNames;
    if (m_classNames.isEmpty()) {
        m_classNames << "class_0";
    }

    m_viewModel->setClassNames(m_classNames);
    m_classSpin->setMaximum(qMax(0, m_classNames.size() - 1));

    const int safeId = qBound(0, currentClassId, m_classSpin->maximum());
    {
        QSignalBlocker blocker(m_classSpin);
        m_classSpin->setValue(safeId);
    }

    refreshCategoryList();
    onCurrentClassIdChanged(safeId);
    persistClassNames();
    appendLog(logMessage);
}

int AnnotationPage::labelUsageCountForClassId(int classId) const
{
    if (classId < 0) {
        return 0;
    }

    const QString datasetRoot = datasetRootForCurrentFolder();
    if (datasetRoot.isEmpty()) {
        return 0;
    }

    int count = 0;
    QDirIterator it(datasetRoot, QStringList() << "*.txt", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString labelPath = it.next();
        const QString normalizedPath = QDir::fromNativeSeparators(labelPath).toLower();
        if (!normalizedPath.contains("/labels/")) {
            continue;
        }

        QFile file(labelPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }

            bool ok = false;
            const int labelClassId = line.section(' ', 0, 0).toInt(&ok);
            if (ok && labelClassId == classId) {
                ++count;
            }
        }
    }
    return count;
}

void AnnotationPage::refreshImageListSummaries()
{
    const QStringList images = m_viewModel->imagePaths();
    for (int i = 0; i < images.size() && i < m_imageList->count(); ++i) {
        m_imageList->item(i)->setText(imageListText(images.at(i)));
    }
}

QString AnnotationPage::datasetRootForCurrentFolder() const
{
    QDir dir(m_folderEdit->text().trimmed());
    if (dir.dirName().compare("images", Qt::CaseInsensitive) == 0) {
        dir.cdUp();
        const QString splitName = dir.dirName().toLower();
        if (splitName == "train" || splitName == "valid" || splitName == "val" || splitName == "test") {
            dir.cdUp();
        }
    }
    return QDir::fromNativeSeparators(dir.absolutePath());
}

QString AnnotationPage::labelPathForImagePath(const QString &imagePath) const
{
    QFileInfo imageInfo(imagePath);
    QDir imageDir = imageInfo.absoluteDir();
    if (imageDir.dirName().compare("images", Qt::CaseInsensitive) == 0) {
        imageDir.cdUp();
        return imageDir.filePath("labels/" + imageInfo.completeBaseName() + ".txt");
    }
    return imageInfo.absolutePath() + "/" + imageInfo.completeBaseName() + ".txt";
}

QString AnnotationPage::imageListText(const QString &imagePath) const
{
    QFile labelFile(labelPathForImagePath(imagePath));
    int count = 0;
    QStringList classIds;
    if (labelFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&labelFile);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            ++count;
            const QString id = line.section(' ', 0, 0);
            if (!classIds.contains(id)) {
                classIds << id;
            }
        }
    }
    QString suffix = QString("  [%1框").arg(count);
    if (!classIds.isEmpty()) {
        suffix += " / 类别 " + classIds.join(",");
    }
    suffix += "]";
    return QFileInfo(imagePath).fileName() + suffix;
}

bool AnnotationPage::saveClassNamesToDatasetYaml() const
{
    const QString datasetRoot = datasetRootForCurrentFolder();
    if (datasetRoot.isEmpty()) {
        return false;
    }

    QDir root(datasetRoot);
    if (!root.exists()) {
        return false;
    }

    QFile file(root.filePath("data.yaml"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif
    out << "path: " << QDir::fromNativeSeparators(root.absolutePath()) << "\n";
    out << "train: train/images\n";
    out << "val: valid/images\n";
    out << "test: test/images\n";
    out << "nc: " << m_classNames.size() << "\n";
    out << "names:\n";
    for (const QString &name : m_classNames) {
        out << "  - " << name << "\n";
    }
    return true;
}
