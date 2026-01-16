#ifndef WRONGANSWERSET_H
#define WRONGANSWERSET_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QMap>
#include "../models/question.h"

/**
 * 错题记录结构
 * 存储单个错题的完整信息
 */
struct WrongAnswerItem {
    QString id;                    // 唯一标识符
    QString subject;               // 科目
    QString questionType;          // 题目类型
    QString questionText;          // 题目描述
    QMap<QString, QString> images;
    QStringList choices;           // 选项列表
    QString correctAnswer;         // 正确答案（单选/判断）
    QStringList correctAnswers;    // 正确答案列表（多选/填空）
    QString userAnswer;            // 用户答案（单选/判断）
    QStringList userAnswers;       // 用户答案列表（多选/填空）
    QDateTime timestamp;           // 添加时间
    int reviewCount;               // 复习次数
    bool isResolved;               // 是否已解决
    QString note;                  // 备注
    
    // 序列化方法
    QJsonObject toJson() const;
    static WrongAnswerItem fromJson(const QJsonObject &json);
    
    // 生成唯一ID
    void generateId();
    
    // 从Question对象创建
    static WrongAnswerItem fromQuestion(const Question &question, const QString &subject, 
                                       const QString &userAnswer = QString(), 
                                       const QList<QString> &userAnswers = QList<QString>());
};

/**
 * 错题集合管理类
 * 负责错题的增删改查和持久化
 */
class WrongAnswerSet : public QObject
{
    Q_OBJECT
    
public:
    explicit WrongAnswerSet(QObject *parent = nullptr);
    ~WrongAnswerSet();
    
    // 文件操作
    bool loadFromFile(const QString &filePath = "");
    bool saveToFile(const QString &filePath = "") const;
    QString getDefaultFilePath() const;
    
    // 错题管理
    void addWrongAnswer(const WrongAnswerItem &item);
    void addWrongAnswers(const QVector<WrongAnswerItem> &items);
    bool removeWrongAnswer(const QString &id);
    bool updateWrongAnswer(const QString &id, const WrongAnswerItem &item);
    WrongAnswerItem getWrongAnswer(const QString &id) const;
    
    // 查询和筛选
    QVector<WrongAnswerItem> getAllWrongAnswers() const;
    QVector<WrongAnswerItem> getWrongAnswersBySubject(const QString &subject) const;
    QVector<WrongAnswerItem> getWrongAnswersByType(const QString &type) const;
    QVector<WrongAnswerItem> getWrongAnswersByDateRange(const QDateTime &from, const QDateTime &to) const;
    QVector<WrongAnswerItem> getUnresolvedWrongAnswers() const;
    QVector<WrongAnswerItem> getResolvedWrongAnswers() const;
    
    // 标记操作
    bool markAsResolved(const QString &id);
    bool markAsUnresolved(const QString &id);
    bool markAsResolved(const QStringList &ids);
    bool markAsUnresolved(const QStringList &ids);
    
    // 统计信息
    int getTotalCount() const;
    int getResolvedCount() const;
    int getUnresolvedCount() const;
    QStringList getAvailableSubjects() const;
    QStringList getAvailableTypes() const;
    
    // 导出功能
    QString exportToMarkdown() const;
    bool exportToMarkdownFile(const QString &filePath) const;
    
    // 清理操作
    void clear();
    bool removeResolvedItems();
    
signals:
    void wrongAnswerAdded(const WrongAnswerItem &item);
    void wrongAnswerRemoved(const QString &id);
    void wrongAnswerUpdated(const QString &id, const WrongAnswerItem &item);
    void dataChanged();
    
private:
    QVector<WrongAnswerItem> m_wrongAnswers;
    QString m_filePath;
    
    // 辅助方法
    QString generateUniqueId() const;
    bool isDuplicate(const WrongAnswerItem &item) const;
    QString formatQuestionForMarkdown(const WrongAnswerItem &item) const;
    QString getQuestionTypeString(const QString &type) const;
};

#endif // WRONGANSWERSET_H
