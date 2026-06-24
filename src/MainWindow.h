#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "model/DatabaseManager.h"

#include <QMainWindow>

class AnnotationPage;
class DatasetPage;
class DeployPage;
class InferencePage;
class TrainingPage;
class VisualizationPage;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupChrome();
    void setupStyle();

private:
    DatabaseManager m_database;
    QTabWidget *m_tabs = nullptr;
    DatasetPage *m_datasetPage = nullptr;
    AnnotationPage *m_annotationPage = nullptr;
    TrainingPage *m_trainingPage = nullptr;
    DeployPage *m_deployPage = nullptr;
    InferencePage *m_inferencePage = nullptr;
    VisualizationPage *m_visualizationPage = nullptr;
};

#endif // MAINWINDOW_H
