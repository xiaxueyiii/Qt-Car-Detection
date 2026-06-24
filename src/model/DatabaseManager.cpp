#include "DatabaseManager.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent),
      m_connectionName("cardet_" + QUuid::createUuid().toString(QUuid::Id128))
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isValid()) {
        const QString name = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
}

bool DatabaseManager::open(const QString &databasePath)
{
    QDir().mkpath(QFileInfo(databasePath).absolutePath());

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(databasePath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::initialize()
{
    return execSql("CREATE TABLE IF NOT EXISTS dataset_stats ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "dataset_path TEXT,"
                   "image_count INTEGER,"
                   "label_count INTEGER,"
                   "class_count INTEGER,"
                   "created_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS train_records ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "dataset_path TEXT,"
                   "model_type TEXT,"
                   "epochs INTEGER,"
                   "batch_size INTEGER,"
                   "image_size INTEGER,"
                   "result_model_path TEXT,"
                   "status TEXT,"
                   "created_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS model_records ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "model_path TEXT,"
                   "model_type TEXT,"
                   "quantized_path TEXT,"
                   "quantization_type TEXT,"
                   "created_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS inference_records ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "image_path TEXT,"
                   "model_path TEXT,"
                   "result_image_path TEXT,"
                   "result_json_path TEXT,"
                   "detected_count INTEGER,"
                   "created_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS inference_objects ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "inference_id INTEGER,"
                   "class_id INTEGER,"
                   "class_name TEXT,"
                   "confidence REAL,"
                   "x1 REAL,"
                   "y1 REAL,"
                   "x2 REAL,"
                   "y2 REAL)")
        && execSql("CREATE TABLE IF NOT EXISTS datasets ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "name TEXT,"
                   "path TEXT UNIQUE,"
                   "created_at TEXT,"
                   "updated_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS images ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "dataset_id INTEGER,"
                   "file_path TEXT UNIQUE,"
                   "label_path TEXT,"
                   "annotation_count INTEGER DEFAULT 0,"
                   "category_summary TEXT,"
                   "created_at TEXT,"
                   "updated_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS categories ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "dataset_id INTEGER,"
                   "class_id INTEGER,"
                   "name TEXT,"
                   "created_at TEXT,"
                   "UNIQUE(dataset_id,class_id))")
        && execSql("CREATE TABLE IF NOT EXISTS annotations ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "image_id INTEGER,"
                   "class_id INTEGER,"
                   "class_name TEXT,"
                   "cx REAL,"
                   "cy REAL,"
                   "width REAL,"
                   "height REAL,"
                   "created_at TEXT,"
                   "updated_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS training_runs ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "dataset_path TEXT,"
                   "train_path TEXT,"
                   "val_path TEXT,"
                   "model_type TEXT,"
                   "epochs INTEGER,"
                   "batch_size INTEGER,"
                   "image_size INTEGER,"
                   "learning_rate REAL,"
                   "output_dir TEXT,"
                   "result_model_path TEXT,"
                   "metrics_json TEXT,"
                   "status TEXT,"
                   "started_at TEXT,"
                   "finished_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS model_files ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "source_model_path TEXT,"
                   "model_path TEXT,"
                   "model_type TEXT,"
                   "quantization_type TEXT,"
                   "backend TEXT,"
                   "created_at TEXT)")
        && execSql("CREATE TABLE IF NOT EXISTS inference_results ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "image_path TEXT,"
                   "model_path TEXT,"
                   "result_image_path TEXT,"
                   "result_json_path TEXT,"
                   "detected_count INTEGER,"
                   "backend TEXT,"
                   "created_at TEXT)");
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

bool DatabaseManager::insertDatasetStats(const DatasetStats &stats)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO dataset_stats "
                  "(dataset_path,image_count,label_count,class_count,created_at) "
                  "VALUES (?,?,?,?,?)");
    query.addBindValue(stats.datasetPath);
    query.addBindValue(stats.imageCount);
    query.addBindValue(stats.labelCount);
    query.addBindValue(stats.classCount);
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::saveDatasetCatalog(const DatasetStats &stats)
{
    const int datasetId = ensureDatasetRecord(stats.datasetPath, QFileInfo(stats.datasetPath).fileName());
    if (datasetId <= 0) {
        return false;
    }
    return saveCategories(stats.datasetPath, stats.classNames);
}

bool DatabaseManager::saveCategories(const QString &datasetPath, const QStringList &classNames)
{
    const int datasetId = ensureDatasetRecord(datasetPath, QFileInfo(datasetPath).fileName());
    if (datasetId <= 0) {
        return false;
    }

    {
        QSqlQuery cleanup(m_db);
        cleanup.prepare("DELETE FROM categories WHERE dataset_id=?");
        cleanup.addBindValue(datasetId);
        if (!cleanup.exec()) {
            m_lastError = cleanup.lastError().text();
            return false;
        }
    }

    for (int i = 0; i < classNames.size(); ++i) {
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO categories "
                      "(dataset_id,class_id,name,created_at) "
                      "VALUES (?,?,?,?)");
        query.addBindValue(datasetId);
        query.addBindValue(i);
        query.addBindValue(classNames.at(i));
        query.addBindValue(now());
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::replaceImageAnnotations(const QString &datasetPath,
                                              const QString &imagePath,
                                              const QString &labelPath,
                                              const QVector<AnnotationBox> &boxes,
                                              const QStringList &classNames)
{
    const QString normalizedDatasetPath = QDir::fromNativeSeparators(datasetPath);
    const QString normalizedImagePath = QDir::fromNativeSeparators(imagePath);
    const QString normalizedLabelPath = QDir::fromNativeSeparators(labelPath);
    const int datasetId = ensureDatasetRecord(normalizedDatasetPath, QFileInfo(normalizedDatasetPath).fileName());
    if (datasetId <= 0) {
        return false;
    }
    if (!saveCategories(normalizedDatasetPath, classNames)) {
        return false;
    }

    QStringList categoryNames;
    for (const AnnotationBox &box : boxes) {
        const QString name = box.className.isEmpty() ? QString("class_%1").arg(box.classId) : box.className;
        if (!categoryNames.contains(name)) {
            categoryNames << name;
        }
        QSqlQuery categoryQuery(m_db);
        categoryQuery.prepare("INSERT OR IGNORE INTO categories "
                              "(dataset_id,class_id,name,created_at) VALUES (?,?,?,?)");
        categoryQuery.addBindValue(datasetId);
        categoryQuery.addBindValue(box.classId);
        categoryQuery.addBindValue(name);
        categoryQuery.addBindValue(now());
        if (!categoryQuery.exec()) {
            m_lastError = categoryQuery.lastError().text();
            return false;
        }
    }

    int imageId = -1;
    {
        QSqlQuery query(m_db);
        query.prepare("SELECT id FROM images WHERE file_path=?");
        query.addBindValue(normalizedImagePath);
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
        if (query.next()) {
            imageId = query.value(0).toInt();
        }
    }

    if (imageId <= 0) {
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO images "
                      "(dataset_id,file_path,label_path,annotation_count,category_summary,created_at,updated_at) "
                      "VALUES (?,?,?,?,?,?,?)");
        query.addBindValue(datasetId);
        query.addBindValue(normalizedImagePath);
        query.addBindValue(normalizedLabelPath);
        query.addBindValue(boxes.size());
        query.addBindValue(categoryNames.join(", "));
        query.addBindValue(now());
        query.addBindValue(now());
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
        imageId = query.lastInsertId().toInt();
    } else {
        QSqlQuery query(m_db);
        query.prepare("UPDATE images SET dataset_id=?, label_path=?, annotation_count=?, "
                      "category_summary=?, updated_at=? WHERE id=?");
        query.addBindValue(datasetId);
        query.addBindValue(normalizedLabelPath);
        query.addBindValue(boxes.size());
        query.addBindValue(categoryNames.join(", "));
        query.addBindValue(now());
        query.addBindValue(imageId);
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
    }

    {
        QSqlQuery query(m_db);
        query.prepare("DELETE FROM annotations WHERE image_id=?");
        query.addBindValue(imageId);
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
    }

    for (const AnnotationBox &box : boxes) {
        const QRectF rect = box.normalizedRect.normalized().intersected(QRectF(0, 0, 1, 1));
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO annotations "
                      "(image_id,class_id,class_name,cx,cy,width,height,created_at,updated_at) "
                      "VALUES (?,?,?,?,?,?,?,?,?)");
        query.addBindValue(imageId);
        query.addBindValue(box.classId);
        query.addBindValue(box.className);
        query.addBindValue(rect.x() + rect.width() / 2.0);
        query.addBindValue(rect.y() + rect.height() / 2.0);
        query.addBindValue(rect.width());
        query.addBindValue(rect.height());
        query.addBindValue(now());
        query.addBindValue(now());
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            return false;
        }
    }
    return true;
}

int DatabaseManager::insertTrainRecord(const TrainParams &params, const QString &status, const QString &modelPath)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO train_records "
                  "(dataset_path,model_type,epochs,batch_size,image_size,result_model_path,status,created_at) "
                  "VALUES (?,?,?,?,?,?,?,?)");
    query.addBindValue(params.datasetPath);
    query.addBindValue(params.modelType);
    query.addBindValue(params.epochs);
    query.addBindValue(params.batchSize);
    query.addBindValue(params.imageSize);
    query.addBindValue(modelPath);
    query.addBindValue(status);
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::updateTrainRecord(int id, const QString &status, const QString &modelPath)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE train_records SET status=?, result_model_path=? WHERE id=?");
    query.addBindValue(status);
    query.addBindValue(modelPath);
    query.addBindValue(id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

int DatabaseManager::insertTrainingRun(const TrainParams &params, const QString &status)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO training_runs "
                  "(dataset_path,train_path,val_path,model_type,epochs,batch_size,image_size,"
                  "learning_rate,output_dir,result_model_path,metrics_json,status,started_at,finished_at) "
                  "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
    query.addBindValue(params.datasetPath);
    query.addBindValue(params.trainPath);
    query.addBindValue(params.valPath);
    query.addBindValue(params.modelType);
    query.addBindValue(params.epochs);
    query.addBindValue(params.batchSize);
    query.addBindValue(params.imageSize);
    query.addBindValue(params.learningRate);
    query.addBindValue(params.outputDir);
    query.addBindValue(QString());
    query.addBindValue(QString());
    query.addBindValue(status);
    query.addBindValue(now());
    query.addBindValue(QString());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toInt();
}

bool DatabaseManager::finishTrainingRun(int id,
                                        const QString &status,
                                        const QString &modelPath,
                                        const QString &metricsJson)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE training_runs SET status=?, result_model_path=?, metrics_json=?, finished_at=? WHERE id=?");
    query.addBindValue(status);
    query.addBindValue(modelPath);
    query.addBindValue(metricsJson);
    query.addBindValue(now());
    query.addBindValue(id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::insertModelRecord(const QString &modelPath,
                                        const QString &modelType,
                                        const QString &quantizedPath,
                                        const QString &quantizationType)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO model_records "
                  "(model_path,model_type,quantized_path,quantization_type,created_at) "
                  "VALUES (?,?,?,?,?)");
    query.addBindValue(modelPath);
    query.addBindValue(modelType);
    query.addBindValue(quantizedPath);
    query.addBindValue(quantizationType);
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    insertModelFile(modelPath,
                    quantizedPath.isEmpty() ? modelPath : quantizedPath,
                    modelType,
                    quantizationType,
                    quantizationType.isEmpty() ? QString("ONNX Runtime export") : QString("ONNX Runtime"));
    return true;
}

bool DatabaseManager::insertModelFile(const QString &sourceModelPath,
                                      const QString &modelPath,
                                      const QString &modelType,
                                      const QString &quantizationType,
                                      const QString &backend)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO model_files "
                  "(source_model_path,model_path,model_type,quantization_type,backend,created_at) "
                  "VALUES (?,?,?,?,?,?)");
    query.addBindValue(sourceModelPath);
    query.addBindValue(modelPath);
    query.addBindValue(modelType);
    query.addBindValue(quantizationType);
    query.addBindValue(backend);
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

int DatabaseManager::insertInferenceRecord(const QString &imagePath,
                                           const QString &modelPath,
                                           const QString &resultImagePath,
                                           const QString &resultJsonPath,
                                           int detectedCount)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO inference_records "
                  "(image_path,model_path,result_image_path,result_json_path,detected_count,created_at) "
                  "VALUES (?,?,?,?,?,?)");
    query.addBindValue(imagePath);
    query.addBindValue(modelPath);
    query.addBindValue(resultImagePath);
    query.addBindValue(resultJsonPath);
    query.addBindValue(detectedCount);
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }
    const int id = query.lastInsertId().toInt();
    QSqlQuery mirror(m_db);
    mirror.prepare("INSERT INTO inference_results "
                   "(image_path,model_path,result_image_path,result_json_path,detected_count,backend,created_at) "
                   "VALUES (?,?,?,?,?,?,?)");
    mirror.addBindValue(imagePath);
    mirror.addBindValue(modelPath);
    mirror.addBindValue(resultImagePath);
    mirror.addBindValue(resultJsonPath);
    mirror.addBindValue(detectedCount);
    mirror.addBindValue("Python Ultralytics");
    mirror.addBindValue(now());
    if (!mirror.exec()) {
        m_lastError = mirror.lastError().text();
    }
    return id;
}

bool DatabaseManager::insertInferenceObject(int inferenceId, const InferenceObject &object)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO inference_objects "
                  "(inference_id,class_id,class_name,confidence,x1,y1,x2,y2) "
                  "VALUES (?,?,?,?,?,?,?,?)");
    query.addBindValue(inferenceId);
    query.addBindValue(object.classId);
    query.addBindValue(object.className);
    query.addBindValue(object.confidence);
    query.addBindValue(object.x1);
    query.addBindValue(object.y1);
    query.addBindValue(object.x2);
    query.addBindValue(object.y2);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

QList<QStringList> DatabaseManager::queryRows(const QString &tableName, const QStringList &columns, int limit) const
{
    QList<QStringList> rows;
    static const QStringList allowedTables = {
        "dataset_stats", "train_records", "model_records", "inference_records", "inference_objects",
        "datasets", "images", "categories", "annotations", "training_runs", "model_files", "inference_results"
    };
    if (!allowedTables.contains(tableName)) {
        return rows;
    }

    const QString sql = QString("SELECT %1 FROM %2 ORDER BY id DESC LIMIT %3")
                            .arg(columns.join(","), tableName)
                            .arg(limit);
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        return rows;
    }

    while (query.next()) {
        QStringList row;
        for (int i = 0; i < columns.size(); ++i) {
            row << query.value(i).toString();
        }
        rows << row;
    }
    return rows;
}

int DatabaseManager::totalInferenceCount() const
{
    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM inference_records") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QMap<QString, int> DatabaseManager::classDetectionCounts() const
{
    QMap<QString, int> counts;
    QSqlQuery query(m_db);
    if (!query.exec("SELECT class_name, COUNT(*) FROM inference_objects GROUP BY class_name ORDER BY COUNT(*) DESC")) {
        return counts;
    }
    while (query.next()) {
        counts.insert(query.value(0).toString(), query.value(1).toInt());
    }
    return counts;
}

QString DatabaseManager::latestTrainingModel() const
{
    QSqlQuery query(m_db);
    if (query.exec("SELECT result_model_path FROM train_records "
                   "WHERE result_model_path IS NOT NULL AND result_model_path<>'' "
                   "ORDER BY id DESC LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

QString DatabaseManager::latestInferenceTime() const
{
    QSqlQuery query(m_db);
    if (query.exec("SELECT created_at FROM inference_records ORDER BY id DESC LIMIT 1") && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

bool DatabaseManager::execSql(const QString &sql)
{
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

int DatabaseManager::ensureDatasetRecord(const QString &datasetPath, const QString &name)
{
    const QString normalizedPath = QDir::fromNativeSeparators(datasetPath);
    const QString datasetName = name.isEmpty() ? QFileInfo(normalizedPath).fileName() : name;

    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO datasets (name,path,created_at,updated_at) VALUES (?,?,?,?)");
    query.addBindValue(datasetName);
    query.addBindValue(normalizedPath);
    query.addBindValue(now());
    query.addBindValue(now());
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return -1;
    }

    QSqlQuery update(m_db);
    update.prepare("UPDATE datasets SET name=?, updated_at=? WHERE path=?");
    update.addBindValue(datasetName);
    update.addBindValue(now());
    update.addBindValue(normalizedPath);
    if (!update.exec()) {
        m_lastError = update.lastError().text();
        return -1;
    }

    QSqlQuery select(m_db);
    select.prepare("SELECT id FROM datasets WHERE path=?");
    select.addBindValue(normalizedPath);
    if (!select.exec() || !select.next()) {
        m_lastError = select.lastError().text();
        return -1;
    }
    return select.value(0).toInt();
}

QString DatabaseManager::now() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}
