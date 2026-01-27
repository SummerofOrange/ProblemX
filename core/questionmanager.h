#ifndef QUESTIONMANAGER_H
#define QUESTIONMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QStringList>
#include "../models/question.h"
#include "../models/questionbank.h"

class QuestionManager : public QObject
{
    Q_OBJECT
    
public:
    explicit QuestionManager(QObject *parent = nullptr);
    
    // Question loading
    bool loadQuestions(const QuestionBank &bank, const QString &subjectPath, bool shuffleQuestions = true);
    bool loadQuestionsFromFiles(const QStringList &filePaths);
    void setQuestions(const QList<Question> &questions);
    
    // Question access
    int getQuestionCount() const { return m_questions.size(); }
    Question getQuestion(int index) const;
    QList<Question> getAllQuestions() const { return m_questions; }
    
    // Question navigation
    int getCurrentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    Question getCurrentQuestion() const;
    bool hasNext() const;
    bool hasPrevious() const;
    void moveNext();
    void movePrevious();
    void moveToFirst();
    void moveToLast();
    
    // Question state management
    bool isAnswered(int index) const;
    void setAnswered(int index, bool answered = true);
    QVector<int> getUnansweredQuestions() const;
    QVector<int> getAnsweredQuestions() const;
    int getAnsweredCount() const;
    int getUnansweredCount() const;
    
    // Answer management
    void setUserAnswer(int index, const QString &answer);
    void setUserAnswers(int index, const QStringList &answers);
    void setUserAnswerWithoutCheck(int index, const QString &answer);
    void setUserAnswersWithoutCheck(int index, const QStringList &answers);
    QString getUserAnswer(int index) const;
    QStringList getUserAnswers(int index) const;
    QStringList getUserMultiAnswers(int index) const;
    
    // Validation
    bool checkAnswer(int index) const;
    bool checkCurrentAnswer() const;
    
    // Wrong answers tracking
    void addWrongAnswer(int index);
    QList<Question> getWrongAnswers() const { return m_wrongAnswers; }
    QVector<int> getWrongAnswerIndices() const { return m_wrongAnswerIndices; }
    void clearWrongAnswers() { m_wrongAnswers.clear(); m_wrongAnswerIndices.clear(); }
    
    // Statistics
    int getCorrectCount() const { return m_correctCount; }
    int getWrongCount() const { return m_wrongCount; }
    double getAccuracy() const;
    void setCorrectCount(int count) { m_correctCount = count; }
    void setWrongCount(int count) { m_wrongCount = count; }
    
    // Reset
    void reset();
    void resetAnswers();
    
    // Utility
    bool isEmpty() const { return m_questions.isEmpty(); }
    QString getQuestionTypeString(int index) const;
    
signals:
    void currentQuestionChanged(int index);
    void questionAnswered(int index, bool correct);
    void allQuestionsAnswered();
    
private:
    QList<Question> m_questions;
    QVector<bool> m_answeredFlags;
    QVector<QString> m_userAnswers;
    QVector<QStringList> m_userMultiAnswers;
    QList<Question> m_wrongAnswers;
    QVector<int> m_wrongAnswerIndices;
    
    int m_currentIndex;
    int m_correctCount;
    int m_wrongCount;
    
    void initializeAnswerTracking();
    bool isValidIndex(int index) const;
};

#endif // QUESTIONMANAGER_H
