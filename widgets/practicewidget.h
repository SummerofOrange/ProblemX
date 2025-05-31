#ifndef PRACTICEWIDGET_H
#define PRACTICEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>
#include <QProgressBar>
#include <QTimer>
#include <QListWidget>
#include <QListWidgetItem>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QFrame>
#include <QKeyEvent>
#include <QShortcut>
#include "../models/question.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QScrollArea;
class QSplitter;
class QProgressBar;
class QTimer;
class QListWidget;
class QListWidgetItem;
class QButtonGroup;
class QStackedWidget;
class QFrame;
class QShortcut;
QT_END_NAMESPACE

class PracticeManager;
enum class QuestionType;

class PracticeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PracticeWidget(QWidget *parent = nullptr);
    ~PracticeWidget();

    void setPracticeManager(PracticeManager *practiceManager);
    void startPractice();
    void pausePractice();
    void resumePractice();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onQuestionChanged(int index, int total);
    void onAnswerSubmitted(int index, bool correct);
    void onProgressChanged(double progress);
    void onStatisticsUpdated(int correct, int wrong, double accuracy);
    void onPracticeCompleted();
    
    void onPreviousClicked();
    void onNextClicked();
    void onSubmitClicked();
    void onPauseClicked();
    void onFinishClicked();
    void onBackClicked();  // 处理返回按钮点击
    void onQuestionListItemClicked(QListWidgetItem *item);
    
    void onChoiceSelected();
    void onFillBlankChanged();
    void updateTimer();

signals:
    void practiceFinished();
    void practicePaused();
    void backRequested();
    void saveAndExitRequested();  // 保存并退出请求
    void practiceCompletedAndClearSave();  // 练习完成并清除存档

private:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    void applyStyles();
    
    void updateQuestionDisplay();
    void updateQuestionList();
    void updateNavigationButtons();
    void updateStatistics();
    void clearAnswerInputs();
    void setAnswerInputsEnabled(bool enabled);
    
    void displayChoiceQuestion(const Question &question);
    void displayTrueOrFalseQuestion(const Question &question);
    void displayFillBlankQuestion(const Question &question);
    void displayMultiChoiceQuestion(const Question &question);
    
    QString getCurrentAnswer() const;
    QStringList getCurrentAnswers() const;
    void restoreUserAnswer();
    void showAnswerResult(bool correct, const QString &correctAnswer = "");
    void showAnswerResult(bool correct, const QStringList &correctAnswers = QStringList());
    void hideAnswerResult();
    
    bool confirmFinish();
    QVector<int> getUnansweredQuestions() const;
    
    // UI Components - Main Layout
    QSplitter *m_mainSplitter;
    
    // Left Panel - Question Display
    QWidget *m_questionPanel;
    QVBoxLayout *m_questionLayout;
    
    // Question Header
    QHBoxLayout *m_headerLayout;
    QLabel *m_questionNumberLabel;
    QLabel *m_questionTypeLabel;
    QLabel *m_timerLabel;
    QPushButton *m_pauseButton;
    
    // Question Content
    QScrollArea *m_questionScrollArea;
    QWidget *m_questionContent;
    QVBoxLayout *m_questionContentLayout;
    QLabel *m_questionTextLabel;
    QLabel *m_questionImageLabel;
    
    // Answer Input Area
    QStackedWidget *m_answerStack;
    
    // Choice Question Widgets
    QWidget *m_choiceWidget;
    QVBoxLayout *m_choiceLayout;
    QButtonGroup *m_choiceButtonGroup;
    QVector<QRadioButton*> m_choiceButtons;
    
    // Multi-Choice Question Widgets
    QWidget *m_multiChoiceWidget;
    QVBoxLayout *m_multiChoiceLayout;
    QVector<QCheckBox*> m_multiChoiceBoxes;
    
    // Fill Blank Question Widgets
    QWidget *m_fillBlankWidget;
    QVBoxLayout *m_fillBlankLayout;
    QVector<QLineEdit*> m_fillBlankEdits;
    
    // Answer Result Display
    QFrame *m_resultFrame;
    QHBoxLayout *m_resultLayout;
    QLabel *m_resultIcon;
    QLabel *m_resultText;
    QLabel *m_correctAnswerLabel;
    
    // Navigation Buttons
    QHBoxLayout *m_navigationLayout;
    QPushButton *m_previousButton;
    QPushButton *m_submitButton;
    QPushButton *m_nextButton;
    QPushButton *m_finishButton;
    
    // Right Panel - Question List and Statistics
    QWidget *m_sidePanel;
    QVBoxLayout *m_sideLayout;
    
    // Statistics Group
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_progressLabel;
    QProgressBar *m_progressBar;
    QLabel *m_answeredLabel;
    QLabel *m_correctLabel;
    QLabel *m_wrongLabel;
    QLabel *m_accuracyLabel;
    QLabel *m_timeElapsedLabel;
    
    // Question List Group
    QGroupBox *m_questionListGroup;
    QVBoxLayout *m_questionListLayout;
    QListWidget *m_questionListWidget;
    
    // Control Buttons
    QHBoxLayout *m_controlLayout;
    QPushButton *m_backButton;
    
    // Data and State
    PracticeManager *m_practiceManager;
    QTimer *m_timer;
    int m_currentQuestionIndex;
    int m_totalQuestions;
    bool m_isAnswerSubmitted;
    bool m_isPaused;
    QuestionType m_currentQuestionType;
    
    // Shortcuts
    QShortcut *m_shortcutA;
    QShortcut *m_shortcutB;
    QShortcut *m_shortcutC;
    QShortcut *m_shortcutD;
    QShortcut *m_shortcutT;
    QShortcut *m_shortcutF;
    QShortcut *m_shortcutEnter;
    QShortcut *m_shortcutLeft;
    QShortcut *m_shortcutRight;
    QShortcut *m_shortcutSpace;        // 空格：跳转到下一题
    QShortcut *m_shortcutShiftSpace;   // Shift+空格：跳转到上一题
};

#endif // PRACTICEWIDGET_H