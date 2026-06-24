#include "MainWindow.h"

#include "utils/AppConfig.h"
#include "view/AnnotationPage.h"
#include "view/DatasetPage.h"
#include "view/DeployPage.h"
#include "view/InferencePage.h"
#include "view/TrainingPage.h"
#include "view/VisualizationPage.h"

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1500, 920);
    setWindowTitle("车辆识别图形界面应用程序");

    if (!m_database.open(AppConfig::databasePath()) || !m_database.initialize()) {
        QMessageBox::warning(this, "数据库初始化失败", m_database.lastError());
    }

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto *title = new QLabel("车辆识别/车辆检测桌面应用", central);
    title->setObjectName("TitleLabel");
    layout->addWidget(title);

    m_tabs = new QTabWidget(central);
    m_datasetPage = new DatasetPage(&m_database, m_tabs);
    m_annotationPage = new AnnotationPage(&m_database, m_tabs);
    m_trainingPage = new TrainingPage(&m_database, m_tabs);
    m_deployPage = new DeployPage(&m_database, m_tabs);
    m_inferencePage = new InferencePage(&m_database, m_tabs);
    m_visualizationPage = new VisualizationPage(&m_database, m_tabs);

    m_tabs->addTab(m_datasetPage, "数据集/统计");
    m_tabs->addTab(m_annotationPage, "标注");
    m_tabs->addTab(m_trainingPage, "训练");
    m_tabs->addTab(m_deployPage, "模型部署/量化");
    m_tabs->addTab(m_inferencePage, "图像推理");
    m_tabs->addTab(m_visualizationPage, "结果可视化");
    layout->addWidget(m_tabs, 1);
    setCentralWidget(central);

    connect(m_datasetPage, &DatasetPage::classNamesUpdated,
            m_annotationPage, &AnnotationPage::setClassNames);
    m_annotationPage->setClassNames(m_datasetPage->classNames());
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (m_tabs->widget(index) == m_visualizationPage) {
            m_visualizationPage->refresh();
        }
    });

    setupChrome();
    setupStyle();
}

void MainWindow::setupChrome()
{
    auto *fileMenu = menuBar()->addMenu("文件");
    auto *viewMenu = menuBar()->addMenu("视图");
    auto *helpMenu = menuBar()->addMenu("帮助");

    auto addPageAction = [this, viewMenu](const QString &text, QWidget *page) {
        auto *action = new QAction(text, this);
        action->setToolTip("切换到" + text + "页");
        connect(action, &QAction::triggered, this, [this, page, text]() {
            const int index = m_tabs->indexOf(page);
            if (index >= 0) {
                m_tabs->setCurrentIndex(index);
                statusBar()->showMessage("当前页面：" + text, 3000);
            }
        });
        viewMenu->addAction(action);
        return action;
    };

    addPageAction("数据集", m_datasetPage);
    addPageAction("标注", m_annotationPage);
    addPageAction("训练", m_trainingPage);
    addPageAction("量化", m_deployPage);
    addPageAction("推理", m_inferencePage);
    addPageAction("结果", m_visualizationPage);

    auto *refreshAction = new QAction("刷新结果", this);
    refreshAction->setToolTip("刷新结果可视化数据");
    connect(refreshAction, &QAction::triggered, this, [this]() {
        m_visualizationPage->refresh();
        m_tabs->setCurrentWidget(m_visualizationPage);
        statusBar()->showMessage("结果数据已刷新", 3000);
    });
    viewMenu->addAction(refreshAction);

    auto *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAction);

    auto *aboutAction = new QAction("关于", this);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this,
                                 "关于 CarDet",
                                 "CarDet 是 Qt/C++ 桌面应用，使用 SQLite 保存数据，"
                                 "通过 QProcess 调用 Python 完成 YOLO 训练、推理和 ONNX 量化。");
    });
    helpMenu->addAction(aboutAction);

    statusBar()->showMessage("就绪：请选择数据集、标注、训练、量化或推理。");
}

void MainWindow::setupStyle()
{
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #eef1f5;
            color: #1f2937;
            font-family: "Microsoft YaHei";
            font-size: 14px;
        }
        QLabel#TitleLabel {
            font-size: 22px;
            font-weight: 700;
            padding: 8px 4px;
            color: #172033;
        }
        QTabWidget::pane {
            border: 1px solid #d6dce6;
            background: #f7f9fc;
        }
        QTabBar::tab {
            background: #dde5ef;
            padding: 10px 18px;
            margin-right: 4px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected {
            background: #ffffff;
            color: #0f62fe;
            font-weight: 700;
        }
        QPushButton {
            background: #1f6feb;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 7px 12px;
        }
        QPushButton:disabled {
            background: #aab5c4;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit, QTableWidget, QListWidget {
            background: white;
            border: 1px solid #cfd7e3;
            border-radius: 5px;
            padding: 4px;
        }
        QSpinBox, QDoubleSpinBox {
            padding: 4px 8px;
            min-height: 24px;
        }
        QGroupBox {
            background: #f7f9fc;
            border: 1px solid #d6dce6;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 12px;
            font-weight: 700;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QHeaderView::section {
            background: #e9eef6;
            border: none;
            padding: 6px;
            font-weight: 700;
        }
    )");
}
