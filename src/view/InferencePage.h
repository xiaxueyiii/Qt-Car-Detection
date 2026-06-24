#ifndef INFERENCEPAGE_H
#define INFERENCEPAGE_H

#include "../model/DataTypes.h"

#include <QWidget>

class DatabaseManager;
class InferenceViewModel;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;

class InferencePage : public QWidget
{
    Q_OBJECT

public:
    explicit InferencePage(DatabaseManager *database, QWidget *parent = nullptr);

private slots:
    void chooseImage();
    void chooseModel();
    void chooseOutputDir();
    void startInference();
    void onInferenceFinished(bool success, int exitCode, const QString &resultImagePath, const QString &resultJsonPath);
    void saveResultImage();

private:
    QString findDefaultImage() const;
    QString findDefaultModel() const;
    void showImage(QLabel *label, const QString &path);
    QVector<InferenceObject> loadResults(const QString &jsonPath);
    void appendLog(const QString &message);

private:
    DatabaseManager *m_database = nullptr;
    InferenceViewModel *m_viewModel = nullptr;
    QLineEdit *m_imageEdit = nullptr;
    QLineEdit *m_modelEdit = nullptr;
    QLineEdit *m_outputEdit = nullptr;
    QLineEdit *m_deviceEdit = nullptr;
    QCheckBox *m_cppOnnxCheck = nullptr;
    QDoubleSpinBox *m_confSpin = nullptr;
    QLabel *m_originalLabel = nullptr;
    QLabel *m_resultLabel = nullptr;
    QTableWidget *m_resultTable = nullptr;
    QPlainTextEdit *m_logEdit = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QString m_lastResultImage;
    QString m_lastResultJson;
};

#endif // INFERENCEPAGE_H
