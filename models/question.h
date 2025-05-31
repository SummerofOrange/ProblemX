#ifndef QUESTION_H
#define QUESTION_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMetaType>
#include <QDebug>
#include <QDataStream>

enum class QuestionType {
    Choice,
    TrueOrFalse,
    FillBlank,
    MultipleChoice
};

class Question
{
public:
    Question();
    Question(const QJsonObject &json);
    
    // Getters
    QuestionType getType() const { return m_type; }
    QString getQuestion() const { return m_question; }
    QStringList getChoices() const { return m_choices; }
    QStringList getAnswers() const { return m_answers; }
    QString getSingleAnswer() const { return m_answers.isEmpty() ? "" : m_answers.first(); }
    QString getImagePath() const { return m_imagePath; }
    int getBlankNum() const { return m_blankNum; }
    bool hasImage() const { return !m_imagePath.isEmpty(); }
    
    // Setters
    void setType(QuestionType type) { m_type = type; }
    void setQuestion(const QString &question) { m_question = question; }
    void setChoices(const QStringList &choices) { m_choices = choices; }
    void setAnswers(const QStringList &answers) { m_answers = answers; }
    void setSingleAnswer(const QString &answer) { m_answers = QStringList() << answer; }
    void setImagePath(const QString &imagePath) { m_imagePath = imagePath; }
    void setBlankNum(int blankNum) { m_blankNum = blankNum; }
    
    // Utility methods
    bool checkAnswer(const QString &userAnswer) const;
    bool checkAnswers(const QStringList &userAnswers) const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    static QuestionType stringToType(const QString &typeStr);
    static QString typeToString(QuestionType type);
    
private:
    QuestionType m_type;
    QString m_question;
    QStringList m_choices;  // For choice questions
    QStringList m_answers;  // Can be single or multiple answers
    QString m_imagePath;    // Optional image path
    int m_blankNum;         // For fill blank questions
};

// Qt Meta Type support
Q_DECLARE_METATYPE(Question)
Q_DECLARE_METATYPE(QList<Question>)

// Stream operators for Qt serialization
QDebug operator<<(QDebug debug, const Question &question);
QDataStream &operator<<(QDataStream &stream, const Question &question);
QDataStream &operator>>(QDataStream &stream, Question &question);

#endif // QUESTION_H