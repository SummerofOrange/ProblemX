#include "questionbank.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <random>

QuestionBank::QuestionBank()
{
}

QuestionBank::QuestionBank(const QString &subject)
    : m_subject(subject)
{
}

QVector<QuestionBankInfo> QuestionBank::getAllBanks() const
{
    QVector<QuestionBankInfo> allBanks;
    allBanks.append(m_choiceBanks);
    allBanks.append(m_trueOrFalseBanks);
    allBanks.append(m_fillBlankBanks);
    return allBanks;
}

void QuestionBank::addChoiceBank(const QuestionBankInfo &bank)
{
    m_choiceBanks.append(bank);
}

void QuestionBank::addTrueOrFalseBank(const QuestionBankInfo &bank)
{
    m_trueOrFalseBanks.append(bank);
}

void QuestionBank::addFillBlankBank(const QuestionBankInfo &bank)
{
    m_fillBlankBanks.append(bank);
}

void QuestionBank::updateBankInfo(QuestionType type, int index, const QuestionBankInfo &bank)
{
    QVector<QuestionBankInfo>& banks = getBanksByType(type);
    if (index >= 0 && index < banks.size()) {
        banks[index] = bank;
    }
}

void QuestionBank::setBankChosen(QuestionType type, int index, bool chosen)
{
    QVector<QuestionBankInfo>& banks = getBanksByType(type);
    if (index >= 0 && index < banks.size()) {
        banks[index].chosen = chosen;
    }
}

void QuestionBank::setBankChosenNum(QuestionType type, int index, int chosenNum)
{
    QVector<QuestionBankInfo>& banks = getBanksByType(type);
    if (index >= 0 && index < banks.size()) {
        banks[index].chosennum = qMin(chosenNum, banks[index].size);
    }
}

QList<Question> QuestionBank::loadSelectedQuestions(const QString &subjectPath, bool shuffleQuestions) const
{
    QList<Question> allQuestions;
    
    // Load from all selected banks
    for (const auto& bank : m_choiceBanks) {
        if (bank.chosen) {
            auto questions = loadQuestionsFromBank(subjectPath, bank, shuffleQuestions);
            allQuestions.append(questions);
        }
    }
    
    for (const auto& bank : m_trueOrFalseBanks) {
        if (bank.chosen) {
            auto questions = loadQuestionsFromBank(subjectPath, bank, shuffleQuestions);
            allQuestions.append(questions);
        }
    }
    
    for (const auto& bank : m_fillBlankBanks) {
        if (bank.chosen) {
            auto questions = loadQuestionsFromBank(subjectPath, bank, shuffleQuestions);
            allQuestions.append(questions);
        }
    }
    
    if (shuffleQuestions) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(allQuestions.begin(), allQuestions.end(), g);
    }
    
    return allQuestions;
}

QList<Question> QuestionBank::loadQuestionsFromBank(const QString &subjectPath, const QuestionBankInfo &bank, bool shuffleQuestions) const
{
    QString filePath = QDir(subjectPath).filePath(bank.src);
    if (!QFileInfo::exists(filePath)) {
        QString typeFolder;
        switch (bank.type) {
        case QuestionType::Choice:
            typeFolder = "Choice";
            break;
        case QuestionType::TrueOrFalse:
            typeFolder = "TrueorFalse";
            break;
        case QuestionType::FillBlank:
            typeFolder = "FillBlank";
            break;
        default:
            typeFolder = "Choice";
            break;
        }
        filePath = QDir(subjectPath).filePath(typeFolder + "/" + bank.src);
    }

    QList<Question> loadedQuestions = loadQuestionsFromFile(filePath);
    QList<Question> allQuestions;
    allQuestions.reserve(loadedQuestions.size());
    for (const auto &q : loadedQuestions) {
        if (q.getType() == bank.type) {
            allQuestions.append(q);
        }
    }
    
    if (shuffleQuestions) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(allQuestions.begin(), allQuestions.end(), g);
    }
    
    int numToSelect = qMin(bank.chosennum, allQuestions.size());
    if (numToSelect > 0) {
        allQuestions.resize(numToSelect);
    }
    
    return allQuestions;
}

QList<Question> QuestionBank::loadAllQuestionsFromBank(const QString &subjectPath, const QuestionBankInfo &bank) const
{
    QString filePath = QDir(subjectPath).filePath(bank.src);
    if (!QFileInfo::exists(filePath)) {
        QString typeFolder;
        switch (bank.type) {
        case QuestionType::Choice:
            typeFolder = "Choice";
            break;
        case QuestionType::TrueOrFalse:
            typeFolder = "TrueorFalse";
            break;
        case QuestionType::FillBlank:
            typeFolder = "FillBlank";
            break;
        default:
            typeFolder = "Choice";
            break;
        }
        filePath = QDir(subjectPath).filePath(typeFolder + "/" + bank.src);
    }

    const QList<Question> loadedQuestions = loadQuestionsFromFile(filePath);
    QList<Question> allQuestions;
    allQuestions.reserve(loadedQuestions.size());
    for (const auto &q : loadedQuestions) {
        if (q.getType() == bank.type) {
            allQuestions.append(q);
        }
    }
    return allQuestions;
}

QList<Question> QuestionBank::loadQuestionsFromFile(const QString &filePath) const
{
    QList<Question> questions;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open question file:" << filePath;
        return questions;
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in file" << filePath << ":" << error.errorString();
        return questions;
    }
    
    QJsonObject root = doc.object();
    if (root.contains("data") && root["data"].isArray()) {
        QJsonArray dataArray = root["data"].toArray();
        const QString baseDir = QFileInfo(filePath).absolutePath();
        for (const QJsonValue &value : dataArray) {
            if (value.isObject()) {
                Question question(value.toObject());
                if (question.hasImage()) {
                    QMap<QString, QString> images = question.getImages();
                    for (auto it = images.begin(); it != images.end(); ++it) {
                        const QString p = it.value().trimmed();
                        if (p.isEmpty()) {
                            continue;
                        }
                        if (p.startsWith("http://", Qt::CaseInsensitive) ||
                            p.startsWith("https://", Qt::CaseInsensitive) ||
                            p.startsWith("file://", Qt::CaseInsensitive) ||
                            QDir::isAbsolutePath(p)) {
                            continue;
                        }
                        it.value() = QDir(baseDir).filePath(p);
                    }
                    question.setImages(images);
                }
                questions.append(question);
            }
        }
    }
    
    return questions;
}

QJsonObject QuestionBank::toJson() const
{
    QJsonObject json;
    
    // Choice banks
    QJsonArray choiceArray;
    for (const auto& bank : m_choiceBanks) {
        QJsonObject bankObj;
        bankObj["name"] = bank.name;
        bankObj["src"] = bank.src;
        bankObj["size"] = bank.size;
        bankObj["chosennum"] = bank.chosennum;
        bankObj["chosen"] = bank.chosen;
        choiceArray.append(bankObj);
    }
    json["Choice"] = choiceArray;
    
    // TrueOrFalse banks
    QJsonArray trueOrFalseArray;
    for (const auto& bank : m_trueOrFalseBanks) {
        QJsonObject bankObj;
        bankObj["name"] = bank.name;
        bankObj["src"] = bank.src;
        bankObj["size"] = bank.size;
        bankObj["chosennum"] = bank.chosennum;
        bankObj["chosen"] = bank.chosen;
        trueOrFalseArray.append(bankObj);
    }
    json["TrueorFalse"] = trueOrFalseArray;
    
    // FillBlank banks
    QJsonArray fillBlankArray;
    for (const auto& bank : m_fillBlankBanks) {
        QJsonObject bankObj;
        bankObj["name"] = bank.name;
        bankObj["src"] = bank.src;
        bankObj["size"] = bank.size;
        bankObj["chosennum"] = bank.chosennum;
        bankObj["chosen"] = bank.chosen;
        fillBlankArray.append(bankObj);
    }
    json["FillBlank"] = fillBlankArray;
    
    return json;
}

void QuestionBank::fromJson(const QJsonObject &json)
{
    // Load Choice banks
    m_choiceBanks.clear();
    if (json.contains("Choice") && json["Choice"].isArray()) {
        QJsonArray choiceArray = json["Choice"].toArray();
        for (const QJsonValue &value : choiceArray) {
            if (value.isObject()) {
                QJsonObject bankObj = value.toObject();
                QuestionBankInfo bank;
                bank.name = bankObj["name"].toString();
                bank.src = bankObj["src"].toString();
                bank.size = bankObj["size"].toInt();
                bank.chosennum = bankObj["chosennum"].toInt();
                bank.chosen = bankObj["chosen"].toBool();
                bank.type = QuestionType::Choice;
                m_choiceBanks.append(bank);
            }
        }
    }
    
    // Load TrueOrFalse banks
    m_trueOrFalseBanks.clear();
    if (json.contains("TrueorFalse") && json["TrueorFalse"].isArray()) {
        QJsonArray trueOrFalseArray = json["TrueorFalse"].toArray();
        for (const QJsonValue &value : trueOrFalseArray) {
            if (value.isObject()) {
                QJsonObject bankObj = value.toObject();
                QuestionBankInfo bank;
                bank.name = bankObj["name"].toString();
                bank.src = bankObj["src"].toString();
                bank.size = bankObj["size"].toInt();
                bank.chosennum = bankObj["chosennum"].toInt();
                bank.chosen = bankObj["chosen"].toBool();
                bank.type = QuestionType::TrueOrFalse;
                m_trueOrFalseBanks.append(bank);
            }
        }
    }
    
    // Load FillBlank banks
    m_fillBlankBanks.clear();
    if (json.contains("FillBlank") && json["FillBlank"].isArray()) {
        QJsonArray fillBlankArray = json["FillBlank"].toArray();
        for (const QJsonValue &value : fillBlankArray) {
            if (value.isObject()) {
                QJsonObject bankObj = value.toObject();
                QuestionBankInfo bank;
                bank.name = bankObj["name"].toString();
                bank.src = bankObj["src"].toString();
                bank.size = bankObj["size"].toInt();
                bank.chosennum = bankObj["chosennum"].toInt();
                bank.chosen = bankObj["chosen"].toBool();
                bank.type = QuestionType::FillBlank;
                m_fillBlankBanks.append(bank);
            }
        }
    }
}

int QuestionBank::getTotalSelectedQuestions() const
{
    int total = 0;
    
    for (const auto& bank : m_choiceBanks) {
        if (bank.chosen) {
            total += bank.chosennum;
        }
    }
    
    for (const auto& bank : m_trueOrFalseBanks) {
        if (bank.chosen) {
            total += bank.chosennum;
        }
    }
    
    for (const auto& bank : m_fillBlankBanks) {
        if (bank.chosen) {
            total += bank.chosennum;
        }
    }
    
    return total;
}

bool QuestionBank::hasSelectedBanks() const
{
    for (const auto& bank : m_choiceBanks) {
        if (bank.chosen) return true;
    }
    
    for (const auto& bank : m_trueOrFalseBanks) {
        if (bank.chosen) return true;
    }
    
    
    for (const auto& bank : m_fillBlankBanks) {
        if (bank.chosen) return true;
    }
    
    return false;
}

QVector<QuestionBankInfo>& QuestionBank::getBanksByType(QuestionType type)
{
    switch (type) {
    case QuestionType::Choice:
        return m_choiceBanks;
    case QuestionType::TrueOrFalse:
        return m_trueOrFalseBanks;
    case QuestionType::FillBlank:
        return m_fillBlankBanks;
    default:
        return m_choiceBanks;
    }
}

const QVector<QuestionBankInfo>& QuestionBank::getBanksByType(QuestionType type) const
{
    switch (type) {
    case QuestionType::Choice:
        return m_choiceBanks;
    case QuestionType::TrueOrFalse:
        return m_trueOrFalseBanks;
    case QuestionType::FillBlank:
        return m_fillBlankBanks;
    default:
        return m_choiceBanks;
    }
}
