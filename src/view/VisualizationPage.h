#ifndef VISUALIZATIONPAGE_H
#define VISUALIZATIONPAGE_H

#include <QWidget>

class DatabaseManager;
class QTabWidget;
class QTableWidget;
class QTextEdit;

class VisualizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationPage(DatabaseManager *database, QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    QTableWidget *createTable(const QStringList &headers);
    void fillTable(QTableWidget *table, const QString &tableName, const QStringList &columns);

private:
    DatabaseManager *m_database = nullptr;
    QTextEdit *m_summaryEdit = nullptr;
    QTableWidget *m_datasetTable = nullptr;
    QTableWidget *m_trainTable = nullptr;
    QTableWidget *m_trainingRunTable = nullptr;
    QTableWidget *m_modelTable = nullptr;
    QTableWidget *m_modelFileTable = nullptr;
    QTableWidget *m_inferTable = nullptr;
    QTableWidget *m_imageTable = nullptr;
    QTableWidget *m_categoryTable = nullptr;
    QTableWidget *m_annotationTable = nullptr;
};

#endif // VISUALIZATIONPAGE_H
