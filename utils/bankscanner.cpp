#include "bankscanner.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// 初始化静态成员变量
QString BankScanner::s_lastError;

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
    
    // 扫描Choice文件夹
    QDir choiceDir(dir.filePath("Choice"));
    if (choiceDir.exists()) {
        QStringList choiceFiles = choiceDir.entryList(QStringList() << "*.json", QDir::Files);
        qDebug() << "找到选择题文件:" << choiceFiles;
        
        for (const QString &fileName : choiceFiles) {
            QuestionBankInfo bankInfo;
            bankInfo.name = QFileInfo(fileName).baseName();
            bankInfo.src = fileName;
            bankInfo.type = QuestionType::Choice;
            bankInfo.chosen = false;
            bankInfo.chosennum = 1;
            
            // 读取文件获取题目数量
            QString filePath = choiceDir.filePath(fileName);
            bankInfo.size = getBankQuestionCount(filePath);
            
            if (bankInfo.size > 0) {
                bank.addChoiceBank(bankInfo);
                qDebug() << "添加选择题库:" << bankInfo.name << "题目数量:" << bankInfo.size;
            } else {
                qWarning() << "选择题库文件无效或为空:" << filePath;
            }
        }
    } else {
        qDebug() << "选择题目录不存在:" << choiceDir.path();
    }
    
    // 扫描TrueorFalse文件夹
    QDir trueOrFalseDir(dir.filePath("TrueorFalse"));
    if (trueOrFalseDir.exists()) {
        QStringList trueOrFalseFiles = trueOrFalseDir.entryList(QStringList() << "*.json", QDir::Files);
        qDebug() << "找到判断题文件:" << trueOrFalseFiles;
        
        for (const QString &fileName : trueOrFalseFiles) {
            QuestionBankInfo bankInfo;
            bankInfo.name = QFileInfo(fileName).baseName();
            bankInfo.src = fileName;
            bankInfo.type = QuestionType::TrueOrFalse;
            bankInfo.chosen = false;
            bankInfo.chosennum = 1;
            
            // 读取文件获取题目数量
            QString filePath = trueOrFalseDir.filePath(fileName);
            bankInfo.size = getBankQuestionCount(filePath);
            
            if (bankInfo.size > 0) {
                bank.addTrueOrFalseBank(bankInfo);
                qDebug() << "添加判断题库:" << bankInfo.name << "题目数量:" << bankInfo.size;
            } else {
                qWarning() << "判断题库文件无效或为空:" << filePath;
            }
        }
    } else {
        qDebug() << "判断题目录不存在:" << trueOrFalseDir.path();
    }
    
    // 扫描FillBlank文件夹
    QDir fillBlankDir(dir.filePath("FillBlank"));
    if (fillBlankDir.exists()) {
        QStringList fillBlankFiles = fillBlankDir.entryList(QStringList() << "*.json", QDir::Files);
        qDebug() << "找到填空题文件:" << fillBlankFiles;
        
        for (const QString &fileName : fillBlankFiles) {
            QuestionBankInfo bankInfo;
            bankInfo.name = QFileInfo(fileName).baseName();
            bankInfo.src = fileName;
            bankInfo.type = QuestionType::FillBlank;
            bankInfo.chosen = false;
            bankInfo.chosennum = 1;
            
            // 读取文件获取题目数量
            QString filePath = fillBlankDir.filePath(fileName);
            bankInfo.size = getBankQuestionCount(filePath);
            
            if (bankInfo.size > 0) {
                bank.addFillBlankBank(bankInfo);
                qDebug() << "添加填空题库:" << bankInfo.name << "题目数量:" << bankInfo.size;
            } else {
                qWarning() << "填空题库文件无效或为空:" << filePath;
            }
        }
    } else {
        qDebug() << "填空题目录不存在:" << fillBlankDir.path();
    }
    
    // 检查是否找到了题库文件
    int totalBanks = bank.getChoiceBanks().size() + bank.getTrueOrFalseBanks().size() + bank.getFillBlankBanks().size();
    if (totalBanks == 0) {
        s_lastError = QString("在科目文件夹 '%1' 中未找到有效的题库文件").arg(subjectName);
        qWarning() << s_lastError;
    }
    
    return bank;
}

bool BankScanner::validateBankFile(const QString &filePath)
{
    s_lastError.clear();
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        s_lastError = QString("题库文件不存在: %1").arg(filePath);
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        s_lastError = QString("无法打开题库文件: %1").arg(filePath);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        s_lastError = QString("JSON解析错误: %1").arg(error.errorString());
        return false;
    }
    
    QJsonObject root = doc.object();
    if (!root.contains("data") || !root["data"].isArray()) {
        s_lastError = "题库文件格式错误: 缺少data数组";
        return false;
    }
    
    QJsonArray dataArray = root["data"].toArray();
    if (dataArray.isEmpty()) {
        s_lastError = "题库文件不包含任何题目";
        return false;
    }
    
    return true;
}

int BankScanner::getBankQuestionCount(const QString &filePath)
{
    if (!validateBankFile(filePath)) {
        return 0;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    return dataArray.size();
}

QString BankScanner::getLastError()
{
    return s_lastError;
}