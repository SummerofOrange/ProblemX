#include "practicewidget.h"
#include "../core/practicemanager.h"
#include "../models/question.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QProgressBar>
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QButtonGroup>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStackedWidget>
#include <QFrame>
#include <QKeyEvent>
#include <QShortcut>
#include <QShowEvent>
#include <QMessageBox>
#include <QPixmap>
#include <QTextDocument>
#include <QApplication>
#include <QDateTime>
#include <QDebug>

PracticeWidget::PracticeWidget(QWidget *parent)
    : QWidget(parent)
    , m_practiceManager(nullptr)
    , m_currentQuestionIndex(-1)
    , m_totalQuestions(0)
    , m_isAnswerSubmitted(false)
    , m_isPaused(false)
    , m_currentQuestionType(QuestionType::Choice)
{
    setupUI();
    setupConnections();
    setupShortcuts();
    applyStyles();
    
    // Setup timer for elapsed time display
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &PracticeWidget::updateTimer);
    m_timer->start(1000); // Update every second
}

PracticeWidget::~PracticeWidget()
{
}

void PracticeWidget::setPracticeManager(PracticeManager *practiceManager)
{
    if (m_practiceManager) {
        disconnect(m_practiceManager, nullptr, this, nullptr);
    }
    
    m_practiceManager = practiceManager;
    
    if (m_practiceManager) {
        connect(m_practiceManager, &PracticeManager::questionChanged,
                this, &PracticeWidget::onQuestionChanged);
        connect(m_practiceManager, &PracticeManager::answerSubmitted,
                this, &PracticeWidget::onAnswerSubmitted);
        connect(m_practiceManager, &PracticeManager::progressChanged,
                this, &PracticeWidget::onProgressChanged);
        connect(m_practiceManager, &PracticeManager::statisticsUpdated,
                this, &PracticeWidget::onStatisticsUpdated);
        connect(m_practiceManager, &PracticeManager::practiceCompleted,
                this, &PracticeWidget::onPracticeCompleted);
    }
}

void PracticeWidget::startPractice()
{
    if (m_practiceManager) {
        // 对于新练习，总是从第一题开始；对于继续练习，使用QuestionManager的当前索引
        if (m_practiceManager->getCurrentMode() == PracticeMode::Resume) {
            // 继续练习：使用QuestionManager中保存的当前索引
            m_currentQuestionIndex = m_practiceManager->getQuestionManager()->getCurrentIndex();
        } else {
            // 新练习或错题复习：总是从第一题开始
            m_currentQuestionIndex = 0;
            // 确保QuestionManager也重置到第一题
            m_practiceManager->getQuestionManager()->setCurrentIndex(0);
        }
        
        m_totalQuestions = m_practiceManager->getQuestionManager()->getQuestionCount();
        
        // 重置暂停状态
        m_isPaused = false;
        m_pauseButton->setText("暂停");
        
        updateQuestionDisplay();
        updateQuestionList();
        updateStatistics();
        m_timer->start(1000);
    }
}

void PracticeWidget::pausePractice()
{
    m_timer->stop();
    m_pauseButton->setText("继续");
    m_isPaused = true;
    emit practicePaused();
}

void PracticeWidget::resumePractice()
{
    m_timer->start(1000);
    m_pauseButton->setText("暂停");
    m_isPaused = false;
}

void PracticeWidget::setupUI()
{
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create question panel (left side)
    m_questionPanel = new QWidget();
    m_questionLayout = new QVBoxLayout(m_questionPanel);
    m_questionLayout->setContentsMargins(15, 15, 15, 15);
    m_questionLayout->setSpacing(15);
    
    // Question header
    m_headerLayout = new QHBoxLayout();
    m_questionNumberLabel = new QLabel("题目 1/1");
    m_questionNumberLabel->setObjectName("questionNumber");
    m_questionTypeLabel = new QLabel("选择题");
    m_questionTypeLabel->setObjectName("questionType");
    m_timerLabel = new QLabel("00:00");
    m_timerLabel->setObjectName("timer");
    m_pauseButton = new QPushButton("暂停");
    m_pauseButton->setObjectName("pauseButton");
    
    m_headerLayout->addWidget(m_questionNumberLabel);
    m_headerLayout->addWidget(m_questionTypeLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_timerLabel);
    m_headerLayout->addWidget(m_pauseButton);
    
    // Question content scroll area
    m_questionScrollArea = new QScrollArea();
    m_questionScrollArea->setWidgetResizable(true);
    m_questionScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_questionScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_questionContent = new QWidget();
    m_questionContentLayout = new QVBoxLayout(m_questionContent);
    m_questionContentLayout->setContentsMargins(10, 10, 10, 10);
    m_questionContentLayout->setSpacing(15);
    
    // Question text and image
    m_questionTextLabel = new QLabel();
    m_questionTextLabel->setWordWrap(true);
    m_questionTextLabel->setObjectName("questionText");
    m_questionTextLabel->setTextFormat(Qt::RichText);
    
    m_questionImageLabel = new QLabel();
    m_questionImageLabel->setAlignment(Qt::AlignCenter);
    m_questionImageLabel->setScaledContents(false);
    m_questionImageLabel->setVisible(false);
    
    m_questionContentLayout->addWidget(m_questionTextLabel);
    m_questionContentLayout->addWidget(m_questionImageLabel);
    
    // Answer input stack
    m_answerStack = new QStackedWidget();
    
    // Choice question widget
    m_choiceWidget = new QWidget();
    m_choiceLayout = new QVBoxLayout(m_choiceWidget);
    m_choiceButtonGroup = new QButtonGroup(this);
    
    // Multi-choice question widget
    m_multiChoiceWidget = new QWidget();
    m_multiChoiceLayout = new QVBoxLayout(m_multiChoiceWidget);
    
    // Fill blank question widget
    m_fillBlankWidget = new QWidget();
    m_fillBlankLayout = new QVBoxLayout(m_fillBlankWidget);
    
    m_answerStack->addWidget(m_choiceWidget);
    m_answerStack->addWidget(m_multiChoiceWidget);
    m_answerStack->addWidget(m_fillBlankWidget);
    
    m_questionContentLayout->addWidget(m_answerStack);
    m_questionContentLayout->addStretch();
    
    m_questionScrollArea->setWidget(m_questionContent);
    
    // Answer result display
    m_resultFrame = new QFrame();
    m_resultFrame->setObjectName("resultFrame");
    m_resultFrame->setVisible(false);
    m_resultLayout = new QHBoxLayout(m_resultFrame);
    
    m_resultIcon = new QLabel();
    m_resultIcon->setFixedSize(24, 24);
    m_resultText = new QLabel();
    m_resultText->setObjectName("resultText");
    m_correctAnswerLabel = new QLabel();
    m_correctAnswerLabel->setObjectName("correctAnswer");
    
    m_resultLayout->addWidget(m_resultIcon);
    m_resultLayout->addWidget(m_resultText);
    m_resultLayout->addWidget(m_correctAnswerLabel);
    m_resultLayout->addStretch();
    
    // Navigation buttons
    m_navigationLayout = new QHBoxLayout();
    m_previousButton = new QPushButton("上一题");
    m_submitButton = new QPushButton("提交答案");
    m_submitButton->setObjectName("submitButton");
    m_nextButton = new QPushButton("下一题");
    m_finishButton = new QPushButton("完成练习");
    m_finishButton->setObjectName("finishButton");
    
    m_navigationLayout->addWidget(m_previousButton);
    m_navigationLayout->addStretch();
    m_navigationLayout->addWidget(m_submitButton);
    m_navigationLayout->addWidget(m_nextButton);
    m_navigationLayout->addWidget(m_finishButton);
    
    // Add to question layout
    m_questionLayout->addLayout(m_headerLayout);
    m_questionLayout->addWidget(m_questionScrollArea, 1);
    m_questionLayout->addWidget(m_resultFrame);
    m_questionLayout->addLayout(m_navigationLayout);
    
    // Create side panel (right side)
    m_sidePanel = new QWidget();
    m_sideLayout = new QVBoxLayout(m_sidePanel);
    m_sideLayout->setContentsMargins(10, 15, 15, 15);
    m_sideLayout->setSpacing(15);
    
    // Statistics group
    m_statisticsGroup = new QGroupBox("练习统计");
    m_statisticsLayout = new QGridLayout(m_statisticsGroup);
    
    m_progressLabel = new QLabel("进度:");
    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat("%p%");
    
    m_answeredLabel = new QLabel("已答题: 0");
    m_correctLabel = new QLabel("正确: 0");
    m_wrongLabel = new QLabel("错误: 0");
    m_accuracyLabel = new QLabel("正确率: 0%");
    m_timeElapsedLabel = new QLabel("用时: 00:00");
    
    m_statisticsLayout->addWidget(m_progressLabel, 0, 0);
    m_statisticsLayout->addWidget(m_progressBar, 0, 1);
    m_statisticsLayout->addWidget(m_answeredLabel, 1, 0);
    m_statisticsLayout->addWidget(m_correctLabel, 1, 1);
    m_statisticsLayout->addWidget(m_wrongLabel, 2, 0);
    m_statisticsLayout->addWidget(m_accuracyLabel, 2, 1);
    m_statisticsLayout->addWidget(m_timeElapsedLabel, 3, 0, 1, 2);
    
    // Question list group
    m_questionListGroup = new QGroupBox("题目列表");
    m_questionListLayout = new QVBoxLayout(m_questionListGroup);
    
    m_questionListWidget = new QListWidget();
    m_questionListWidget->setMaximumHeight(300);
    
    m_questionListLayout->addWidget(m_questionListWidget);
    
    // Control buttons
    m_controlLayout = new QHBoxLayout();
    m_backButton = new QPushButton("返回");
    m_backButton->setObjectName("backButton");
    
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_backButton);
    
    // Add to side layout
    m_sideLayout->addWidget(m_statisticsGroup);
    m_sideLayout->addWidget(m_questionListGroup);
    m_sideLayout->addStretch();
    m_sideLayout->addLayout(m_controlLayout);
    
    // Add panels to splitter
    m_mainSplitter->addWidget(m_questionPanel);
    m_mainSplitter->addWidget(m_sidePanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
    
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_mainSplitter);
    
    setLayout(mainLayout);
}

void PracticeWidget::setupConnections()
{
    connect(m_previousButton, &QPushButton::clicked, this, &PracticeWidget::onPreviousClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &PracticeWidget::onNextClicked);
    connect(m_submitButton, &QPushButton::clicked, this, &PracticeWidget::onSubmitClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &PracticeWidget::onPauseClicked);
    connect(m_finishButton, &QPushButton::clicked, this, &PracticeWidget::onFinishClicked);
    connect(m_backButton, &QPushButton::clicked, this, &PracticeWidget::onBackClicked);
    
    connect(m_questionListWidget, &QListWidget::itemClicked,
            this, &PracticeWidget::onQuestionListItemClicked);
    
    connect(m_choiceButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &PracticeWidget::onChoiceSelected);
}

void PracticeWidget::setupShortcuts()
{
    m_shortcutA = new QShortcut(QKeySequence("A"), this);
    m_shortcutB = new QShortcut(QKeySequence("B"), this);
    m_shortcutC = new QShortcut(QKeySequence("C"), this);
    m_shortcutD = new QShortcut(QKeySequence("D"), this);
    m_shortcutT = new QShortcut(QKeySequence("T"), this);
    m_shortcutF = new QShortcut(QKeySequence("F"), this);
    m_shortcutEnter = new QShortcut(QKeySequence(Qt::Key_Return), this);
    m_shortcutLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    m_shortcutRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    m_shortcutSpace = new QShortcut(QKeySequence(Qt::Key_Space), this);
    m_shortcutShiftSpace = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Space), this);
    
    connect(m_shortcutA, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::Choice || m_currentQuestionType == QuestionType::TrueOrFalse) {
            if (m_choiceButtons.size() > 0) m_choiceButtons[0]->setChecked(true);
        }
    });
    
    connect(m_shortcutB, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::Choice || m_currentQuestionType == QuestionType::TrueOrFalse) {
            if (m_choiceButtons.size() > 1) m_choiceButtons[1]->setChecked(true);
        }
    });
    
    connect(m_shortcutC, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::Choice) {
            if (m_choiceButtons.size() > 2) m_choiceButtons[2]->setChecked(true);
        }
    });
    
    connect(m_shortcutD, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::Choice) {
            if (m_choiceButtons.size() > 3) m_choiceButtons[3]->setChecked(true);
        }
    });
    
    connect(m_shortcutT, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::TrueOrFalse) {
            if (m_choiceButtons.size() > 0) m_choiceButtons[0]->setChecked(true);
        }
    });
    
    connect(m_shortcutF, &QShortcut::activated, [this]() {
        if (m_currentQuestionType == QuestionType::TrueOrFalse) {
            if (m_choiceButtons.size() > 1) m_choiceButtons[1]->setChecked(true);
        }
    });
    
    connect(m_shortcutEnter, &QShortcut::activated, this, &PracticeWidget::onSubmitClicked);
    connect(m_shortcutLeft, &QShortcut::activated, this, &PracticeWidget::onPreviousClicked);
    connect(m_shortcutRight, &QShortcut::activated, this, &PracticeWidget::onNextClicked);
    
    // 新增快捷键：空格跳转到下一题
    connect(m_shortcutSpace, &QShortcut::activated, [this]() {
        // 检查是否为最后一题
        if (m_practiceManager && m_practiceManager->getQuestionManager()) {
            int currentIndex = m_practiceManager->getQuestionManager()->getCurrentIndex();
            int totalQuestions = m_practiceManager->getQuestionManager()->getQuestionCount();
            
            if (currentIndex < totalQuestions - 1) {
                // 不是最后一题，跳转到下一题
                onNextClicked();
            } else {
                // 是最后一题，可以选择完成练习或者不做任何操作
                qDebug() << "Already at the last question, cannot go to next.";
            }
        }
    });
    
    // 新增快捷键：Shift+空格跳转到上一题
    connect(m_shortcutShiftSpace, &QShortcut::activated, [this]() {
        // 检查是否为第一题
        if (m_practiceManager && m_practiceManager->getQuestionManager()) {
            int currentIndex = m_practiceManager->getQuestionManager()->getCurrentIndex();
            
            if (currentIndex > 0) {
                // 不是第一题，跳转到上一题
                onPreviousClicked();
            } else {
                // 是第一题，不做任何操作
                qDebug() << "Already at the first question, cannot go to previous.";
            }
        }
    });
}

void PracticeWidget::applyStyles()
{
    setStyleSheet(
        "PracticeWidget {"
        "    background-color: #f8f9fa;"
        "}"
        
        "#questionNumber {"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "}"
        
        "#questionType {"
        "    font-size: 14px;"
        "    color: #6c757d;"
        "    background-color: #e9ecef;"
        "    padding: 4px 8px;"
        "    border-radius: 4px;"
        "}"
        
        "#timer {"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #28a745;"
        "}"
        
        "#questionText {"
        "    font-size: 16px;"
        "    line-height: 1.6;"
        "    color: #2c3e50;"
        "    background-color: white;"
        "    padding: 15px;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "}"
        
        "QRadioButton, QCheckBox {"
        "    font-size: 14px;"
        "    padding: 8px;"
        "    background-color: white;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    margin: 2px 0;"
        "}"
        
        "QRadioButton:hover, QCheckBox:hover {"
        "    background-color: #f8f9fa;"
        "    border-color: #4A90E2;"
        "}"
        
        "QRadioButton:checked, QCheckBox:checked {"
        "    background-color: #e3f2fd;"
        "    border-color: #4A90E2;"
        "    font-weight: bold;"
        "}"
        
        "QLineEdit {"
        "    font-size: 14px;"
        "    padding: 8px;"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 6px;"
        "    background-color: white;"
        "}"
        
        "QLineEdit:focus {"
        "    border-color: #4A90E2;"
        "    outline: none;"
        "}"
        
        "#submitButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 20px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        
        "#submitButton:hover {"
        "    background-color: #218838;"
        "}"
        
        "#submitButton:disabled {"
        "    background-color: #6c757d;"
        "}"
        
        "#finishButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 20px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        
        "#finishButton:hover {"
        "    background-color: #c82333;"
        "}"
        
        "#pauseButton {"
        "    background-color: #ffc107;"
        "    color: #212529;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    font-size: 12px;"
        "}"
        
        "#pauseButton:hover {"
        "    background-color: #e0a800;"
        "}"
        
        "#backButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 13px;"
        "}"
        
        "#backButton:hover {"
        "    background-color: #5a6268;"
        "}"
        
        "#resultFrame {"
        "    background-color: #d4edda;"
        "    border: 1px solid #c3e6cb;"
        "    border-radius: 6px;"
        "    padding: 10px;"
        "}"
        
        "#resultFrame[correct=\"false\"] {"
        "    background-color: #f8d7da;"
        "    border-color: #f5c6cb;"
        "}"
        
        "#resultText {"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        
        "#correctAnswer {"
        "    font-size: 13px;"
        "    color: #6c757d;"
        "}"
        
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    background-color: white;"
        "}"
        
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    color: #495057;"
        "}"
        
        "QListWidget {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    alternate-background-color: #f8f9fa;"
        "}"
        
        "QListWidget::item {"
        "    padding: 6px;"
        "    border-bottom: 1px solid #e9ecef;"
        "}"
        
        "QListWidget::item:selected {"
        "    background-color: transparent;"
        "    color: inherit;"
        "    outline: none;"
        "}"
        
        "QListWidget::item:selected:focus {"
        "    background-color: transparent;"
        "    color: inherit;"
        "    outline: none;"
        "}"
        

        
        "QListWidget::item:hover {"
        "    opacity: 0.8;"
        "}"
        
        "QProgressBar {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    background-color: #f8f9fa;"
        "}"
        
        "QProgressBar::chunk {"
        "    background-color: #28a745;"
        "    border-radius: 3px;"
        "}"
        
        "QScrollArea {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 8px;"
        "    background-color: white;"
        "}"
    );
}

void PracticeWidget::keyPressEvent(QKeyEvent *event)
{
    // Handle key events that aren't covered by shortcuts
    QWidget::keyPressEvent(event);
}

void PracticeWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateQuestionDisplay();
    updateQuestionList();
    updateStatistics();
}

void PracticeWidget::onQuestionChanged(int index, int total)
{
    m_currentQuestionIndex = index;
    m_totalQuestions = total;
    
    updateQuestionDisplay();
    updateQuestionList();
    updateNavigationButtons();
    
    // 不再在这里隐藏结果框架，让updateQuestionDisplay方法来处理结果显示
    // 这样切换到已作答的题目时可以显示判题结果
}

void PracticeWidget::onAnswerSubmitted(int index, bool correct)
{
    Q_UNUSED(index)
    
    qDebug() << "onAnswerSubmitted called: index=" << index << ", correct=" << correct;
    
    m_isAnswerSubmitted = true;
    
    // Get correct answer(s) and show result
    if (m_practiceManager) {
        Question currentQuestion = m_practiceManager->getQuestionManager()->getCurrentQuestion();
        qDebug() << "Question type:" << static_cast<int>(currentQuestion.getType());
        
        if (currentQuestion.getType() == QuestionType::FillBlank) {
            QStringList correctAnswers = currentQuestion.getAnswers();
            qDebug() << "Fill blank correct answers:" << correctAnswers;
            showAnswerResult(correct, correctAnswers);
        } else if (currentQuestion.getType() == QuestionType::TrueOrFalse) {
            // 对于判断题，将T/F转换为用户友好的显示文本
            QString correctAnswer = currentQuestion.getSingleAnswer();
            QString displayAnswer = (correctAnswer == "T") ? "正确" : "错误";
            qDebug() << "TrueOrFalse correct answer:" << correctAnswer << "-> display:" << displayAnswer;
            showAnswerResult(correct, displayAnswer);
        } else {
            QString correctAnswer = currentQuestion.getSingleAnswer();
            qDebug() << "Single correct answer:" << correctAnswer;
            showAnswerResult(correct, correctAnswer);
        }
    }
    
    setAnswerInputsEnabled(false);
    updateNavigationButtons();
    updateQuestionList();
}

void PracticeWidget::onProgressChanged(double progress)
{
    m_progressBar->setValue(static_cast<int>(progress));
}

void PracticeWidget::onStatisticsUpdated(int correct, int wrong, double accuracy)
{
    m_correctLabel->setText(QString("正确: %1").arg(correct));
    m_wrongLabel->setText(QString("错误: %1").arg(wrong));
    m_accuracyLabel->setText(QString("正确率: %1%").arg(QString::number(accuracy, 'f', 1)));
    m_answeredLabel->setText(QString("已答题: %1").arg(correct + wrong));
}

void PracticeWidget::onPracticeCompleted()
{
    m_timer->stop();
    
    if (m_practiceManager) {
        PracticeSession session = m_practiceManager->getCurrentSession();
        
        QString message = QString(
            "练习完成！\n\n"
            "总题数: %1\n"
            "正确: %2\n"
            "错误: %3\n"
            "正确率: %4%\n"
            "用时: %5"
        ).arg(session.totalQuestions)
         .arg(session.correctAnswers)
         .arg(session.wrongAnswers)
         .arg(QString::number(session.accuracy, 'f', 1))
         .arg(m_practiceManager->getElapsedTimeString());
        
        QMessageBox::information(this, "练习完成", message);
    }
    
    emit practiceFinished();
}

void PracticeWidget::onPreviousClicked()
{
    if (m_practiceManager && m_practiceManager->canGoToPrevious()) {
        m_practiceManager->goToPrevious();
    }
}

void PracticeWidget::onNextClicked()
{
    if (m_practiceManager && m_practiceManager->canGoToNext()) {
        m_practiceManager->goToNext();
    }
}

void PracticeWidget::onSubmitClicked()
{
    if (!m_practiceManager || m_isAnswerSubmitted) {
        return;
    }
    
    if (m_currentQuestionType == QuestionType::FillBlank) {
        QStringList answers = getCurrentAnswers();
        if (answers.isEmpty() || answers.join("").trimmed().isEmpty()) {
            QMessageBox::information(this, "提示", "请填写答案");
            return;
        }
        m_practiceManager->submitAnswers(answers);
    } else {
        QString answer = getCurrentAnswer();
        if (answer.isEmpty()) {
            QMessageBox::information(this, "提示", "请选择答案");
            return;
        }
        m_practiceManager->submitAnswer(answer);
    }
}

void PracticeWidget::onPauseClicked()
{
    if (!m_practiceManager) {
        return;
    }
    
    if (m_isPaused) {
        // 当前是暂停状态，点击后恢复
        m_timer->start(1000);
        m_pauseButton->setText("暂停");
        m_isPaused = false;
        
        // 调用PracticeManager的恢复方法
        m_practiceManager->resumePractice();
    } else {
        // 当前是运行状态，点击后暂停
        m_timer->stop();
        m_pauseButton->setText("继续");
        m_isPaused = true;
        
        // 调用PracticeManager的暂停方法
        m_practiceManager->pausePractice();
        emit practicePaused();
    }
}

void PracticeWidget::onFinishClicked()
{
    if (confirmFinish()) {
        if (m_practiceManager) {
            m_practiceManager->completePractice();
            // 完成练习时发射信号，让主窗口清除存档
            emit practiceCompletedAndClearSave();
        }
    }
}

void PracticeWidget::onQuestionListItemClicked(QListWidgetItem *item)
{
    if (!item || !m_practiceManager) {
        return;
    }
    
    int questionIndex = item->data(Qt::UserRole).toInt();
    
    // 如果点击的是当前题目，不需要切换
    if (questionIndex == m_currentQuestionIndex) {
        return;
    }
    
    m_practiceManager->goToQuestion(questionIndex);
    
    // goToQuestion会触发questionChanged信号，进而调用onQuestionChanged
    // onQuestionChanged中会调用updateQuestionList，所以这里不需要额外调用
}

void PracticeWidget::onChoiceSelected()
{
    // Enable submit button when choice is selected
    m_submitButton->setEnabled(!m_isAnswerSubmitted);
}

void PracticeWidget::onFillBlankChanged()
{
    // Enable submit button when fill blank has content
    bool hasContent = false;
    for (QLineEdit *edit : m_fillBlankEdits) {
        if (!edit->text().trimmed().isEmpty()) {
            hasContent = true;
            break;
        }
    }
    m_submitButton->setEnabled(hasContent && !m_isAnswerSubmitted);
}

void PracticeWidget::updateTimer()
{
    if (m_practiceManager) {
        QString timeStr = m_practiceManager->getElapsedTimeString();
        m_timerLabel->setText(timeStr);
        m_timeElapsedLabel->setText(QString("用时: %1").arg(timeStr));
    }
}

void PracticeWidget::updateQuestionDisplay()
{
    if (!m_practiceManager || m_currentQuestionIndex < 0) {
        return;
    }
    
    Question currentQuestion = m_practiceManager->getQuestionManager()->getCurrentQuestion();
    m_currentQuestionType = currentQuestion.getType();
    
    // Update header
    m_questionNumberLabel->setText(QString("题目 %1/%2")
                                  .arg(m_currentQuestionIndex + 1)
                                  .arg(m_totalQuestions));
    
    QString typeStr;
    switch (m_currentQuestionType) {
        case QuestionType::Choice:
            typeStr = "选择题";
            break;
        case QuestionType::TrueOrFalse:
            typeStr = "判断题";
            break;
        case QuestionType::FillBlank:
            typeStr = "填空题";
            break;
        case QuestionType::MultipleChoice:
            typeStr = "多选题";
            break;
    }
    m_questionTypeLabel->setText(typeStr);
    
    // Update question text (support markdown-like formatting)
    QString questionText = currentQuestion.getQuestion();
    // 先转义HTML特殊字符，防止代码中的<、>等符号被误解为HTML标签
    questionText.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
    // Simple markdown to HTML conversion
    questionText.replace("**", "<b>").replace("**", "</b>");
    questionText.replace("*", "<i>").replace("*", "</i>");
    questionText.replace("\n", "<br>");
    m_questionTextLabel->setText(questionText);
    
    // Update question image
    QString imagePath = currentQuestion.getImagePath();
    if (!imagePath.isEmpty()) {
        QString fullImagePath;
        
        // 检查是否为绝对路径
        if (QDir::isAbsolutePath(imagePath)) {
            fullImagePath = imagePath;
        } else {
            // 相对路径：基于当前科目路径和题型解析
            if (m_practiceManager) {
                QString subjectPath = m_practiceManager->getCurrentSubjectPath();
                if (!subjectPath.isEmpty()) {
                    // 获取题型信息
                    QString questionType;
                    switch (currentQuestion.getType()) {
                        case QuestionType::Choice:
                            questionType = "Choice";
                            break;
                        case QuestionType::TrueOrFalse:
                            questionType = "TrueOrFalse";
                            break;
                        case QuestionType::FillBlank:
                            questionType = "FillBlank";
                            break;
                        default:
                            questionType = "Choice"; // 默认值
                            break;
                    }
                    
                    // 构建完整路径：科目路径/题型/图片路径
                    QDir subjectDir(subjectPath);
                    fullImagePath = subjectDir.filePath(questionType + "/" + imagePath);
                } else {
                    // 如果没有科目路径，尝试基于应用程序目录
                    QString questionType;
                    switch (currentQuestion.getType()) {
                        case QuestionType::Choice:
                            questionType = "Choice";
                            break;
                        case QuestionType::TrueOrFalse:
                            questionType = "TrueOrFalse";
                            break;
                        case QuestionType::FillBlank:
                            questionType = "FillBlank";
                            break;
                        default:
                            questionType = "Choice";
                            break;
                    }
                    
                    QDir appDir(QApplication::applicationDirPath());
                    QString currentSubject = m_practiceManager->getCurrentSession().subject;
                    fullImagePath = appDir.filePath("Subject/" + currentSubject + "/" + questionType + "/" + imagePath);
                }
            } else {
                fullImagePath = imagePath;
            }
        }
        
        qDebug() << "Loading image from:" << fullImagePath;
        
        if (QFile::exists(fullImagePath)) {
            QPixmap pixmap(fullImagePath);
            if (!pixmap.isNull()) {
                // Scale image to fit while maintaining aspect ratio
                QPixmap scaledPixmap = pixmap.scaled(400, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_questionImageLabel->setPixmap(scaledPixmap);
                m_questionImageLabel->setVisible(true);
            } else {
                qDebug() << "Failed to load image:" << fullImagePath;
                m_questionImageLabel->setVisible(false);
            }
        } else {
            qDebug() << "Image file does not exist:" << fullImagePath;
            m_questionImageLabel->setVisible(false);
        }
    } else {
        m_questionImageLabel->setVisible(false);
    }
    
    // Display question based on type
    clearAnswerInputs();
    switch (m_currentQuestionType) {
        case QuestionType::Choice:
            displayChoiceQuestion(currentQuestion);
            m_answerStack->setCurrentWidget(m_choiceWidget);
            break;
        case QuestionType::TrueOrFalse:
            displayTrueOrFalseQuestion(currentQuestion);
            m_answerStack->setCurrentWidget(m_choiceWidget);
            break;
        case QuestionType::FillBlank:
            displayFillBlankQuestion(currentQuestion);
            m_answerStack->setCurrentWidget(m_fillBlankWidget);
            break;
        case QuestionType::MultipleChoice:
            displayMultiChoiceQuestion(currentQuestion);
            m_answerStack->setCurrentWidget(m_multiChoiceWidget);
            break;
    }
    
    setAnswerInputsEnabled(true);
    
    // 恢复用户之前的答案状态
    if (m_practiceManager->getQuestionManager()->isAnswered(m_currentQuestionIndex)) {
        restoreUserAnswer();
        m_submitButton->setEnabled(false);
        m_isAnswerSubmitted = true;
        
        // 显示答题结果
        bool isCorrect = m_practiceManager->getQuestionManager()->checkAnswer(m_currentQuestionIndex);
        Question currentQuestion = m_practiceManager->getQuestionManager()->getCurrentQuestion();
        if (currentQuestion.getType() == QuestionType::FillBlank) {
            showAnswerResult(isCorrect, currentQuestion.getAnswers());
        } else if (currentQuestion.getType() == QuestionType::TrueOrFalse) {
            // 对于判断题，将T/F转换为用户友好的显示文本
            QString correctAnswer = currentQuestion.getSingleAnswer();
            QString displayAnswer = (correctAnswer == "T") ? "正确" : "错误";
            showAnswerResult(isCorrect, displayAnswer);
        } else {
            showAnswerResult(isCorrect, currentQuestion.getSingleAnswer());
        }
        setAnswerInputsEnabled(false);
    } else {
        m_submitButton->setEnabled(false);
        m_isAnswerSubmitted = false;
        hideAnswerResult();
    }
}

void PracticeWidget::displayChoiceQuestion(const Question &question)
{
    QStringList choices = question.getChoices();
    
    // Clear existing buttons
    for (QRadioButton *button : m_choiceButtons) {
        m_choiceButtonGroup->removeButton(button);
        button->deleteLater();
    }
    m_choiceButtons.clear();
    
    // Create new buttons
    QStringList labels = {"A", "B", "C", "D", "E", "F"};
    for (int i = 0; i < choices.size() && i < labels.size(); ++i) {
        // QRadioButton *button = new QRadioButton(QString("%1. %2").arg(labels[i]).arg(choices[i]));
        QRadioButton *button = new QRadioButton(choices[i]);
        button->setObjectName("choiceButton");
        m_choiceButtons.append(button);
        m_choiceButtonGroup->addButton(button, i);
        m_choiceLayout->addWidget(button);
        
        connect(button, &QRadioButton::toggled, this, &PracticeWidget::onChoiceSelected);
    }
    
    m_choiceLayout->addStretch();
}

void PracticeWidget::displayTrueOrFalseQuestion(const Question &question)
{
    Q_UNUSED(question)
    
    // Clear existing buttons
    for (QRadioButton *button : m_choiceButtons) {
        m_choiceButtonGroup->removeButton(button);
        button->deleteLater();
    }
    m_choiceButtons.clear();
    
    // Create True/False buttons
    QRadioButton *trueButton = new QRadioButton("A. 正确");
    QRadioButton *falseButton = new QRadioButton("B. 错误");
    
    trueButton->setObjectName("choiceButton");
    falseButton->setObjectName("choiceButton");
    
    m_choiceButtons.append(trueButton);
    m_choiceButtons.append(falseButton);
    
    m_choiceButtonGroup->addButton(trueButton, 0);
    m_choiceButtonGroup->addButton(falseButton, 1);
    
    m_choiceLayout->addWidget(trueButton);
    m_choiceLayout->addWidget(falseButton);
    m_choiceLayout->addStretch();
    
    connect(trueButton, &QRadioButton::toggled, this, &PracticeWidget::onChoiceSelected);
    connect(falseButton, &QRadioButton::toggled, this, &PracticeWidget::onChoiceSelected);
}

void PracticeWidget::displayFillBlankQuestion(const Question &question)
{
    // Clear existing edits
    for (QLineEdit *edit : m_fillBlankEdits) {
        edit->deleteLater();
    }
    m_fillBlankEdits.clear();
    
    int blankCount = question.getBlankNum();
    if (blankCount <= 0) {
        blankCount = 1; // At least one blank
    }
    
    // Create fill blank edits
    for (int i = 0; i < blankCount; ++i) {
        QLabel *label = new QLabel(QString("第 %1 空:").arg(i + 1));
        QLineEdit *edit = new QLineEdit();
        edit->setPlaceholderText("请输入答案");
        
        m_fillBlankLayout->addWidget(label);
        m_fillBlankLayout->addWidget(edit);
        m_fillBlankEdits.append(edit);
        
        connect(edit, &QLineEdit::textChanged, this, &PracticeWidget::onFillBlankChanged);
    }
    
    m_fillBlankLayout->addStretch();
}

void PracticeWidget::displayMultiChoiceQuestion(const Question &question)
{
    QStringList choices = question.getChoices();
    
    // Clear existing checkboxes
    for (QCheckBox *checkbox : m_multiChoiceBoxes) {
        checkbox->deleteLater();
    }
    m_multiChoiceBoxes.clear();
    
    // Create new checkboxes
    QStringList labels = {"A", "B", "C", "D", "E", "F"};
    for (int i = 0; i < choices.size() && i < labels.size(); ++i) {
        // QCheckBox *checkbox = new QCheckBox(QString("%1. %2").arg(labels[i]).arg(choices[i]));
        QCheckBox *checkbox = new QCheckBox(choices[i]);
        checkbox->setObjectName("multiChoiceBox");
        m_multiChoiceBoxes.append(checkbox);
        m_multiChoiceLayout->addWidget(checkbox);
        
        connect(checkbox, &QCheckBox::toggled, [this]() {
            bool hasSelection = false;
            for (QCheckBox *box : m_multiChoiceBoxes) {
                if (box->isChecked()) {
                    hasSelection = true;
                    break;
                }
            }
            m_submitButton->setEnabled(hasSelection && !m_isAnswerSubmitted);
        });
    }
    
    m_multiChoiceLayout->addStretch();
}

void PracticeWidget::updateQuestionList()
{
    if (!m_practiceManager) {
        return;
    }
    
    // 保存当前选中项
    int selectedRow = m_questionListWidget->currentRow();
    
    m_questionListWidget->clear();
    
    int totalQuestions = m_practiceManager->getTotalQuestions();
    for (int i = 0; i < totalQuestions; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole, i);
        
        QString status;
        QColor backgroundColor;
        QColor textColor = QColor("black");
        bool isBold = false;
        
        // 首先检查是否为当前题目
        if (i == m_currentQuestionIndex) {
            status = "●";
            backgroundColor = QColor("#007bff");
            textColor = QColor("black");
            isBold = true;
        }
        // 然后检查答题状态
        else if (m_practiceManager->getQuestionManager()->isAnswered(i)) {
            // Check if answer was correct
            bool correct = m_practiceManager->getQuestionManager()->checkAnswer(i);
            status = correct ? "✓" : "✗";
            backgroundColor = correct ? QColor("#d4edda") : QColor("#f8d7da");
            textColor = correct ? QColor("#155724") : QColor("#721c24");
        } else {
            status = "○";
            backgroundColor = QColor("white");
        }
        
        item->setText(QString("%1 %2").arg(i + 1).arg(status));
        
        // 设置样式
        item->setBackground(backgroundColor);
        item->setForeground(textColor);
        
        if (isBold) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        
        // 设置工具提示
        QString tooltip;
        if (i == m_currentQuestionIndex) {
            tooltip = QString("当前题目 %1").arg(i + 1);
        } else if (m_practiceManager->getQuestionManager()->isAnswered(i)) {
            bool correct = m_practiceManager->getQuestionManager()->checkAnswer(i);
            tooltip = QString("题目 %1 - %2").arg(i + 1).arg(correct ? "答对" : "答错");
        } else {
            tooltip = QString("题目 %1 - 未答").arg(i + 1);
        }
        item->setToolTip(tooltip);
        
        m_questionListWidget->addItem(item);
    }
    
    // 添加完所有项后，清除选中状态以避免与当前题目样式冲突
    m_questionListWidget->setCurrentRow(-1);
    
    // 确保当前题目可见
    if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < totalQuestions) {
        m_questionListWidget->scrollToItem(m_questionListWidget->item(m_currentQuestionIndex));
    }
}

void PracticeWidget::updateNavigationButtons()
{
    if (!m_practiceManager) {
        return;
    }
    
    m_previousButton->setEnabled(m_practiceManager->canGoToPrevious());
    m_nextButton->setEnabled(m_practiceManager->canGoToNext());
    m_submitButton->setEnabled(!m_isAnswerSubmitted);
    m_finishButton->setEnabled(m_practiceManager->canFinishPractice());
}

void PracticeWidget::updateStatistics()
{
    if (!m_practiceManager) {
        return;
    }
    
    m_progressBar->setValue(static_cast<int>(m_practiceManager->getProgress()));
    m_answeredLabel->setText(QString("已答题: %1").arg(m_practiceManager->getAnsweredQuestions()));
    m_correctLabel->setText(QString("正确: %1").arg(m_practiceManager->getCorrectAnswers()));
    m_wrongLabel->setText(QString("错误: %1").arg(m_practiceManager->getWrongAnswers()));
    m_accuracyLabel->setText(QString("正确率: %1%").arg(QString::number(m_practiceManager->getAccuracy(), 'f', 1)));
    m_timeElapsedLabel->setText(QString("用时: %1").arg(m_practiceManager->getElapsedTimeString()));
}

void PracticeWidget::clearAnswerInputs()
{
    // Clear choice buttons
    for (QRadioButton *button : m_choiceButtons) {
        m_choiceButtonGroup->removeButton(button);
        button->deleteLater();
    }
    m_choiceButtons.clear();
    
    // Clear multi-choice boxes
    for (QCheckBox *checkbox : m_multiChoiceBoxes) {
        checkbox->deleteLater();
    }
    m_multiChoiceBoxes.clear();
    
    // Clear fill blank edits
    for (QLineEdit *edit : m_fillBlankEdits) {
        edit->deleteLater();
    }
    m_fillBlankEdits.clear();
    
    // Clear layouts
    QLayoutItem *item;
    while ((item = m_choiceLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    while ((item = m_multiChoiceLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    while ((item = m_fillBlankLayout->takeAt(0)) != nullptr) {
        delete item;
    }
}

void PracticeWidget::setAnswerInputsEnabled(bool enabled)
{
    for (QRadioButton *button : m_choiceButtons) {
        button->setEnabled(enabled);
    }
    for (QCheckBox *checkbox : m_multiChoiceBoxes) {
        checkbox->setEnabled(enabled);
    }
    for (QLineEdit *edit : m_fillBlankEdits) {
        edit->setEnabled(enabled);
    }
}

QString PracticeWidget::getCurrentAnswer() const
{
    if (m_currentQuestionType == QuestionType::Choice || m_currentQuestionType == QuestionType::TrueOrFalse) {
        QAbstractButton *checkedButton = m_choiceButtonGroup->checkedButton();
        if (checkedButton) {
            int id = m_choiceButtonGroup->id(checkedButton);
            if (m_currentQuestionType == QuestionType::TrueOrFalse) {
                return id == 0 ? "T" : "F";
            } else {
                QStringList labels = {"A", "B", "C", "D", "E", "F"};
                return id < labels.size() ? labels[id] : "";
            }
        }
    } else if (m_currentQuestionType == QuestionType::MultipleChoice) {
        QStringList selectedOptions;
        QStringList labels = {"A", "B", "C", "D", "E", "F"};
        for (int i = 0; i < m_multiChoiceBoxes.size() && i < labels.size(); ++i) {
            if (m_multiChoiceBoxes[i]->isChecked()) {
                selectedOptions.append(labels[i]);
            }
        }
        return selectedOptions.join(",");
    }
    
    return QString();
}

QStringList PracticeWidget::getCurrentAnswers() const
{
    QStringList answers;
    for (QLineEdit *edit : m_fillBlankEdits) {
        answers.append(edit->text().trimmed());
    }
    return answers;
}

void PracticeWidget::showAnswerResult(bool correct, const QString &correctAnswer)
{
    qDebug() << "showAnswerResult called: correct=" << correct << ", correctAnswer=" << correctAnswer;
    
    m_resultFrame->setVisible(true);
    m_resultFrame->setProperty("correct", correct);
    m_resultFrame->style()->unpolish(m_resultFrame);
    m_resultFrame->style()->polish(m_resultFrame);
    
    // Ensure all child widgets are visible
    if (m_resultIcon) m_resultIcon->setVisible(true);
    if (m_resultText) m_resultText->setVisible(true);
    if (m_correctAnswerLabel) m_correctAnswerLabel->setVisible(true);
    
    if (correct) {
        m_resultIcon->setText("✓");
        m_resultIcon->setStyleSheet("color: #28a745; font-size: 18px; font-weight: bold;");
        m_resultText->setText("回答正确！");
        m_resultText->setStyleSheet("color: #28a745;");
        m_correctAnswerLabel->setText("");
        m_correctAnswerLabel->setVisible(false); // Hide when correct
    } else {
        m_resultIcon->setText("✗");
        m_resultIcon->setStyleSheet("color: #dc3545; font-size: 18px; font-weight: bold;");
        m_resultText->setText("回答错误");
        m_resultText->setStyleSheet("color: #dc3545;");
        m_correctAnswerLabel->setText(QString("正确答案: %1").arg(correctAnswer));
        m_correctAnswerLabel->setVisible(true); // Ensure it's visible when showing correct answer
        qDebug() << "Set correct answer label to:" << QString("正确答案: %1").arg(correctAnswer);
    }
}

void PracticeWidget::showAnswerResult(bool correct, const QStringList &correctAnswers)
{
    showAnswerResult(correct, correctAnswers.join(", "));
}

bool PracticeWidget::confirmFinish()
{
    if (!m_practiceManager) {
        return false;
    }
    
    QVector<int> unanswered = m_practiceManager->getUnansweredQuestions();
    
    QString message = "确定要完成练习吗？";
    if (!unanswered.isEmpty()) {
        QStringList unansweredStr;
        for (int index : unanswered) {
            unansweredStr.append(QString::number(index + 1));
        }
        message += QString("\n\n还有 %1 道题未作答：%2")
                  .arg(unanswered.size())
                  .arg(unansweredStr.join(", "));
    }
    
    int ret = QMessageBox::question(this, "确认完成", message,
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    return ret == QMessageBox::Yes;
}

QVector<int> PracticeWidget::getUnansweredQuestions() const
{
    if (m_practiceManager) {
        return m_practiceManager->getUnansweredQuestions();
    }
    return QVector<int>();
}

void PracticeWidget::onBackClicked()
{
    if (!m_practiceManager) {
        emit backRequested();
        return;
    }
    
    // 检查练习状态
    PracticeState currentState = m_practiceManager->getCurrentState();
    if (currentState == PracticeState::Completed) {
        // 已完成的练习直接返回
        emit backRequested();
        return;
    }
    
    // 显示保存并退出确认对话框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("保存并退出");
    msgBox.setText("是否保存当前练习进度并退出？");
    msgBox.setInformativeText("保存后可以在下次开始练习时选择继续。");
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.setButtonText(QMessageBox::Save, "保存并退出");
    msgBox.setButtonText(QMessageBox::Discard, "直接退出");
    msgBox.setButtonText(QMessageBox::Cancel, "取消");
    
    int ret = msgBox.exec();
    
    switch (ret) {
        case QMessageBox::Save:
            // 保存练习进度
            if (m_practiceManager) {
                // 需要传入ConfigManager，从主窗口获取
                // 这里先发射信号，让主窗口处理保存逻辑
                emit saveAndExitRequested();
            }
            break;
        case QMessageBox::Discard:
            // 直接退出，不保存
            emit backRequested();
            break;
        case QMessageBox::Cancel:
        default:
            // 取消，什么都不做
            break;
    }
}

void PracticeWidget::restoreUserAnswer()
{
    if (!m_practiceManager || !m_practiceManager->getQuestionManager()) {
        return;
    }
    
    const Question &currentQuestion = m_practiceManager->getQuestionManager()->getCurrentQuestion();
    
    switch (currentQuestion.getType()) {
        case QuestionType::Choice: {
            QString userAnswer = m_practiceManager->getQuestionManager()->getUserAnswer(m_currentQuestionIndex);
            if (!userAnswer.isEmpty()) {
                // 恢复选择题答案
                for (int i = 0; i < m_choiceButtons.size(); ++i) {
                    if (m_choiceButtons[i]->text().startsWith(userAnswer + ".")) {
                        m_choiceButtons[i]->setChecked(true);
                        break;
                    }
                }
            }
            break;
        }
        case QuestionType::TrueOrFalse: {
            QString userAnswer = m_practiceManager->getQuestionManager()->getUserAnswer(m_currentQuestionIndex);
            if (!userAnswer.isEmpty()) {
                // 恢复判断题答案：T对应第0个按钮(A. 正确)，F对应第1个按钮(B. 错误)
                if (userAnswer == "T" && m_choiceButtons.size() > 0) {
                    m_choiceButtons[0]->setChecked(true);
                } else if (userAnswer == "F" && m_choiceButtons.size() > 1) {
                    m_choiceButtons[1]->setChecked(true);
                }
            }
            break;
        }
        case QuestionType::MultipleChoice: {
            QStringList userAnswers = m_practiceManager->getQuestionManager()->getUserMultiAnswers(m_currentQuestionIndex);
            if (!userAnswers.isEmpty()) {
                // 恢复多选答案
                for (int i = 0; i < m_multiChoiceBoxes.size(); ++i) {
                    QString optionText = m_multiChoiceBoxes[i]->text();
                    if (!optionText.isEmpty()) {
                        QString option = optionText.left(1); // 获取选项字母
                        m_multiChoiceBoxes[i]->setChecked(userAnswers.contains(option));
                    }
                }
            }
            break;
        }
        case QuestionType::FillBlank: {
            QString userAnswer = m_practiceManager->getQuestionManager()->getUserAnswer(m_currentQuestionIndex);
            if (!userAnswer.isEmpty() && !m_fillBlankEdits.isEmpty()) {
                m_fillBlankEdits[0]->setText(userAnswer);
            }
            break;
        }
    }
}

void PracticeWidget::hideAnswerResult()
{
    if (m_resultFrame) {
        m_resultFrame->hide();
    }
    // Note: m_correctAnswerLabel is part of m_resultFrame, so it will be hidden automatically
    // No need to hide it separately
}
