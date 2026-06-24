#include "DatasetViewModel.h"

#include "../model/DatabaseManager.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

DatasetViewModel::DatasetViewModel(DatabaseManager *database, QObject *parent)
    : QObject(parent),
      m_database(database)
{
}

DatasetStats DatasetViewModel::scan(const QString &datasetPath)
{
    DatasetStats stats;
    stats.datasetPath = QDir::fromNativeSeparators(datasetPath);

    QDir root(stats.datasetPath);
    if (!root.exists()) {
        stats.formatSummary = "数据集目录不存在。";
        emit logMessage(stats.formatSummary);
        return stats;
    }

    const QStringList imageSuffixes = {"jpg", "jpeg", "png", "bmp", "webp"};
    stats.imageCount = countFiles(root.absolutePath(), imageSuffixes);
    stats.labelCount = countFiles(root.absolutePath(), {"txt"});

    QString yamlPath = root.filePath("data.yaml");
    if (!QFileInfo::exists(yamlPath)) {
        yamlPath = root.filePath("dataset.yaml");
    }

    stats.classNames = parseClassNamesFromYaml(yamlPath);
    stats.classCount = stats.classNames.size();

    const bool trainOk = hasYoloSplit(root.absolutePath(), "train");
    const bool validOk = hasYoloSplit(root.absolutePath(), "valid") || hasYoloSplit(root.absolutePath(), "val");
    const bool testOk = hasYoloSplit(root.absolutePath(), "test");
    stats.isYoloDetection = trainOk && validOk && stats.labelCount > 0;

    if (stats.isYoloDetection) {
        stats.formatSummary = QString("YOLO 检测数据集：train=%1，valid/val=%2，test=%3。")
                                  .arg(trainOk ? "OK" : "缺失",
                                       validOk ? "OK" : "缺失",
                                       testOk ? "OK" : "可选缺失");
    } else if (stats.imageCount > 0 && stats.labelCount == 0) {
        stats.formatSummary = "检测到图片但没有 YOLO 标签，训练前需要先完成标注。";
    } else {
        stats.formatSummary = "未识别为标准 YOLO 检测结构，请检查 images/labels 或 data.yaml。";
    }

    m_lastClassNames = stats.classNames;
    emit logMessage(QString("扫描完成：图片 %1，标签 %2，类别 %3。")
                        .arg(stats.imageCount)
                        .arg(stats.labelCount)
                        .arg(stats.classCount));
    return stats;
}

bool DatasetViewModel::saveStats(const DatasetStats &stats)
{
    if (!m_database) {
        emit logMessage("数据库未初始化，无法保存统计信息。");
        return false;
    }
    const bool ok = m_database->insertDatasetStats(stats);
    emit logMessage(ok ? "数据集统计已保存到 SQLite。" : "保存统计失败：" + m_database->lastError());
    return ok;
}

QStringList DatasetViewModel::classNames() const
{
    return m_lastClassNames;
}

QStringList DatasetViewModel::parseClassNamesFromYaml(const QString &yamlPath) const
{
    QStringList names;
    QFile file(yamlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return names;
    }

    const QString text = QString::fromUtf8(file.readAll());
    QRegularExpression inlineNames(R"(names\s*:\s*\[(.*?)\])", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = inlineNames.match(text);
    if (match.hasMatch()) {
        const QString body = match.captured(1);
        QRegularExpression itemRegex(R"(['"]?([^,'"\]]+)['"]?)");
        QRegularExpressionMatchIterator it = itemRegex.globalMatch(body);
        while (it.hasNext()) {
            QString name = it.next().captured(1).trimmed();
            if (!name.isEmpty()) {
                names << name;
            }
        }
        return names;
    }

    bool inNamesBlock = false;
    const QStringList lines = text.split('\n');
    QRegularExpression blockItem(R"(^\s*(?:-\s*)?(?:\d+\s*:\s*)?['"]?([^'":#]+)['"]?\s*(?:#.*)?$)");
    for (const QString &line : lines) {
        if (line.trimmed().startsWith("names:")) {
            inNamesBlock = true;
            continue;
        }
        if (inNamesBlock && !line.startsWith(' ') && !line.startsWith('-')) {
            break;
        }
        if (inNamesBlock) {
            QRegularExpressionMatch blockMatch = blockItem.match(line);
            if (blockMatch.hasMatch()) {
                const QString name = blockMatch.captured(1).trimmed();
                if (!name.isEmpty()) {
                    names << name;
                }
            }
        }
    }
    return names;
}

int DatasetViewModel::countFiles(const QString &root, const QStringList &suffixes) const
{
    int count = 0;
    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info(it.next());
        if (suffixes.contains(info.suffix().toLower())) {
            ++count;
        }
    }
    return count;
}

bool DatasetViewModel::hasYoloSplit(const QString &datasetPath, const QString &splitName) const
{
    QDir splitDir(QDir(datasetPath).filePath(splitName));
    return splitDir.exists("images") && splitDir.exists("labels");
}
