#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include "../models/questionbank.h"

struct CheckpointData {
    int trueOrFalseCheck;
    int choiceCheck;
    int fillBlankCheck;
    QVector<int> trueOrFalseOrder;
    QVector<int> choiceOrder;
    QVector<int> fillBlankOrder;
    QList<Question> trueOrFalseData;
    QList<Question> choiceData;
    QList<Question> fillBlankData;
    QList<Question> wrongAnswers;
    
    // 答题状态数据
    QVector<bool> answeredFlags;        // 每道题是否已回答
    QVector<QString> userAnswers;       // 用户的单选答案
    QVector<QStringList> userMultiAnswers; // 用户的多选答案
    QVector<bool> correctFlags;         // 每道题是否答对
    int correctCount;                   // 答对题目数
    int wrongCount;                     // 答错题目数
    
    CheckpointData() : trueOrFalseCheck(0), choiceCheck(0), fillBlankCheck(0), 
                      correctCount(0), wrongCount(0) {}
};

class ConfigManager
{
public:
    ConfigManager();
    
    // Configuration file management
    bool loadConfig(const QString &configPath = "config.json");
    bool saveConfig(const QString &configPath = "config.json");
    
    // Subject management
    QString getCurrentSubject() const { return m_currentSubject; }
    void setCurrentSubject(const QString &subject) { m_currentSubject = subject; }
    QStringList getAvailableSubjects() const;
    QStringList getSubjects() const { return getAvailableSubjects(); }
    void addSubject(const QString &subject, const QString &path);
    void removeSubject(const QString &subject);
    void refreshSubjectBanks(const QString &subject);
    
    // Question bank management
    QuestionBank getQuestionBank(const QString &subject) const;
    void setQuestionBank(const QString &subject, const QuestionBank &bank);
    bool hasQuestionBank(const QString &subject) const;
    
    // Checkpoint management
    bool hasCheckpoint() const;
    CheckpointData getCheckpoint() const { return m_checkpoint; }
    void setCheckpoint(const CheckpointData &checkpoint) { m_checkpoint = checkpoint; }
    void clearCheckpoint();
    
    // Paths
    QString getSubjectPath(const QString &subject) const;
    QString getWrongAnswersPath() const { return "WA"; }
    QString getWrongAnswersPath(const QString &subject) const;
    
    // Validation
    bool isValidConfig() const;
    QString getLastError() const { return m_lastError; }
    
    // Default configuration
    void createDefaultConfig();
    bool copyFromReference(const QString &referencePath = "Reference/config.json");
    
    // Configuration initialization
    void initializeConfig();
    void createDefaultConfigFile();
    
private:
    QString m_currentSubject;
    QMap<QString, QuestionBank> m_questionBanks;
    QMap<QString, QString> m_subjectPaths; // 存储科目名称到科目路径的映射
    CheckpointData m_checkpoint;
    QString m_lastError;
    
    void parseQuestionBanks(const QJsonObject &json);
    QJsonObject questionBanksToJson() const;
    void parseCheckpoint(const QJsonObject &json);
    QJsonObject checkpointToJson() const;
    QuestionBank mergeQuestionBankInfo(const QuestionBank &configBank, const QuestionBank &scannedBank, const QString &subjectName) const;
};

#endif // CONFIGMANAGER_H
