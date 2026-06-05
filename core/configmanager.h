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

    bool isShuffleQuestionsEnabled() const { return m_shuffleQuestionsEnabled; }
    void setShuffleQuestionsEnabled(bool enabled) { m_shuffleQuestionsEnabled = enabled; }

    int getAssistantSearchTopK() const { return m_assistantSearchTopK; }
    void setAssistantSearchTopK(int k) { m_assistantSearchTopK = qBound(1, k, 50); }

    double getAssistantAutoThreshold() const { return m_assistantAutoThreshold; }
    void setAssistantAutoThreshold(double v) { m_assistantAutoThreshold = qBound(0.0, v, 1.0); }

    QString getOcsServiceHost() const { return m_ocsServiceHost; }
    void setOcsServiceHost(const QString &host) { m_ocsServiceHost = host.trimmed().isEmpty() ? "127.0.0.1" : host.trimmed(); }

    int getOcsServicePort() const { return m_ocsServicePort; }
    void setOcsServicePort(int port) { m_ocsServicePort = qBound(1, port, 65535); }

    double getOcsServiceThreshold() const { return m_ocsServiceThreshold; }
    void setOcsServiceThreshold(double threshold) { m_ocsServiceThreshold = qBound(0.0, threshold, 1.0); }

    int getOcsServiceTopK() const { return m_ocsServiceTopK; }
    void setOcsServiceTopK(int topK) { m_ocsServiceTopK = qBound(1, topK, 50); }
    
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
    bool m_shuffleQuestionsEnabled = true;

    int m_assistantSearchTopK = 5;
    double m_assistantAutoThreshold = 0.85;
    QString m_ocsServiceHost = "127.0.0.1";
    int m_ocsServicePort = 27419;
    double m_ocsServiceThreshold = 0.85;
    int m_ocsServiceTopK = 5;
    
    void parseQuestionBanks(const QJsonObject &json);
    QJsonObject questionBanksToJson() const;
    void parseCheckpoint(const QJsonObject &json);
    QJsonObject checkpointToJson() const;
    QuestionBank mergeQuestionBankInfo(const QuestionBank &configBank, const QuestionBank &scannedBank, const QString &subjectName) const;
};

#endif // CONFIGMANAGER_H
