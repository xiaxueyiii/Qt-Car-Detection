#ifndef DATASETPAGE_H
#define DATASETPAGE_H

#include "../model/DataTypes.h"

#include <QWidget>

class DatabaseManager;
class DatasetViewModel;
class QLineEdit;
class QTableWidget;
class QTextEdit;

class DatasetPage : public QWidget
{
    Q_OBJECT

public:
    explicit DatasetPage(DatabaseManager *database, QWidget *parent = nullptr);
    QStringList classNames() const;

signals:
    void classNamesUpdated(const QStringList &classNames);

private slots:
    void createDataset();
    void chooseDataset();
    void importImageFolder();
    void scanDataset();

private:
    void showStats(const DatasetStats &stats);
    void refreshImageTable(const QString &datasetPath);
    void appendLog(const QString &message);

private:
    DatabaseManager *m_database = nullptr;
    DatasetViewModel *m_viewModel = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QTableWidget *m_table = nullptr;
    QTableWidget *m_imageTable = nullptr;
    QTextEdit *m_detailEdit = nullptr;
    DatasetStats m_lastStats;
};

#endif // DATASETPAGE_H
