#include "VisualizationPage.h"

#include "../model/DatabaseManager.h"

#include <QHeaderView>
#include <QMap>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

VisualizationPage::VisualizationPage(DatabaseManager *database, QWidget *parent)
    : QWidget(parent),
      m_database(database)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *refreshButton = new QPushButton("刷新统计", this);
    m_summaryEdit = new QTextEdit(this);
    m_summaryEdit->setReadOnly(true);
    m_summaryEdit->setMaximumHeight(160);

    auto *tabs = new QTabWidget(this);
    m_trainTable = createTable({"id", "dataset_path", "model_type", "epochs", "batch_size", "image_size", "result_model_path", "status", "created_at"});
    m_trainingRunTable = createTable({"id", "dataset_path", "train_path", "val_path", "model_type", "epochs", "batch_size", "image_size", "learning_rate", "output_dir", "result_model_path", "metrics_json", "status", "started_at", "finished_at"});
    m_modelTable = createTable({"id", "model_path", "model_type", "quantized_path", "quantization_type", "created_at"});
    m_modelFileTable = createTable({"id", "source_model_path", "model_path", "model_type", "quantization_type", "backend", "created_at"});
    m_inferTable = createTable({"id", "image_path", "model_path", "result_image_path", "result_json_path", "detected_count", "created_at"});
    m_datasetTable = createTable({"id", "dataset_path", "image_count", "label_count", "class_count", "created_at"});
    m_imageTable = createTable({"id", "dataset_id", "file_path", "label_path", "annotation_count", "category_summary", "created_at", "updated_at"});
    m_categoryTable = createTable({"id", "dataset_id", "class_id", "name", "created_at"});
    m_annotationTable = createTable({"id", "image_id", "class_id", "class_name", "cx", "cy", "width", "height", "created_at", "updated_at"});
    tabs->addTab(m_trainTable, "训练记录");
    tabs->addTab(m_trainingRunTable, "训练指标");
    tabs->addTab(m_modelTable, "模型记录");
    tabs->addTab(m_modelFileTable, "模型文件");
    tabs->addTab(m_inferTable, "推理记录");
    tabs->addTab(m_datasetTable, "数据集统计");
    tabs->addTab(m_imageTable, "图像记录");
    tabs->addTab(m_categoryTable, "类别记录");
    tabs->addTab(m_annotationTable, "标注记录");

    layout->addWidget(refreshButton);
    layout->addWidget(m_summaryEdit);
    layout->addWidget(tabs, 1);

    connect(refreshButton, &QPushButton::clicked, this, &VisualizationPage::refresh);
    refresh();
}

void VisualizationPage::refresh()
{
    if (!m_database) {
        return;
    }

    QString summary;
    summary += QString("总推理次数：%1\n").arg(m_database->totalInferenceCount());
    summary += "最近一次训练模型路径：" + m_database->latestTrainingModel() + "\n";
    summary += "最近一次推理时间：" + m_database->latestInferenceTime() + "\n\n";
    summary += "各类别检测数量：\n";
    const QMap<QString, int> counts = m_database->classDetectionCounts();
    if (counts.isEmpty()) {
        summary += "暂无推理对象记录。\n";
    } else {
        for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
            summary += QString("%1：%2\n").arg(it.key()).arg(it.value());
        }
    }
    m_summaryEdit->setPlainText(summary);

    fillTable(m_trainTable, "train_records", {"id", "dataset_path", "model_type", "epochs", "batch_size", "image_size", "result_model_path", "status", "created_at"});
    fillTable(m_trainingRunTable, "training_runs", {"id", "dataset_path", "train_path", "val_path", "model_type", "epochs", "batch_size", "image_size", "learning_rate", "output_dir", "result_model_path", "metrics_json", "status", "started_at", "finished_at"});
    fillTable(m_modelTable, "model_records", {"id", "model_path", "model_type", "quantized_path", "quantization_type", "created_at"});
    fillTable(m_modelFileTable, "model_files", {"id", "source_model_path", "model_path", "model_type", "quantization_type", "backend", "created_at"});
    fillTable(m_inferTable, "inference_records", {"id", "image_path", "model_path", "result_image_path", "result_json_path", "detected_count", "created_at"});
    fillTable(m_datasetTable, "dataset_stats", {"id", "dataset_path", "image_count", "label_count", "class_count", "created_at"});
    fillTable(m_imageTable, "images", {"id", "dataset_id", "file_path", "label_path", "annotation_count", "category_summary", "created_at", "updated_at"});
    fillTable(m_categoryTable, "categories", {"id", "dataset_id", "class_id", "name", "created_at"});
    fillTable(m_annotationTable, "annotations", {"id", "image_id", "class_id", "class_name", "cx", "cy", "width", "height", "created_at", "updated_at"});
}

QTableWidget *VisualizationPage::createTable(const QStringList &headers)
{
    auto *table = new QTableWidget(0, headers.size(), this);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    return table;
}

void VisualizationPage::fillTable(QTableWidget *table, const QString &tableName, const QStringList &columns)
{
    table->setRowCount(0);
    const QList<QStringList> rows = m_database->queryRows(tableName, columns, 300);
    for (const QStringList &items : rows) {
        const int row = table->rowCount();
        table->insertRow(row);
        for (int col = 0; col < items.size(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(items.at(col)));
        }
    }
}
