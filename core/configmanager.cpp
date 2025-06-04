#include "configmanager.h"
#include "../utils/jsonutils.h"
#include "../utils/bankscanner.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QDebug>

ConfigManager::ConfigManager()
{
    // 程序启动时自动加载配置文件
    initializeConfig();
}

bool ConfigManager::loadConfig(const QString &configPath)
{
    m_lastError.clear();
    
    QJsonObject config = JsonUtils::loadJsonFromFile(configPath);
    if (config.isEmpty()) {
        m_lastError = JsonUtils::getLastError();
        return false;
    }
    
    // Load current subject
    if (config.contains("Subject")) {
        m_currentSubject = config["Subject"].toString();
    }
    
    // Load question banks
    if (config.contains("QuestionBank")) {
        parseQuestionBanks(config["QuestionBank"].toObject());
    }
    
    // Load checkpoint if exists
    if (config.contains("last_checkpoint")) {
        parseCheckpoint(config["last_checkpoint"].toObject());
    }
    
    return true;
}

bool ConfigManager::saveConfig(const QString &configPath)
{
    m_lastError.clear();
    
    QJsonObject config;
    config["Subject"] = m_currentSubject;
    
    // Save question banks
    config["QuestionBank"] = questionBanksToJson();
    
    // Save checkpoint if exists
    if (hasCheckpoint()) {
        config["last_checkpoint"] = checkpointToJson();
    }
    
    bool success = JsonUtils::saveJsonToFile(config, configPath);
    if (!success) {
        m_lastError = JsonUtils::getLastError();
    }
    
    return success;
}

QStringList ConfigManager::getAvailableSubjects() const
{
    return m_questionBanks.keys();
}

QuestionBank ConfigManager::getQuestionBank(const QString &subject) const
{
    return m_questionBanks.value(subject, QuestionBank());
}

void ConfigManager::setQuestionBank(const QString &subject, const QuestionBank &bank)
{
    m_questionBanks[subject] = bank;
}

bool ConfigManager::hasQuestionBank(const QString &subject) const
{
    return m_questionBanks.contains(subject);
}

bool ConfigManager::hasCheckpoint() const
{
    return m_checkpoint.trueOrFalseCheck > 0 || 
           m_checkpoint.choiceCheck > 0 || 
           m_checkpoint.fillBlankCheck > 0 ||
           !m_checkpoint.trueOrFalseData.isEmpty() ||
           !m_checkpoint.choiceData.isEmpty() ||
           !m_checkpoint.fillBlankData.isEmpty();
}

void ConfigManager::clearCheckpoint()
{
    m_checkpoint = CheckpointData();
}

void ConfigManager::addSubject(const QString &subject, const QString &path)
{
    // 存储科目路径
    m_subjectPaths[subject] = path;
    
    // 使用BankScanner自动扫描科目目录并加载题库数据
    QuestionBank bank = BankScanner::scanSubjectDirectory(path, subject);
    
    // 如果扫描失败，创建空的题库作为备选
    if (bank.getChoiceBanks().isEmpty() && 
        bank.getTrueOrFalseBanks().isEmpty() && 
        bank.getFillBlankBanks().isEmpty()) {
        qDebug() << "No question banks found for subject:" << subject << "at path:" << path;
        qDebug() << "Creating empty question bank as fallback";
        bank.setSubject(subject);
    }
    
    m_questionBanks[subject] = bank;
    qDebug() << "Added subject:" << subject << "with" 
             << bank.getChoiceBanks().size() << "choice banks,"
             << bank.getTrueOrFalseBanks().size() << "true/false banks,"
             << bank.getFillBlankBanks().size() << "fill blank banks";
}

void ConfigManager::removeSubject(const QString &subject)
{
    m_questionBanks.remove(subject);
    m_subjectPaths.remove(subject); // 同时移除科目路径
    if (m_currentSubject == subject) {
        m_currentSubject.clear();
    }
}

QString ConfigManager::getSubjectPath(const QString &subject) const
{
    // 如果存在存储的路径，则返回存储的路径，否则返回默认路径
    if (m_subjectPaths.contains(subject)) {
        return m_subjectPaths[subject];
    }
    return QString("Subject/%1").arg(subject);
}

QString ConfigManager::getWrongAnswersPath(const QString &subject) const
{
    return QString("WA/%1").arg(subject);
}

bool ConfigManager::isValidConfig() const
{
    if (m_currentSubject.isEmpty()) {
        return false;
    }
    
    if (!hasQuestionBank(m_currentSubject)) {
        return false;
    }
    
    return true;
}

void ConfigManager::createDefaultConfig()
{
    m_currentSubject = "C++";
    m_questionBanks.clear();
    m_subjectPaths.clear();
    m_checkpoint = CheckpointData();
    
    // 检查Subject目录下是否有可用的科目
    QDir subjectDir("Subject");
    if (subjectDir.exists()) {
        QStringList subjects = subjectDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!subjects.isEmpty()) {
            // 如果有可用科目，设置第一个为当前科目
            m_currentSubject = subjects.first();
            qDebug() << "Found subjects in Subject directory, setting default to:" << m_currentSubject;
        }
    }
    
    qDebug() << "Default configuration created with subject:" << m_currentSubject;
}

bool ConfigManager::copyFromReference(const QString &referencePath)
{
    m_lastError.clear();
    
    if (!QFileInfo::exists(referencePath)) {
        m_lastError = QString("Reference config file not found: %1").arg(referencePath);
        return false;
    }
    
    return loadConfig(referencePath);
}

void ConfigManager::initializeConfig()
{
    const QString configPath = "config.json";
    
    // 检查根目录是否存在config.json
    if (QFileInfo::exists(configPath)) {
        // 存在则直接加载
        if (!loadConfig(configPath)) {
            qDebug() << "Failed to load existing config.json:" << m_lastError;
            // 加载失败，创建默认配置
            createDefaultConfigFile();
        }
    } else {
        // 不存在则创建默认配置文件
        qDebug() << "config.json not found, creating default configuration";
        createDefaultConfigFile();
    }
}

void ConfigManager::createDefaultConfigFile()
{
    // 创建默认配置
    createDefaultConfig();
    
    // 保存到根目录的config.json
    if (!saveConfig("config.json")) {
        qDebug() << "Failed to create default config.json:" << m_lastError;
    } else {
        qDebug() << "Default config.json created successfully";
    }
}

void ConfigManager::parseQuestionBanks(const QJsonObject &json)
{
    m_questionBanks.clear();
    m_subjectPaths.clear(); // 清除现有的路径映射
    
    if (!json.contains("Subject") || !json["Subject"].isObject()) {
        return;
    }
    
    QJsonObject subjects = json["Subject"].toObject();
    for (auto it = subjects.begin(); it != subjects.end(); ++it) {
        QString subjectName = it.key();
        QJsonObject subjectData = it.value().toObject();
        
        // 加载科目路径
        QString subjectPath;
        if (subjectData.contains("path") && subjectData["path"].isString()) {
            subjectPath = subjectData["path"].toString();
        } else {
            // 如果没有路径信息，使用默认路径
            subjectPath = QString("Subject/%1").arg(subjectName);
        }
        m_subjectPaths[subjectName] = subjectPath;
        
        // 实时扫描题库目录，获取最新的题库信息
        QuestionBank bank = BankScanner::scanSubjectDirectory(subjectPath, subjectName);
        
        // 如果扫描失败，创建空的题库作为备选
        if (bank.getChoiceBanks().isEmpty() && 
            bank.getTrueOrFalseBanks().isEmpty() && 
            bank.getFillBlankBanks().isEmpty()) {
            qDebug() << "No question banks found for subject:" << subjectName << "at path:" << subjectPath;
            qDebug() << "Creating empty question bank as fallback";
            bank.setSubject(subjectName);
        }
        
        m_questionBanks[subjectName] = bank;
        qDebug() << "Loaded subject:" << subjectName << "with" 
                 << bank.getChoiceBanks().size() << "choice banks,"
                 << bank.getTrueOrFalseBanks().size() << "true/false banks,"
                 << bank.getFillBlankBanks().size() << "fill blank banks";
    }
}

QJsonObject ConfigManager::questionBanksToJson() const
{
    QJsonObject json;
    QJsonObject subjects;
    
    for (auto it = m_questionBanks.begin(); it != m_questionBanks.end(); ++it) {
        QJsonObject subjectData = it.value().toJson();
        
        // 保存科目路径
        if (m_subjectPaths.contains(it.key())) {
            subjectData["path"] = m_subjectPaths[it.key()];
        }
        
        subjects[it.key()] = subjectData;
    }
    
    json["Subject"] = subjects;
    return json;
}

void ConfigManager::parseCheckpoint(const QJsonObject &json)
{
    m_checkpoint = CheckpointData();
    
    // Parse TrueOrFalse checkpoint
    if (json.contains("TrueorFalse") && json["TrueorFalse"].isObject()) {
        QJsonObject tfObj = json["TrueorFalse"].toObject();
        m_checkpoint.trueOrFalseCheck = tfObj["check"].toInt();
        
        if (tfObj.contains("order") && tfObj["order"].isArray()) {
            QJsonArray orderArray = tfObj["order"].toArray();
            for (const QJsonValue &value : orderArray) {
                m_checkpoint.trueOrFalseOrder.append(value.toInt());
            }
        }
        
        if (tfObj.contains("data") && tfObj["data"].isArray()) {
            QJsonArray dataArray = tfObj["data"].toArray();
            for (const QJsonValue &value : dataArray) {
                if (value.isObject()) {
                    Question question(value.toObject());
                    m_checkpoint.trueOrFalseData.append(question);
                }
            }
        }
    }
    
    // Parse Choice checkpoint
    if (json.contains("Choice") && json["Choice"].isObject()) {
        QJsonObject choiceObj = json["Choice"].toObject();
        m_checkpoint.choiceCheck = choiceObj["check"].toInt();
        
        if (choiceObj.contains("order") && choiceObj["order"].isArray()) {
            QJsonArray orderArray = choiceObj["order"].toArray();
            for (const QJsonValue &value : orderArray) {
                m_checkpoint.choiceOrder.append(value.toInt());
            }
        }
        
        if (choiceObj.contains("data") && choiceObj["data"].isArray()) {
            QJsonArray dataArray = choiceObj["data"].toArray();
            for (const QJsonValue &value : dataArray) {
                if (value.isObject()) {
                    Question question(value.toObject());
                    m_checkpoint.choiceData.append(question);
                }
            }
        }
    }
    
    // Parse FillBlank checkpoint
    if (json.contains("FillBlank") && json["FillBlank"].isObject()) {
        QJsonObject fbObj = json["FillBlank"].toObject();
        m_checkpoint.fillBlankCheck = fbObj["check"].toInt();
        
        if (fbObj.contains("order") && fbObj["order"].isArray()) {
            QJsonArray orderArray = fbObj["order"].toArray();
            for (const QJsonValue &value : orderArray) {
                m_checkpoint.fillBlankOrder.append(value.toInt());
            }
        }
        
        if (fbObj.contains("data") && fbObj["data"].isArray()) {
            QJsonArray dataArray = fbObj["data"].toArray();
            for (const QJsonValue &value : dataArray) {
                if (value.isObject()) {
                    Question question(value.toObject());
                    m_checkpoint.fillBlankData.append(question);
                }
            }
        }
    }
    
    // Parse wrong answers
    if (json.contains("WA") && json["WA"].isObject()) {
        QJsonObject waObj = json["WA"].toObject();
        if (waObj.contains("data") && waObj["data"].isArray()) {
            QJsonArray dataArray = waObj["data"].toArray();
            for (const QJsonValue &value : dataArray) {
                if (value.isObject()) {
                    Question question(value.toObject());
                    m_checkpoint.wrongAnswers.append(question);
                }
            }
        }
    }
    
    // Parse answer status data
    if (json.contains("AnswerStatus") && json["AnswerStatus"].isObject()) {
        QJsonObject statusObj = json["AnswerStatus"].toObject();
        
        // 解析答题标记
        if (statusObj.contains("answered") && statusObj["answered"].isArray()) {
            QJsonArray answeredArray = statusObj["answered"].toArray();
            for (const QJsonValue &value : answeredArray) {
                m_checkpoint.answeredFlags.append(value.toBool());
            }
        }
        
        // 解析用户单选答案
        if (statusObj.contains("userAnswers") && statusObj["userAnswers"].isArray()) {
            QJsonArray userAnswersArray = statusObj["userAnswers"].toArray();
            for (const QJsonValue &value : userAnswersArray) {
                m_checkpoint.userAnswers.append(value.toString());
            }
        }
        
        // 解析用户多选答案
        if (statusObj.contains("userMultiAnswers") && statusObj["userMultiAnswers"].isArray()) {
            QJsonArray userMultiAnswersArray = statusObj["userMultiAnswers"].toArray();
            for (const QJsonValue &value : userMultiAnswersArray) {
                if (value.isArray()) {
                    QJsonArray answerArray = value.toArray();
                    QStringList answers;
                    for (const QJsonValue &answerValue : answerArray) {
                        answers.append(answerValue.toString());
                    }
                    m_checkpoint.userMultiAnswers.append(answers);
                } else {
                    m_checkpoint.userMultiAnswers.append(QStringList());
                }
            }
        }
        
        // 解析正确性标记
        if (statusObj.contains("correct") && statusObj["correct"].isArray()) {
            QJsonArray correctArray = statusObj["correct"].toArray();
            for (const QJsonValue &value : correctArray) {
                m_checkpoint.correctFlags.append(value.toBool());
            }
        }
        
        // 解析统计数据
        if (statusObj.contains("correctCount")) {
            m_checkpoint.correctCount = statusObj["correctCount"].toInt();
        }
        if (statusObj.contains("wrongCount")) {
            m_checkpoint.wrongCount = statusObj["wrongCount"].toInt();
        }
    }
}

QJsonObject ConfigManager::checkpointToJson() const
{
    QJsonObject json;
    
    // TrueOrFalse checkpoint
    QJsonObject tfObj;
    tfObj["check"] = m_checkpoint.trueOrFalseCheck;
    
    QJsonArray tfOrderArray;
    for (int order : m_checkpoint.trueOrFalseOrder) {
        tfOrderArray.append(order);
    }
    tfObj["order"] = tfOrderArray;
    
    QJsonArray tfDataArray;
    for (const Question &question : m_checkpoint.trueOrFalseData) {
        tfDataArray.append(question.toJson());
    }
    tfObj["data"] = tfDataArray;
    
    json["TrueorFalse"] = tfObj;
    
    // Choice checkpoint
    QJsonObject choiceObj;
    choiceObj["check"] = m_checkpoint.choiceCheck;
    
    QJsonArray choiceOrderArray;
    for (int order : m_checkpoint.choiceOrder) {
        choiceOrderArray.append(order);
    }
    choiceObj["order"] = choiceOrderArray;
    
    QJsonArray choiceDataArray;
    for (const Question &question : m_checkpoint.choiceData) {
        choiceDataArray.append(question.toJson());
    }
    choiceObj["data"] = choiceDataArray;
    
    json["Choice"] = choiceObj;
    
    // FillBlank checkpoint
    QJsonObject fbObj;
    fbObj["check"] = m_checkpoint.fillBlankCheck;
    
    QJsonArray fbOrderArray;
    for (int order : m_checkpoint.fillBlankOrder) {
        fbOrderArray.append(order);
    }
    fbObj["order"] = fbOrderArray;
    
    QJsonArray fbDataArray;
    for (const Question &question : m_checkpoint.fillBlankData) {
        fbDataArray.append(question.toJson());
    }
    fbObj["data"] = fbDataArray;
    
    json["FillBlank"] = fbObj;
    
    // Wrong answers
    QJsonObject waObj;
    QJsonArray waDataArray;
    for (const Question &question : m_checkpoint.wrongAnswers) {
        waDataArray.append(question.toJson());
    }
    waObj["data"] = waDataArray;
    
    json["WA"] = waObj;
    
    // 保存答题状态数据
    QJsonObject statusObj;
    
    // 答题标记
    QJsonArray answeredArray;
    for (bool answered : m_checkpoint.answeredFlags) {
        answeredArray.append(answered);
    }
    statusObj["answered"] = answeredArray;
    
    // 用户单选答案
    QJsonArray userAnswersArray;
    for (const QString &answer : m_checkpoint.userAnswers) {
        userAnswersArray.append(answer);
    }
    statusObj["userAnswers"] = userAnswersArray;
    
    // 用户多选答案
    QJsonArray userMultiAnswersArray;
    for (const QStringList &answers : m_checkpoint.userMultiAnswers) {
        QJsonArray answerArray;
        for (const QString &answer : answers) {
            answerArray.append(answer);
        }
        userMultiAnswersArray.append(answerArray);
    }
    statusObj["userMultiAnswers"] = userMultiAnswersArray;
    
    // 正确性标记
    QJsonArray correctArray;
    for (bool correct : m_checkpoint.correctFlags) {
        correctArray.append(correct);
    }
    statusObj["correct"] = correctArray;
    
    // 统计数据
    statusObj["correctCount"] = m_checkpoint.correctCount;
    statusObj["wrongCount"] = m_checkpoint.wrongCount;
    
    json["AnswerStatus"] = statusObj;
    
    return json;
}