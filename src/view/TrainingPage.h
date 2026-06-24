#ifndef TRAININGPAGE_H
#define TRAININGPAGE_H

#include "../model/DataTypes.h"

#include <QList>
#include <QPair>
#include <QWidget>

class DatabaseManager;
class TrainingViewModel;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class TrainingPage : public QWidget
{
    Q_OBJECT

public:
    explicit TrainingPage(DatabaseManager *database, QWidget *parent = nullptr);

private slots:
    void chooseDataset();
    void chooseTrainPath();
    void chooseValPath();
    void chooseOutputDir();
    void choosePython();
    void chooseScript();
    void startTraining();
    void onTrainingFinished(bool success, int exitCode, const QString &modelPath, const QString &metricsJson);

private:
    void appendLog(const QString &message);
    void showMetrics(const QString &metricsJson);
    void showMetricBars(const QList<QPair<QString, double>> &bars);

private:
    DatabaseManager *m_database = nullptr;
    TrainingViewModel *m_viewModel = nullptr;
    QLineEdit *m_datasetEdit = nullptr;
    QLineEdit *m_trainEdit = nullptr;
    QLineEdit *m_valEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
    QSpinBox *m_epochsSpin = nullptr;
    QSpinBox *m_batchSpin = nullptr;
    QSpinBox *m_imgszSpin = nullptr;
    QDoubleSpinBox *m_lrSpin = nullptr;
    QLineEdit *m_deviceEdit = nullptr;
    QLineEdit *m_outputEdit = nullptr;
    QLineEdit *m_pythonEdit = nullptr;
    QLineEdit *m_scriptEdit = nullptr;
    QTableWidget *m_metricsTable = nullptr;
    QLabel *m_metricChartLabel = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    int m_recordId = -1;
    int m_trainingRunId = -1;
    TrainParams m_currentParams;
};

#endif // TRAININGPAGE_H
