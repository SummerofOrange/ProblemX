#ifndef PRACTICEMANAGER_H
#define PRACTICEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "questionmanager.h"
#include "configmanager.h"
#include "wronganswerset.h"

enum class PracticeMode {
    Normal,         // 正常练习模式
    Resume,         // 从存档继续
    Review          // 错题复习模式
};

enum class PracticeState {
    NotStarted,     // 未开始
    InProgress,     // 进行中
    Paused,         // 暂停
    Completed,      // 已完成
    Saved           // 已保存退出
};

struct PracticeSession {
    PracticeMode mode;
    PracticeState state;
    QDateTime startTime;
    QDateTime endTime;
    QDateTime pauseTime;        // 暂停开始时间
    qint64 totalPausedTime;     // 累计暂停时长（秒）
    QString subject;
    int totalQuestions;
    int answeredQuestions;
    int correctAnswers;
    int wrongAnswers;
    double accuracy;
    QList<Question> wrongQuestionsList;
    
    PracticeSession() 
        : mode(PracticeMode::Normal)
        , state(PracticeState::NotStarted)
        , totalPausedTime(0)
        , totalQuestions(0)
        , answeredQuestions(0)
        , correctAnswers(0)
        , wrongAnswers(0)
        , accuracy(0.0) {}
};

class PracticeManager : public QObject
{
    Q_OBJECT
    
public:
    explicit PracticeManager(QObject *parent = nullptr);
    
    // Practice session management
    bool startNewPractice(const QString &subject, ConfigManager *configManager);
    bool resumePractice(ConfigManager *configManager);
    bool startReviewPractice(const QList<Question> &wrongQuestions, const QString &subjectPath = QString());
    void pausePractice();
    void resumePractice();  // 从暂停状态恢复
    void completePractice();
    void savePractice(ConfigManager *configManager);
    
    // Question management
    QuestionManager* getQuestionManager() { return &m_questionManager; }
    const QuestionManager* getQuestionManager() const { return &m_questionManager; }
    
    // Session information
    PracticeSession getCurrentSession() const { return m_currentSession; }
    PracticeMode getCurrentMode() const { return m_currentSession.mode; }
    PracticeState getCurrentState() const { return m_currentSession.state; }
    
    // Progress tracking
    int getTotalQuestions() const;
    int getAnsweredQuestions() const;
    int getRemainingQuestions() const;
    double getProgress() const; // 0.0 to 100.0
    
    // Statistics
    int getCorrectAnswers() const;
    int getWrongAnswers() const;
    double getAccuracy() const;
    QList<Question> getWrongQuestions() const;
    
    // Time tracking
    QDateTime getStartTime() const { return m_currentSession.startTime; }
    QDateTime getEndTime() const { return m_currentSession.endTime; }
    qint64 getElapsedTime() const; // in seconds
    QString getElapsedTimeString() const;
    
    // Answer submission
    bool submitAnswer(const QString &answer);
    bool submitAnswers(const QStringList &answers);
    bool canSubmitAnswer() const;
    
    // Navigation
    bool canGoToNext() const;
    bool canGoToPrevious() const;
    void goToNext();
    void goToPrevious();
    void goToQuestion(int index);
    
    // Validation
    bool canFinishPractice() const;
    QVector<int> getUnansweredQuestions() const;
    
    // Wrong answers management
    void saveWrongAnswers(const QString &filePath = "");
    static QList<Question> loadWrongAnswers(const QString &filePath);
    static QStringList getAvailableWrongAnswerFiles(const QString &directory = "WA");
    
    // 新的错题集合管理
    void setWrongAnswerSet(WrongAnswerSet *wrongAnswerSet);
    bool askUserToImportWrongAnswers();
    int importWrongAnswersToSet();
    
    // Checkpoint management
    void saveCheckpoint(ConfigManager *configManager);
    
    // Reset
    void reset();
    
signals:
    void practiceStarted(PracticeMode mode);
    void practiceCompleted(const PracticeSession &session);
    void practicePaused();
    void practiceResumed();
    void questionChanged(int currentIndex, int totalQuestions);
    void answerSubmitted(int questionIndex, bool correct);
    void progressChanged(double percentage);
    void statisticsUpdated(int correct, int wrong, double accuracy);
    void wrongAnswersImportRequested(const QList<Question> &wrongQuestions, const QString &subject);
    
private slots:
    void onQuestionAnswered(int index, bool correct);
    void onAllQuestionsAnswered();
    
private:
    QuestionManager m_questionManager;
    PracticeSession m_currentSession;
    WrongAnswerSet *m_wrongAnswerSet;
    QString m_currentSubjectPath;  // 添加当前科目路径存储
    
    void updateSessionStatistics();
    void setState(PracticeState state);
    QString generateWrongAnswersFileName() const;
    
public:
    // 添加获取当前科目路径的方法
    QString getCurrentSubjectPath() const { return m_currentSubjectPath; }
};

#endif // PRACTICEMANAGER_H
