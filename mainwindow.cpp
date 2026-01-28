#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/startwidget.h"
#include "widgets/configwidget.h"
#include "widgets/practicewidget.h"
#include "widgets/reviewwidget.h"
#include "widgets/questionassistantwidget.h"
#include "core/configmanager.h"
#include "core/practicemanager.h"
#include "models/question.h"
#include "core/wronganswerset.h"
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_stackedWidget(nullptr)
    , m_startWidget(nullptr)
    , m_configWidget(nullptr)
    , m_practiceWidget(nullptr)
    , m_reviewWidget(nullptr)
    , m_questionAssistantWidget(nullptr)
    , m_configManager(nullptr)
    , m_practiceManager(nullptr)
    , m_wrongAnswerSet(nullptr)
{
    ui->setupUi(this);
    
    // Set window properties
    setWindowTitle("ProblemX - 练习系统");
    setMinimumSize(1000, 700);
    resize(1400, 900);
    
    initializeManagers();
    setupUI();
    setupConnections();
    
    // Show start widget by default
    showStartWidget();
}

MainWindow::~MainWindow()
{
    delete ui;
    
    // Managers will be deleted automatically as they are children of widgets
}

void MainWindow::initializeManagers()
{
    // Create config manager
    m_configManager = new ConfigManager();
    
    // Create wrong answer set
    m_wrongAnswerSet = new WrongAnswerSet(this);
    m_wrongAnswerSet->loadFromFile("WA_SET.json");
    
    // Create practice manager
    m_practiceManager = new PracticeManager(this);
    m_practiceManager->setWrongAnswerSet(m_wrongAnswerSet);
    // PracticeManager doesn't need setConfigManager, it receives ConfigManager as parameter in methods
}

void MainWindow::setupUI()
{
    // Create central widget and layout
    m_centralWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Create stacked widget
    m_stackedWidget = new QStackedWidget();
    m_mainLayout->addWidget(m_stackedWidget);
    
    // Create widgets
    m_startWidget = new StartWidget();
    m_startWidget->setConfigManager(m_configManager);
    
    m_configWidget = new ConfigWidget();
    m_configWidget->setConfigManager(m_configManager);
    
    m_practiceWidget = new PracticeWidget();
    m_practiceWidget->setPracticeManager(m_practiceManager);
    
    m_reviewWidget = new ReviewWidget();
    m_reviewWidget->setConfigManager(m_configManager);
    m_reviewWidget->setPracticeManager(m_practiceManager);
    m_reviewWidget->setWrongAnswerSet(m_wrongAnswerSet);

    m_questionAssistantWidget = new QuestionAssistantWidget();
    m_questionAssistantWidget->setConfigManager(m_configManager);
    
    // Add widgets to stacked widget
    m_stackedWidget->addWidget(m_startWidget);
    m_stackedWidget->addWidget(m_configWidget);
    m_stackedWidget->addWidget(m_practiceWidget);
    m_stackedWidget->addWidget(m_reviewWidget);
    m_stackedWidget->addWidget(m_questionAssistantWidget);
    
    // Set central widget
    setCentralWidget(m_centralWidget);
}

void MainWindow::setupConnections()
{
    // Start widget connections
    connect(m_startWidget, &StartWidget::startPracticeRequested,
            this, &MainWindow::startPractice);
    connect(m_startWidget, &StartWidget::configureRequested, this, &MainWindow::showConfigWidget);
    connect(m_startWidget, &StartWidget::reviewWrongAnswersRequested, this, &MainWindow::showReviewWidget);
    connect(m_startWidget, &StartWidget::assistantRequested, this, &MainWindow::showQuestionAssistantWidget);
    connect(m_startWidget, &StartWidget::resumePracticeRequested,
            this, &MainWindow::resumePractice);
    connect(m_startWidget, &StartWidget::exitRequested,
            this, &MainWindow::close);
    
    // Config widget connections
    connect(m_configWidget, &ConfigWidget::backRequested,
            this, &MainWindow::showStartWidget);
    
    // Practice widget connections
    connect(m_practiceWidget, &PracticeWidget::practiceFinished,
            this, &MainWindow::onPracticeFinished);
    // practiceAborted signal doesn't exist, using backRequested instead
    connect(m_practiceWidget, &PracticeWidget::backRequested,
            this, &MainWindow::showStartWidget);
    connect(m_practiceWidget, &PracticeWidget::saveAndExitRequested,
            this, &MainWindow::onSaveAndExit);
    connect(m_practiceWidget, &PracticeWidget::practiceCompletedAndClearSave,
            this, &MainWindow::onPracticeCompletedAndClearSave);
    
    // Review widget connections
    connect(m_reviewWidget, &ReviewWidget::backRequested,
            this, &MainWindow::showStartWidget);
    connect(m_reviewWidget, &ReviewWidget::startReviewRequested,
            this, &MainWindow::startReview);

    connect(m_questionAssistantWidget, &QuestionAssistantWidget::backRequested,
            this, &MainWindow::showStartWidget);
    
    // Practice manager connections
    connect(m_practiceManager, &PracticeManager::practiceStarted,
            this, &MainWindow::showPracticeWidget);
}

void MainWindow::showStartWidget()
{
    m_stackedWidget->setCurrentWidget(m_startWidget);
    setWindowTitle("ProblemX - 练习系统");
}

void MainWindow::showConfigWidget()
{
    m_stackedWidget->setCurrentWidget(m_configWidget);
    setWindowTitle("ProblemX - 配置管理");
}

void MainWindow::showPracticeWidget()
{
    m_stackedWidget->setCurrentWidget(m_practiceWidget);
    setWindowTitle("ProblemX - 练习中");
}

void MainWindow::showReviewWidget()
{
    m_stackedWidget->setCurrentWidget(m_reviewWidget);
    setWindowTitle("ProblemX - 错题复习");
    
    // 自动加载错题数据
    if (m_reviewWidget) {
        m_reviewWidget->loadWrongAnswers();
    }
}

void MainWindow::showQuestionAssistantWidget()
{
    if (m_questionAssistantWidget) {
        m_questionAssistantWidget->setConfigManager(m_configManager);
        if (!m_questionAssistantWidget->prepareForShow()) {
            return;
        }
    }
    m_stackedWidget->setCurrentWidget(m_questionAssistantWidget);
    setWindowTitle("ProblemX - 题目助手");
}

void MainWindow::startPractice()
{
    if (!m_configManager) {
        QMessageBox::warning(this, "错误", "配置管理器未初始化");
        return;
    }
    
    if (!m_practiceManager) {
        QMessageBox::warning(this, "错误", "练习管理器未初始化");
        return;
    }
    
    // Check if there are any subjects configured
    QStringList subjects = m_configManager->getSubjects();
    if (subjects.isEmpty()) {
        QMessageBox::information(this, "提示", "请先配置科目和题库");
        showConfigWidget();
        return;
    }
    
    // 检测是否存在存档
    if (m_configManager->hasCheckpoint()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("检测到存档");
        msgBox.setText("检测到上次未完成的练习进度。");
        msgBox.setInformativeText("是否要从上次的进度继续练习？");
        
        QPushButton *continueButton = msgBox.addButton("继续上次练习", QMessageBox::YesRole);
        QPushButton *newButton = msgBox.addButton("开始新练习", QMessageBox::NoRole);
        QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);
        msgBox.setDefaultButton(continueButton);
        
        msgBox.exec();
        QPushButton *clickedButton = qobject_cast<QPushButton*>(msgBox.clickedButton());
        
        if (clickedButton == continueButton) {
            // 继续上次练习
            resumePractice();
            return;
        } else if (clickedButton == newButton) {
            // 开始新练习，删除存档
            m_configManager->clearCheckpoint();
            m_configManager->saveConfig();
            // 更新开始界面按钮状态
            if (m_startWidget) {
                m_startWidget->checkForCheckpoint();
            }
        } else {
            // 取消，什么都不做
            return;
        }
    }
    
    // Start new practice session
    QString currentSubject = m_configManager->getCurrentSubject();
    qDebug() << "Starting practice with current subject:" << currentSubject;
    
    // 如果当前科目为空，尝试使用第一个可用科目
    if (currentSubject.isEmpty()) {
        QStringList availableSubjects = m_configManager->getAvailableSubjects();
        if (!availableSubjects.isEmpty()) {
            currentSubject = availableSubjects.first();
            m_configManager->setCurrentSubject(currentSubject);
            qDebug() << "Current subject was empty, using first available:" << currentSubject;
        } else {
            QMessageBox::information(this, "提示", "没有可用的科目，请先配置科目和题库");
            showConfigWidget();
            return;
        }
    }
    
    if (m_practiceManager->startNewPractice(currentSubject, m_configManager)) {
        showPracticeWidget();
        // 启动练习界面显示
        m_practiceWidget->startPractice();
    } else {
        QMessageBox::warning(this, "错误", 
            QString("无法开始练习，请检查题库配置\n\n科目: %1\n请确保该科目有已启用的题库。")
            .arg(currentSubject));
    }
}

void MainWindow::startReview(const QList<Question> &questions)
{
    if (!m_practiceManager) {
        QMessageBox::warning(this, "错误", "练习管理器未初始化");
        return;
    }
    
    if (questions.isEmpty()) {
        QMessageBox::information(this, "提示", "没有选择要复习的题目");
        return;
    }
    
    // Start review session
    if (m_practiceManager->startReviewPractice(questions)) {
        showPracticeWidget();
        // 启动练习界面显示
        m_practiceWidget->startPractice();
    } else {
        QMessageBox::warning(this, "错误", "无法开始复习");
    }
}

void MainWindow::resumePractice()
{
    if (!m_practiceManager) {
        QMessageBox::warning(this, "错误", "练习管理器未初始化");
        return;
    }
    
    // Resume practice session
    if (m_practiceManager->resumePractice(m_configManager)) {
        showPracticeWidget();
        // 启动练习界面显示
        m_practiceWidget->startPractice();
    } else {
        QMessageBox::warning(this, "错误", "无法恢复练习，可能没有保存的进度");
    }
}

void MainWindow::onPracticeFinished()
{
    // Practice completed, return to start screen
    showStartWidget();
    
    // Show completion message
    QMessageBox::information(this, "练习完成", "恭喜！您已完成本次练习。");
}

void MainWindow::onSaveAndExit()
{
    if (!m_practiceManager || !m_configManager) {
        QMessageBox::warning(this, "错误", "管理器未初始化");
        return;
    }
    
    // 保存练习进度
    m_practiceManager->savePractice(m_configManager);
    
    // 返回开始界面
    showStartWidget();
    
    // 显示保存成功消息
    QMessageBox::information(this, "保存成功", "练习进度已保存，下次可以选择继续练习。");
}

void MainWindow::onPracticeCompletedAndClearSave()
{
    if (!m_configManager) {
        return;
    }
    
    // 清除存档
    m_configManager->clearCheckpoint();
    m_configManager->saveConfig();
    
    // 更新开始界面的按钮状态
    if (m_startWidget) {
        m_startWidget->checkForCheckpoint();
    }
}

// onPracticeAborted method removed as practiceAborted signal doesn't exist
