#ifndef DEPLOYPAGE_H
#define DEPLOYPAGE_H

#include <QWidget>

class DatabaseManager;
class DeployViewModel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class DeployPage : public QWidget
{
    Q_OBJECT

public:
    explicit DeployPage(DatabaseManager *database, QWidget *parent = nullptr);

private slots:
    void choosePython();
    void chooseWeights();
    void chooseOnnx();
    void chooseQuantOutput();
    void exportOnnx();
    void quantizeOnnx();
    void onTaskFinished(bool success, int exitCode, const QString &outputPath);

private:
    void appendLog(const QString &message);

private:
    DatabaseManager *m_database = nullptr;
    DeployViewModel *m_viewModel = nullptr;
    QLineEdit *m_pythonEdit = nullptr;
    QLineEdit *m_weightsEdit = nullptr;
    QLineEdit *m_onnxOutputEdit = nullptr;
    QLineEdit *m_quantOutputEdit = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QPushButton *m_exportButton = nullptr;
    QPushButton *m_quantButton = nullptr;
    QString m_currentTask;
};

#endif // DEPLOYPAGE_H
