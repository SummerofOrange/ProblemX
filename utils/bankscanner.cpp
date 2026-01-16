#include "bankscanner.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// 初始化静态成员变量
QString BankScanner::s_lastError;

static QString questionTypeToString(QuestionType type)
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
    return "Choice";
}

static bool readAndValidateBankFile(const QString &filePath, QJsonDocument *docOut, QString *errorOut)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (errorOut) {
            *errorOut = QString("题库文件不存在: %1").arg(filePath);
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QString("无法打开题库文件: %1").arg(filePath);
        }
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        if (errorOut) {
            *errorOut = QString("JSON解析错误: %1").arg(error.errorString());
        }
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("data") || !root["data"].isArray()) {
        if (errorOut) {
            *errorOut = "题库文件格式错误: 缺少data数组";
        }
        return false;
    }

    QJsonArray dataArray = root["data"].toArray();
    if (dataArray.isEmpty()) {
        if (errorOut) {
            *errorOut = "题库文件不包含任何题目";
        }
        return false;
    }

    if (docOut) {
        *docOut = doc;
    }
    return true;
}

static QMap<QuestionType, int> countQuestionsByType(const QJsonDocument &doc)
{
    QMap<QuestionType, int> counts;
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();

    for (const auto &value : dataArray) {
        if (!value.isObject()) {
            continue;
        }
        QJsonObject obj = value.toObject();
        QString typeStr = obj.value("type").toString();
        if (typeStr == "Choice") {
            counts[QuestionType::Choice] = counts.value(QuestionType::Choice, 0) + 1;
        } else if (typeStr == "TrueorFalse" || typeStr == "TrueOrFalse") {
            counts[QuestionType::TrueOrFalse] = counts.value(QuestionType::TrueOrFalse, 0) + 1;
        } else if (typeStr == "FillBlank") {
            counts[QuestionType::FillBlank] = counts.value(QuestionType::FillBlank, 0) + 1;
        } else if (typeStr == "MultipleChoice") {
            counts[QuestionType::MultipleChoice] = counts.value(QuestionType::MultipleChoice, 0) + 1;
        }
    }

    return counts;
}

QuestionBank BankScanner::scanSubjectDirectory(const QString &dirPath, const QString &subjectName)
{
    s_lastError.clear();
    QuestionBank bank(subjectName);
    QDir dir(dirPath);
    
    if (!dir.exists()) {
        s_lastError = QString("科目目录不存在: %1").arg(dirPath);
        qWarning() << s_lastError;
        return bank;
    }

    QString firstError;
    QDirIterator it(dirPath, QStringList() << "*.json", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();

        QJsonDocument doc;
        QString error;
        if (!readAndValidateBankFile(filePath, &doc, &error)) {
            if (firstError.isEmpty()) {
                firstError = error;
            }
            continue;
        }

        QMap<QuestionType, int> counts = countQuestionsByType(doc);
        QMap<QuestionType, int> supportedCounts;
        supportedCounts[QuestionType::Choice] = counts.value(QuestionType::Choice, 0);
        supportedCounts[QuestionType::TrueOrFalse] = counts.value(QuestionType::TrueOrFalse, 0);
        supportedCounts[QuestionType::FillBlank] = counts.value(QuestionType::FillBlank, 0);

        int supportedTypesInFile = 0;
        for (auto itCount = supportedCounts.constBegin(); itCount != supportedCounts.constEnd(); ++itCount) {
            if (itCount.value() > 0) {
                supportedTypesInFile++;
            }
        }
        if (supportedTypesInFile == 0) {
            continue;
        }

        QString relPath = QDir(dirPath).relativeFilePath(filePath);
        relPath = QDir::fromNativeSeparators(relPath);
        QString baseDisplayName = QFileInfo(relPath).completeBaseName();

        auto addBank = [&](QuestionType type, int count) {
            if (count <= 0) {
                return;
            }
            QuestionBankInfo bankInfo;
            QString srcToStore = relPath;
            QString typeFolder = questionTypeToString(type);
            QString prefix = typeFolder + "/";
            if (supportedTypesInFile == 1 && relPath.startsWith(prefix) && !relPath.mid(prefix.size()).contains('/')) {
                srcToStore = relPath.mid(prefix.size());
            }
            bankInfo.src = srcToStore;
            bankInfo.type = type;
            bankInfo.chosen = false;
            bankInfo.chosennum = 1;
            bankInfo.size = count;

            if (supportedTypesInFile > 1) {
                bankInfo.name = QString("%1 (%2)").arg(baseDisplayName, questionTypeToString(type));
            } else {
                bankInfo.name = baseDisplayName;
            }

            if (type == QuestionType::Choice) {
                bank.addChoiceBank(bankInfo);
            } else if (type == QuestionType::TrueOrFalse) {
                bank.addTrueOrFalseBank(bankInfo);
            } else if (type == QuestionType::FillBlank) {
                bank.addFillBlankBank(bankInfo);
            }
        };

        addBank(QuestionType::Choice, supportedCounts.value(QuestionType::Choice, 0));
        addBank(QuestionType::TrueOrFalse, supportedCounts.value(QuestionType::TrueOrFalse, 0));
        addBank(QuestionType::FillBlank, supportedCounts.value(QuestionType::FillBlank, 0));
    }
    
    // 检查是否找到了题库文件
    int totalBanks = bank.getChoiceBanks().size() + bank.getTrueOrFalseBanks().size() + bank.getFillBlankBanks().size();
    if (totalBanks == 0) {
        s_lastError = firstError.isEmpty()
            ? QString("在科目文件夹 '%1' 中未找到有效的题库文件").arg(subjectName)
            : firstError;
        qWarning() << s_lastError;
    }
    
    return bank;
}

bool BankScanner::validateBankFile(const QString &filePath)
{
    s_lastError.clear();

    QJsonDocument doc;
    QString error;
    bool ok = readAndValidateBankFile(filePath, &doc, &error);
    if (!ok) {
        s_lastError = error;
    }
    return ok;
}

int BankScanner::getBankQuestionCount(const QString &filePath)
{
    QString error;
    QJsonDocument doc;
    if (!readAndValidateBankFile(filePath, &doc, &error)) {
        return 0;
    }

    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    return dataArray.size();
}

QString BankScanner::getLastError()
{
    return s_lastError;
}
