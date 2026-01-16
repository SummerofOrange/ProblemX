#include "question.h"
#include <QJsonArray>
#include <QDebug>

Question::Question()
    : m_type(QuestionType::Choice)
    , m_blankNum(0)
{
}

Question::Question(const QJsonObject &json)
    : m_blankNum(0)
{
    fromJson(json);
}

bool Question::checkAnswer(const QString &userAnswer) const
{
    if (m_answers.isEmpty()) {
        return false;
    }
    
    QString trimmedAnswer = userAnswer.trimmed().toUpper();
    QString correctAnswer = m_answers.first().trimmed().toUpper();
    
    return trimmedAnswer == correctAnswer;
}

bool Question::checkAnswers(const QStringList &userAnswers) const
{
    if (userAnswers.size() != m_answers.size()) {
        return false;
    }
    
    for (int i = 0; i < userAnswers.size(); ++i) {
        QString userAns = userAnswers[i].trimmed();
        QString correctAns = m_answers[i].trimmed();
        if (userAns != correctAns) {
            return false;
        }
    }
    
    return true;
}

QJsonObject Question::toJson() const
{
    QJsonObject json;
    json["type"] = typeToString(m_type);
    json["question"] = m_question;
    
    if (!m_choices.isEmpty()) {
        QJsonArray choicesArray;
        for (const QString &choice : m_choices) {
            choicesArray.append(choice);
        }
        json["choices"] = choicesArray;
    }
    
    if (m_type == QuestionType::FillBlank) {
        json["BlankNum"] = m_blankNum;
        QJsonArray answersArray;
        for (const QString &answer : m_answers) {
            answersArray.append(answer);
        }
        json["answer"] = answersArray;
    } else {
        json["answer"] = m_answers.isEmpty() ? "" : m_answers.first();
    }
    
    if (!m_images.isEmpty()) {
        QJsonObject imageObj;
        for (auto it = m_images.constBegin(); it != m_images.constEnd(); ++it) {
            imageObj.insert(it.key(), it.value());
        }
        json["image"] = imageObj;
    }
    
    return json;
}

void Question::fromJson(const QJsonObject &json)
{
    // Parse type
    QString typeStr = json["type"].toString();
    m_type = stringToType(typeStr);
    
    // Parse question
    m_question = json["question"].toString();
    
    // Parse choices (for choice questions)
    if (json.contains("choices") && json["choices"].isArray()) {
        QJsonArray choicesArray = json["choices"].toArray();
        m_choices.clear();
        for (const QJsonValue &value : choicesArray) {
            m_choices.append(value.toString());
        }
    }
    
    // Parse answers
    m_answers.clear();
    if (m_type == QuestionType::FillBlank) {
        m_blankNum = json["BlankNum"].toInt();
        if (json["answer"].isArray()) {
            QJsonArray answersArray = json["answer"].toArray();
            for (const QJsonValue &value : answersArray) {
                m_answers.append(value.toString());
            }
        }
    } else {
        QString answer = json["answer"].toString();
        if (!answer.isEmpty()) {
            m_answers.append(answer);
        }
    }
    
    // Parse image path
    m_images.clear();
    if (json.contains("image")) {
        const QJsonValue imageValue = json.value("image");
        if (imageValue.isObject()) {
            const QJsonObject imageObj = imageValue.toObject();
            for (auto it = imageObj.begin(); it != imageObj.end(); ++it) {
                const QString key = it.key().trimmed();
                const QString value = it.value().toString();
                if (!key.isEmpty() && !value.isEmpty()) {
                    m_images.insert(key, value);
                }
            }
        }
    }
}

QuestionType Question::stringToType(const QString &typeStr)
{
    if (typeStr == "Choice") {
        return QuestionType::Choice;
    } else if (typeStr == "TrueorFalse" || typeStr == "TrueOrFalse") {
        return QuestionType::TrueOrFalse;
    } else if (typeStr == "FillBlank") {
        return QuestionType::FillBlank;
    } else if (typeStr == "MultipleChoice") {
        return QuestionType::MultipleChoice;
    }
    
    return QuestionType::Choice; // Default
}

QString Question::typeToString(QuestionType type)
{
    switch (type) {
    case QuestionType::Choice:
        return "Choice";
    case QuestionType::TrueOrFalse:
        return "TrueorFalse";
    case QuestionType::FillBlank:
        return "FillBlank";
    case QuestionType::MultipleChoice:
        return "MultipleChoice";
    }
    
    return "Choice"; // Default
}

// Stream operators implementation
QDebug operator<<(QDebug debug, const Question &question)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Question(type=" << static_cast<int>(question.getType())
                    << ", question=" << question.getQuestion() << ")";
    return debug;
}

QDataStream &operator<<(QDataStream &stream, const Question &question)
{
    stream << static_cast<int>(question.getType())
           << question.getQuestion()
           << question.getChoices()
           << question.getAnswers()
           << question.getImages()
           << question.getBlankNum();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Question &question)
{
    int type;
    QString questionText;
    QStringList choices, answers;
    QMap<QString, QString> images;
    int blankNum;
    
    stream >> type >> questionText >> choices >> answers >> images >> blankNum;
    
    question.setType(static_cast<QuestionType>(type));
    question.setQuestion(questionText);
    question.setChoices(choices);
    question.setAnswers(answers);
    question.setImages(images);
    question.setBlankNum(blankNum);
    
    return stream;
}
