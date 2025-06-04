#include "startwidget.h"
#include "../core/configmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QShowEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QApplication>
#include <QDir>

StartWidget::StartWidget(QWidget *parent)
    : QWidget(parent)
    , m_configManager(nullptr)
    , m_hasCheckpoint(false)
{
    setupUI();
    setupConnections();
    setupAnimations();
    applyStyles();
}

StartWidget::~StartWidget()
{
}

void StartWidget::setConfigManager(ConfigManager *configManager)
{
    m_configManager = configManager;
    updateButtonStates();
}

void StartWidget::setupUI()
{
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(50, 30, 50, 30);
    m_mainLayout->setSpacing(20);
    
    // Create title section
    m_titleLayout = new QVBoxLayout();
    m_titleLayout->setSpacing(10);
    
    // Logo section
    m_logoLayout = new QHBoxLayout();
    m_logoLabel = new QLabel();
    m_logoLabel->setFixedSize(64, 64);
    m_logoLabel->setScaledContents(true);
    
    // Try to load logo, create a simple one if not found
    QPixmap logo(64, 64);
    logo.fill(Qt::transparent);
    QPainter painter(&logo);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw a simple logo
    QLinearGradient gradient(0, 0, 64, 64);
    gradient.setColorAt(0, QColor("#4A90E2"));
    gradient.setColorAt(1, QColor("#357ABD"));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(8, 8, 48, 48, 8, 8);
    
    // Draw "P" for ProblemX
    painter.setPen(QPen(Qt::white, 3));
    QFont logoFont;
    logoFont.setFamily("Arial");
    logoFont.setPointSize(24);
    logoFont.setBold(true);
    painter.setFont(logoFont);
    painter.drawText(QRect(8, 8, 48, 48), Qt::AlignCenter, "P");
    
    m_logoLabel->setPixmap(logo);
    m_logoLayout->addStretch();
    m_logoLayout->addWidget(m_logoLabel);
    m_logoLayout->addStretch();
    
    // Title and subtitle
    m_titleLabel = new QLabel("ProblemX");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setObjectName("titleLabel");
    
    m_subtitleLabel = new QLabel("智能刷题系统");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setObjectName("subtitleLabel");
    
    m_titleLayout->addLayout(m_logoLayout);
    m_titleLayout->addWidget(m_titleLabel);
    m_titleLayout->addWidget(m_subtitleLabel);
    
    // Create button section
    m_buttonLayout = new QVBoxLayout();
    m_buttonLayout->setSpacing(15);
    
    // Create buttons
    m_startButton = new QPushButton("开始刷题");
    m_startButton->setObjectName("primaryButton");
    m_startButton->setMinimumHeight(50);
    
    m_resumeButton = new QPushButton("继续练习");
    m_resumeButton->setObjectName("secondaryButton");
    m_resumeButton->setMinimumHeight(45);
    m_resumeButton->setVisible(false);
    
    m_configButton = new QPushButton("配置程序");
    m_configButton->setObjectName("secondaryButton");
    m_configButton->setMinimumHeight(45);
    
    m_reviewButton = new QPushButton("错题复习");
    m_reviewButton->setObjectName("secondaryButton");
    m_reviewButton->setMinimumHeight(45);
    
    m_aboutButton = new QPushButton("关于程序");
    m_aboutButton->setObjectName("secondaryButton");
    m_aboutButton->setMinimumHeight(45);
    
    m_exitButton = new QPushButton("退出程序");
    m_exitButton->setObjectName("exitButton");
    m_exitButton->setMinimumHeight(45);
    
    // Add buttons to layout
    m_buttonLayout->addWidget(m_startButton);
    m_buttonLayout->addWidget(m_resumeButton);
    m_buttonLayout->addWidget(m_configButton);
    m_buttonLayout->addWidget(m_reviewButton);
    m_buttonLayout->addWidget(m_aboutButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_exitButton);
    
    // Add sections to main layout
    m_mainLayout->addStretch();
    m_mainLayout->addLayout(m_titleLayout);
    m_mainLayout->addStretch();
    m_mainLayout->addLayout(m_buttonLayout);
    m_mainLayout->addStretch();
    
    setLayout(m_mainLayout);
}

void StartWidget::setupConnections()
{
    connect(m_startButton, &QPushButton::clicked, this, &StartWidget::onStartPracticeClicked);
    connect(m_resumeButton, &QPushButton::clicked, this, &StartWidget::resumePracticeRequested);
    connect(m_configButton, &QPushButton::clicked, this, &StartWidget::onConfigureClicked);
    connect(m_reviewButton, &QPushButton::clicked, this, &StartWidget::onReviewWrongAnswersClicked);
    connect(m_aboutButton, &QPushButton::clicked, this, &StartWidget::onAboutClicked);
    connect(m_exitButton, &QPushButton::clicked, this, &StartWidget::onExitClicked);
}

void StartWidget::setupAnimations()
{
    // Setup fade-in animation
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);
    
    m_fadeInAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeInAnimation->setDuration(800);
    m_fadeInAnimation->setStartValue(0.0);
    m_fadeInAnimation->setEndValue(1.0);
}

void StartWidget::applyStyles()
{
    setStyleSheet(
        "StartWidget {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #f8f9fa, stop:1 #e9ecef);"
        "}"
        
        "#titleLabel {"
        "    font-family: 'Microsoft YaHei', 'Arial', sans-serif;"
        "    font-size: 48px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    margin: 10px 0;"
        "}"
        
        "#subtitleLabel {"
        "    font-family: 'Microsoft YaHei', 'Arial', sans-serif;"
        "    font-size: 18px;"
        "    color: #7f8c8d;"
        "    margin-bottom: 20px;"
        "}"
        
        "#primaryButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #4A90E2, stop:1 #357ABD);"
        "    border: none;"
        "    border-radius: 8px;"
        "    color: white;"
        "    font-family: 'Microsoft YaHei', 'Arial', sans-serif;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    padding: 12px 24px;"
        "    min-width: 200px;"
        "}"
        
        "#primaryButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #5BA0F2, stop:1 #4A8ACD);"
        "}"
        
        "#primaryButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #3A80D2, stop:1 #2A6AAD);"
        "}"
        
        "#secondaryButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #ffffff, stop:1 #f8f9fa);"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 8px;"
        "    color: #495057;"
        "    font-family: 'Microsoft YaHei', 'Arial', sans-serif;"
        "    font-size: 14px;"
        "    padding: 10px 20px;"
        "    min-width: 200px;"
        "}"
        
        "#secondaryButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #f8f9fa, stop:1 #e9ecef);"
        "    border-color: #adb5bd;"
        "}"
        
        "#secondaryButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #e9ecef, stop:1 #dee2e6);"
        "}"
        
        "#exitButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #dc3545, stop:1 #c82333);"
        "    border: none;"
        "    border-radius: 8px;"
        "    color: white;"
        "    font-family: 'Microsoft YaHei', 'Arial', sans-serif;"
        "    font-size: 14px;"
        "    padding: 10px 20px;"
        "    min-width: 200px;"
        "}"
        
        "#exitButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #e74c3c, stop:1 #d73527);"
        "}"
        
        "#exitButton:pressed {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
        "                                stop:0 #bd2130, stop:1 #a71e2a);"
        "}"
    );
}

void StartWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    checkForCheckpoint();
    m_fadeInAnimation->start();
}

void StartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background gradient
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(248, 249, 250));
    gradient.setColorAt(1, QColor(233, 236, 239));
    
    painter.fillRect(rect(), gradient);
}

void StartWidget::onStartPracticeClicked()
{
    if (!m_configManager) {
        QMessageBox::warning(this, "错误", "配置管理器未初始化");
        return;
    }
    
    // Check if there are any subjects configured
    QStringList subjects = m_configManager->getAvailableSubjects();
    if (subjects.isEmpty()) {
        QMessageBox::information(this, "提示", 
            "还没有配置任何科目，请先配置题库。");
        emit configureRequested();
        return;
    }
    
    // Check if any question banks are enabled
    bool hasEnabledBanks = false;
    for (const QString &subject : subjects) {
        QuestionBank bank = m_configManager->getQuestionBank(subject);
        if (bank.hasSelectedBanks()) {
            hasEnabledBanks = true;
            break;
        }
    }
    
    if (!hasEnabledBanks) {
        QMessageBox::information(this, "提示", 
            "没有启用的题库，请先在配置中启用题库。");
        emit configureRequested();
        return;
    }
    
    emit startPracticeRequested();
}

void StartWidget::onResumePracticeClicked()
{
    emit resumePracticeRequested();
}

void StartWidget::onConfigureClicked()
{
    emit configureRequested();
}

void StartWidget::onReviewWrongAnswersClicked()
{
    // Check if there are any wrong answer files
    QDir waDir("WA");
    if (!waDir.exists() || waDir.entryList(QStringList() << "*.json", QDir::Files).isEmpty()) {
        QMessageBox::information(this, "提示", 
            "还没有错题记录，请先进行练习。");
        return;
    }
    
    emit reviewWrongAnswersRequested();
}

void StartWidget::onAboutClicked()
{
    // 创建关于对话框
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("关于 ProblemX");
    aboutBox.setTextFormat(Qt::RichText);
    
    QString aboutText = QString(
        "<h2>ProblemX 智能刷题系统</h2>"
        "<p><b>版本:</b> Beta 0.3 </p>"
        "<p><b>作者:</b> Orange</p>"
        "<p><b>描述:</b> 一个基于Qt框架开发的智能刷题练习系统，支持多科目题库管理、错题复习等功能。</p>"
        "<hr>"
        "<p><b>技术栈:</b></p>"
        "<ul>"
        "<li>Qt %1 - 跨平台应用程序框架</li>"
        "<li>C++ - 核心开发语言</li>"
        "<li>JSON - 数据存储格式</li>"
        "</ul>"
        "<p><b>功能特性:</b></p>"
        "<ul>"
        "<li>多科目题库支持 (C/C++/Java/Python/数据结构)</li>"
        "<li>智能练习进度管理</li>"
        "<li>错题自动收集与复习</li>"
        "<li>灵活的配置管理</li>"
        "<li>支持latex和markdown渲染</li>"
        "</ul>"
        "<p><b>联系方式:</b> orangesummer.ovo@qq.com</p>"
    ).arg(QT_VERSION_STR);
    
    aboutBox.setText(aboutText);
    aboutBox.setIconPixmap(m_logoLabel->pixmap().scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // 添加关于Qt按钮
    QPushButton *aboutQtButton = aboutBox.addButton("关于 Qt", QMessageBox::ActionRole);
    aboutBox.addButton("确定", QMessageBox::AcceptRole);
    
    aboutBox.exec();
    
    // 如果点击了关于Qt按钮，显示Qt的关于对话框
    if (aboutBox.clickedButton() == aboutQtButton) {
        QMessageBox::aboutQt(this, "关于 Qt");
    }
}

void StartWidget::onExitClicked()
{
    int ret = QMessageBox::question(this, "确认退出", 
        "确定要退出 ProblemX 吗？",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        emit exitRequested();
    }
}

void StartWidget::checkForCheckpoint()
{
    if (m_configManager) {
        m_hasCheckpoint = m_configManager->hasCheckpoint();
        updateButtonStates();
    }
}

void StartWidget::updateButtonStates()
{
    if (m_configManager) {
        // Show/hide resume button based on checkpoint
        m_resumeButton->setVisible(m_hasCheckpoint);
        
        // Enable/disable review button based on wrong answer files
        QDir waDir("WA");
        bool hasWrongAnswers = waDir.exists() && 
                              !waDir.entryList(QStringList() << "*.json", QDir::Files).isEmpty();
        m_reviewButton->setEnabled(hasWrongAnswers);
        
        // Enable/disable start button based on configuration
        QStringList subjects = m_configManager->getAvailableSubjects();
        bool hasConfiguration = !subjects.isEmpty();
        m_startButton->setEnabled(hasConfiguration);
        
        if (!hasConfiguration) {
            m_startButton->setToolTip("请先配置题库");
        } else {
            m_startButton->setToolTip("");
        }
    }
}
