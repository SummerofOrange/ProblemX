#include "wronganswerset.h"
#include "../utils/jsonutils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QCryptographicHash>
#include <QStringConverter>

// WrongAnswerItem implementation
QJsonObject WrongAnswerItem::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["subject"] = subject;
    obj["questionType"] = questionType;
    obj["questionText"] = questionText;
    if (!images.isEmpty()) {
        QJsonObject imageObj;
        for (auto it = images.constBegin(); it != images.constEnd(); ++it) {
            imageObj.insert(it.key(), it.value());
        }
        obj["image"] = imageObj;
    }
    
    QJsonArray choicesArray;
    for (const QString &choice : choices) {
        choicesArray.append(choice);
    }
    obj["choices"] = choicesArray;
    
    obj["correctAnswer"] = correctAnswer;
    
    QJsonArray correctAnswersArray;
    for (const QString &answer : correctAnswers) {
        correctAnswersArray.append(answer);
    }
    obj["correctAnswers"] = correctAnswersArray;
    
    obj["userAnswer"] = userAnswer;
    
    QJsonArray userAnswersArray;
    for (const QString &answer : userAnswers) {
        userAnswersArray.append(answer);
    }
    obj["userAnswers"] = userAnswersArray;
    
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["reviewCount"] = reviewCount;
    obj["isResolved"] = isResolved;
    obj["note"] = note;
    
    return obj;
}

WrongAnswerItem WrongAnswerItem::fromJson(const QJsonObject &json)
{
    WrongAnswerItem item;
    item.id = json["id"].toString();
    item.subject = json["subject"].toString();
    item.questionType = json["questionType"].toString();
    item.questionText = json["questionText"].toString();
    item.images.clear();
    if (json.contains("image")) {
        const QJsonValue imageValue = json.value("image");
        if (imageValue.isObject()) {
            const QJsonObject imageObj = imageValue.toObject();
            for (auto it = imageObj.begin(); it != imageObj.end(); ++it) {
                const QString key = it.key().trimmed();
                const QString value = it.value().toString();
                if (!key.isEmpty() && !value.isEmpty()) {
                    item.images.insert(key, value);
                }
            }
        }
    }
    
    QJsonArray choicesArray = json["choices"].toArray();
    for (const QJsonValue &value : choicesArray) {
        item.choices.append(value.toString());
    }
    
    item.correctAnswer = json["correctAnswer"].toString();
    
    QJsonArray correctAnswersArray = json["correctAnswers"].toArray();
    for (const QJsonValue &value : correctAnswersArray) {
        item.correctAnswers.append(value.toString());
    }
    
    item.userAnswer = json["userAnswer"].toString();
    
    QJsonArray userAnswersArray = json["userAnswers"].toArray();
    for (const QJsonValue &value : userAnswersArray) {
        item.userAnswers.append(value.toString());
    }
    
    item.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    item.reviewCount = json["reviewCount"].toInt();
    item.isResolved = json["isResolved"].toBool();
    item.note = json["note"].toString();
    
    return item;
}

void WrongAnswerItem::generateId()
{
    id = QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(16);
}

WrongAnswerItem WrongAnswerItem::fromQuestion(const Question &question, const QString &subject,
                                             const QString &userAnswer, const QList<QString> &userAnswers)
{
    WrongAnswerItem item;
    item.subject = subject;
    item.questionText = question.getQuestion();
    item.images = question.getImages();
    item.choices = question.getChoices();
    item.timestamp = QDateTime::currentDateTime();
    item.reviewCount = 0;
    item.isResolved = false;
    
    // 设置题目类型
    switch (question.getType()) {
        case QuestionType::Choice:
            item.questionType = "Choice";
            break;
        case QuestionType::TrueOrFalse:
            item.questionType = "TrueOrFalse";
            break;
        case QuestionType::FillBlank:
            item.questionType = "FillBlank";
            break;
        case QuestionType::MultipleChoice:
            item.questionType = "MultiChoice";
            break;
        default:
            item.questionType = "Unknown";
            break;
    }
    
    // 设置正确答案
    if (question.getType() == QuestionType::FillBlank || question.getType() == QuestionType::MultipleChoice) {
        item.correctAnswers = question.getAnswers();
        if (!item.correctAnswers.isEmpty()) {
            item.correctAnswer = item.correctAnswers.first();
        }
    } else {
        item.correctAnswer = question.getSingleAnswer();
        item.correctAnswers.append(item.correctAnswer);
    }
    
    // 设置用户答案
    if (question.getType() == QuestionType::FillBlank || question.getType() == QuestionType::MultipleChoice) {
        item.userAnswers = userAnswers;
        if (!item.userAnswers.isEmpty()) {
            item.userAnswer = item.userAnswers.first();
        }
    } else {
        item.userAnswer = userAnswer;
        item.userAnswers.append(item.userAnswer);
    }
    
    item.generateId();
    return item;
}

// WrongAnswerSet implementation
WrongAnswerSet::WrongAnswerSet(QObject *parent)
    : QObject(parent)
    , m_filePath(getDefaultFilePath())
{
}

WrongAnswerSet::~WrongAnswerSet()
{
}

bool WrongAnswerSet::loadFromFile(const QString &filePath)
{
    QString path = filePath.isEmpty() ? m_filePath : filePath;
    
    // 检查文件是否存在，如果不存在则创建一个空的错题集合文件
    QFile file(path);
    if (!file.exists()) {
        qDebug() << "WA_SET.json file does not exist, creating new empty file:" << path;
        
        // 创建空的错题集合
        m_wrongAnswers.clear();
        
        // 保存空的错题集合到文件
        if (saveToFile(path)) {
            qDebug() << "Created new empty wrong answer set file:" << path;
            emit dataChanged();
            return true;
        } else {
            qDebug() << "Failed to create new wrong answer set file:" << path;
            return false;
        }
    }
    
    QJsonObject json = JsonUtils::loadJsonFromFile(path);
    if (json.isEmpty()) {
        qDebug() << "Failed to load wrong answer set from:" << path;
        return false;
    }
    
    m_wrongAnswers.clear();
    
    if (json.contains("wrongAnswers") && json["wrongAnswers"].isArray()) {
        QJsonArray array = json["wrongAnswers"].toArray();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                WrongAnswerItem item = WrongAnswerItem::fromJson(value.toObject());
                m_wrongAnswers.append(item);
            }
        }
    }
    
    qDebug() << "Loaded" << m_wrongAnswers.size() << "wrong answers from:" << path;
    emit dataChanged();
    return true;
}

bool WrongAnswerSet::saveToFile(const QString &filePath) const
{
    QString path = filePath.isEmpty() ? m_filePath : filePath;
    
    // 确保目录存在
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject json;
    json["version"] = "1.0";
    json["createdTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json["totalCount"] = m_wrongAnswers.size();
    
    QJsonArray array;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        array.append(item.toJson());
    }
    json["wrongAnswers"] = array;
    
    bool success = JsonUtils::saveJsonToFile(json, path);
    if (success) {
        qDebug() << "Saved" << m_wrongAnswers.size() << "wrong answers to:" << path;
    } else {
        qDebug() << "Failed to save wrong answers to:" << path;
    }
    
    return success;
}

QString WrongAnswerSet::getDefaultFilePath() const
{
    return QCoreApplication::applicationDirPath() + "/WA_SET.json";
}

void WrongAnswerSet::addWrongAnswer(const WrongAnswerItem &item)
{
    WrongAnswerItem newItem = item;
    if (newItem.id.isEmpty()) {
        newItem.generateId();
    }
    
    m_wrongAnswers.append(newItem);
    emit wrongAnswerAdded(newItem);
    emit dataChanged();
}

void WrongAnswerSet::addWrongAnswers(const QVector<WrongAnswerItem> &items)
{
    for (const WrongAnswerItem &item : items) {
        WrongAnswerItem newItem = item;
        if (newItem.id.isEmpty()) {
            newItem.generateId();
        }
        m_wrongAnswers.append(newItem);
        emit wrongAnswerAdded(newItem);
    }
    
    if (!items.isEmpty()) {
        emit dataChanged();
    }
    
    qDebug() << "Added" << items.size() << "wrong answers";
}

bool WrongAnswerSet::removeWrongAnswer(const QString &id)
{
    for (int i = 0; i < m_wrongAnswers.size(); ++i) {
        if (m_wrongAnswers[i].id == id) {
            m_wrongAnswers.removeAt(i);
            emit wrongAnswerRemoved(id);
            emit dataChanged();
            return true;
        }
    }
    return false;
}

bool WrongAnswerSet::updateWrongAnswer(const QString &id, const WrongAnswerItem &item)
{
    for (int i = 0; i < m_wrongAnswers.size(); ++i) {
        if (m_wrongAnswers[i].id == id) {
            m_wrongAnswers[i] = item;
            emit wrongAnswerUpdated(id, item);
            emit dataChanged();
            return true;
        }
    }
    return false;
}

WrongAnswerItem WrongAnswerSet::getWrongAnswer(const QString &id) const
{
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.id == id) {
            return item;
        }
    }
    return WrongAnswerItem();
}

QVector<WrongAnswerItem> WrongAnswerSet::getAllWrongAnswers() const
{
    return m_wrongAnswers;
}

QVector<WrongAnswerItem> WrongAnswerSet::getWrongAnswersBySubject(const QString &subject) const
{
    QVector<WrongAnswerItem> result;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.subject == subject) {
            result.append(item);
        }
    }
    return result;
}

QVector<WrongAnswerItem> WrongAnswerSet::getWrongAnswersByType(const QString &type) const
{
    QVector<WrongAnswerItem> result;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.questionType == type) {
            result.append(item);
        }
    }
    return result;
}

QVector<WrongAnswerItem> WrongAnswerSet::getWrongAnswersByDateRange(const QDateTime &from, const QDateTime &to) const
{
    QVector<WrongAnswerItem> result;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.timestamp >= from && item.timestamp <= to) {
            result.append(item);
        }
    }
    return result;
}

QVector<WrongAnswerItem> WrongAnswerSet::getUnresolvedWrongAnswers() const
{
    QVector<WrongAnswerItem> result;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (!item.isResolved) {
            result.append(item);
        }
    }
    return result;
}

QVector<WrongAnswerItem> WrongAnswerSet::getResolvedWrongAnswers() const
{
    QVector<WrongAnswerItem> result;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.isResolved) {
            result.append(item);
        }
    }
    return result;
}

bool WrongAnswerSet::markAsResolved(const QString &id)
{
    for (WrongAnswerItem &item : m_wrongAnswers) {
        if (item.id == id) {
            item.isResolved = true;
            item.reviewCount++;
            emit wrongAnswerUpdated(id, item);
            emit dataChanged();
            return true;
        }
    }
    return false;
}

bool WrongAnswerSet::markAsUnresolved(const QString &id)
{
    for (WrongAnswerItem &item : m_wrongAnswers) {
        if (item.id == id) {
            item.isResolved = false;
            emit wrongAnswerUpdated(id, item);
            emit dataChanged();
            return true;
        }
    }
    return false;
}

bool WrongAnswerSet::markAsResolved(const QStringList &ids)
{
    bool anyChanged = false;
    for (const QString &id : ids) {
        if (markAsResolved(id)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

bool WrongAnswerSet::markAsUnresolved(const QStringList &ids)
{
    bool anyChanged = false;
    for (const QString &id : ids) {
        if (markAsUnresolved(id)) {
            anyChanged = true;
        }
    }
    return anyChanged;
}

int WrongAnswerSet::getTotalCount() const
{
    return m_wrongAnswers.size();
}

int WrongAnswerSet::getResolvedCount() const
{
    int count = 0;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (item.isResolved) {
            count++;
        }
    }
    return count;
}

int WrongAnswerSet::getUnresolvedCount() const
{
    return getTotalCount() - getResolvedCount();
}

QStringList WrongAnswerSet::getAvailableSubjects() const
{
    QStringList subjects;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (!subjects.contains(item.subject)) {
            subjects.append(item.subject);
        }
    }
    subjects.sort();
    return subjects;
}

QStringList WrongAnswerSet::getAvailableTypes() const
{
    QStringList types;
    for (const WrongAnswerItem &item : m_wrongAnswers) {
        if (!types.contains(item.questionType)) {
            types.append(item.questionType);
        }
    }
    types.sort();
    return types;
}

QString WrongAnswerSet::exportToMarkdown() const
{
    QString markdown;
    markdown += "# 错题集合\n\n";
    markdown += QString("**生成时间**: %1\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    markdown += QString("**总题数**: %1\n\n").arg(m_wrongAnswers.size());
    markdown += QString("**已解决**: %1\n\n").arg(getResolvedCount());
    markdown += QString("**未解决**: %1\n\n").arg(getUnresolvedCount());
    markdown += "---\n\n";
    
    // 按科目分组
    QStringList subjects = getAvailableSubjects();
    for (const QString &subject : subjects) {
        QVector<WrongAnswerItem> subjectItems = getWrongAnswersBySubject(subject);
        if (subjectItems.isEmpty()) continue;
        
        markdown += QString("## %1\n\n").arg(subject);
        
        int questionNumber = 1;
        for (const WrongAnswerItem &item : subjectItems) {
            markdown += formatQuestionForMarkdown(item);
            markdown += "\n---\n\n";
        }
    }
    
    return markdown;
}

bool WrongAnswerSet::exportToMarkdownFile(const QString &filePath) const
{
    QString markdown = exportToMarkdown();
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << markdown;
    
    qDebug() << "Exported wrong answers to markdown file:" << filePath;
    return true;
}

void WrongAnswerSet::clear()
{
    m_wrongAnswers.clear();
    emit dataChanged();
}

bool WrongAnswerSet::removeResolvedItems()
{
    int originalSize = m_wrongAnswers.size();
    
    for (int i = m_wrongAnswers.size() - 1; i >= 0; --i) {
        if (m_wrongAnswers[i].isResolved) {
            QString id = m_wrongAnswers[i].id;
            m_wrongAnswers.removeAt(i);
            emit wrongAnswerRemoved(id);
        }
    }
    
    int removedCount = originalSize - m_wrongAnswers.size();
    if (removedCount > 0) {
        emit dataChanged();
    }
    
    return removedCount > 0;
}

QString WrongAnswerSet::generateUniqueId() const
{
    QString id;
    do {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    } while (getWrongAnswer(id).id == id); // 确保ID唯一
    
    return id;
}

bool WrongAnswerSet::isDuplicate(const WrongAnswerItem &item) const
{
    auto buildKey = [](const WrongAnswerItem &it) -> QString {
        QStringList normalizedChoices = it.choices;
        for (QString &choice : normalizedChoices) {
            choice = choice.trimmed();
        }

        QStringList normalizedImages;
        normalizedImages.reserve(it.images.size());
        for (auto imgIt = it.images.constBegin(); imgIt != it.images.constEnd(); ++imgIt) {
            const QString key = imgIt.key().trimmed();
            const QString value = imgIt.value().trimmed();
            if (!key.isEmpty() && !value.isEmpty()) {
                normalizedImages.append(key + "=" + value);
            }
        }
        normalizedImages.sort();

        QStringList normalizedCorrectAnswers = it.correctAnswers;
        for (QString &ans : normalizedCorrectAnswers) {
            ans = ans.trimmed();
        }
        normalizedCorrectAnswers.removeAll(QString());
        normalizedCorrectAnswers.sort();

        QString normalizedCorrectAnswer = it.correctAnswer.trimmed();

        return it.subject + "\n" +
               it.questionType + "\n" +
               it.questionText + "\n" +
               normalizedImages.join("|") + "\n" +
               normalizedChoices.join("\n") + "\n" +
               normalizedCorrectAnswer + "\n" +
               normalizedCorrectAnswers.join("|");
    };

    const QString key = buildKey(item);
    for (const WrongAnswerItem &existing : m_wrongAnswers) {
        if (buildKey(existing) == key) {
            return true;
        }
    }
    return false;
}

QString WrongAnswerSet::formatQuestionForMarkdown(const WrongAnswerItem &item) const
{
    QString markdown;
    
    // 题目标题
    QString statusIcon = item.isResolved ? "✅" : "❌";
    markdown += QString("### %1 [%2] %3\n\n").arg(statusIcon).arg(item.subject).arg(getQuestionTypeString(item.questionType));
    
    // 题目内容
    markdown += QString("**题目**: %1\n\n").arg(item.questionText);
    
    // 选项（如果有）
    if (!item.choices.isEmpty()) {
        markdown += "**选项**:\n\n";
        QStringList options = {"A", "B", "C", "D", "E", "F"};
        for (int i = 0; i < item.choices.size() && i < options.size(); ++i) {
            markdown += QString("%1. %2\n\n").arg(options[i]).arg(item.choices[i]);
        }
    }
    
    // 正确答案
    if (item.questionType == "FillBlank" || item.questionType == "MultiChoice") {
        markdown += QString("**正确答案**: %1\n\n").arg(item.correctAnswers.join(", "));
    } else {
        markdown += QString("**正确答案**: %1\n\n").arg(item.correctAnswer);
    }
    
    // 用户答案
    if (item.questionType == "FillBlank" || item.questionType == "MultiChoice") {
        markdown += QString("**你的答案**: %1\n\n").arg(item.userAnswers.join(", "));
    } else {
        markdown += QString("**你的答案**: %1\n\n").arg(item.userAnswer);
    }
    
    // 其他信息
    markdown += QString("**添加时间**: %1\n\n").arg(item.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    markdown += QString("**复习次数**: %1\n\n").arg(item.reviewCount);
    
    if (!item.note.isEmpty()) {
        markdown += QString("**备注**: %1\n\n").arg(item.note);
    }
    
    return markdown;
}

QString WrongAnswerSet::getQuestionTypeString(const QString &type) const
{
    if (type == "Choice") {
        return "选择题";
    } else if (type == "TrueOrFalse") {
        return "判断题";
    } else if (type == "FillBlank") {
        return "填空题";
    } else if (type == "MultiChoice") {
        return "多选题";
    } else {
        return "未知题型";
    }
}
