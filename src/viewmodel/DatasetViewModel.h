#ifndef DATASETVIEWMODEL_H
#define DATASETVIEWMODEL_H

#include "../model/DataTypes.h"

#include <QObject>

class DatabaseManager;

class DatasetViewModel : public QObject
{
    Q_OBJECT

public:
    explicit DatasetViewModel(DatabaseManager *database, QObject *parent = nullptr);

    DatasetStats scan(const QString &datasetPath);
    bool saveStats(const DatasetStats &stats);
    QStringList classNames() const;

signals:
    void logMessage(const QString &message);

private:
    QStringList parseClassNamesFromYaml(const QString &yamlPath) const;
    int countFiles(const QString &root, const QStringList &suffixes) const;
    bool hasYoloSplit(const QString &datasetPath, const QString &splitName) const;

private:
    DatabaseManager *m_database = nullptr;
    QStringList m_lastClassNames;
};

#endif // DATASETVIEWMODEL_H
