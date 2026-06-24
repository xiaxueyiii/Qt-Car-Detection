#ifndef ANNOTATIONPAGE_H
#define ANNOTATIONPAGE_H

#include "../model/DataTypes.h"

#include <QStringList>
#include <QVector>
#include <QWidget>

class AnnotationCanvas;
class AnnotationViewModel;
class DatabaseManager;
class QListWidget;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;

class AnnotationPage : public QWidget
{
    Q_OBJECT

public:
    explicit AnnotationPage(DatabaseManager *database, QWidget *parent = nullptr);

public slots:
    void setClassNames(const QStringList &classNames);

private slots:
    void chooseFolder();
    void loadFolder();
    void showPreviousImage();
    void showNextImage();
    void showImageAt(int index);
    void onCurrentImageChanged(const QString &path);
    void saveLabels();
    void updateBoxTable(const QVector<AnnotationBox> &boxes);
    void updateCurrentClass();
    void onCurrentClassIdChanged(int id);
    void onCurrentClassNameEdited(const QString &text);
    void addCategory();
    void deleteCategory();
    void undoEditAction();
    void redoEditAction();
    void applyClassToSelectedBox();
    void onCategorySelectionChanged();
    void onBoxTableItemChanged(QTableWidgetItem *item);
    void onBoxTableSelectionChanged();
    void onCanvasSelectionChanged(int index);

private:
    void appendLog(const QString &message);
    void navigateImage(int offset);
    void selectImageRow(int row);
    void syncImageListSelection();
    QString classNameForId(int id) const;
    void refreshCategoryList();
    void refreshImageListSummaries();
    QString datasetRootForCurrentFolder() const;
    QString labelPathForImagePath(const QString &imagePath) const;
    QString imageListText(const QString &imagePath) const;
    bool saveClassNamesToDatasetYaml() const;
    void persistClassNames();
    void applyCategoryState(const QStringList &classNames, int currentClassId, const QString &logMessage);
    int labelUsageCountForClassId(int classId) const;

private:
    struct CategoryDeleteHistory
    {
        QStringList beforeNames;
        QStringList afterNames;
        int beforeClassId = 0;
        int afterClassId = 0;
        int removedClassId = -1;
        QString removedClassName;
    };

    DatabaseManager *m_database = nullptr;
    AnnotationViewModel *m_viewModel = nullptr;
    AnnotationCanvas *m_canvas = nullptr;
    QLineEdit *m_folderEdit = nullptr;
    QListWidget *m_imageList = nullptr;
    QListWidget *m_categoryList = nullptr;
    QSpinBox *m_classSpin = nullptr;
    QLineEdit *m_classNameEdit = nullptr;
    QTableWidget *m_boxTable = nullptr;
    QTextEdit *m_logEdit = nullptr;
    QStringList m_classNames;
    QVector<CategoryDeleteHistory> m_categoryUndoStack;
    QVector<CategoryDeleteHistory> m_categoryRedoStack;
    bool m_updatingBoxTable = false;
};

#endif // ANNOTATIONPAGE_H
