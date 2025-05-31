#ifndef BANKSCANNER_H
#define BANKSCANNER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include "../models/questionbank.h"

/**
 * @brief 题库扫描工具类
 * 
 * 用于扫描科目目录，加载题库文件，并验证题库文件的有效性
 */
class BankScanner
{
public:
    /**
     * @brief 扫描科目目录，加载题库信息
     * @param dirPath 科目目录路径
     * @param subjectName 科目名称
     * @return 加载的题库对象
     */
    static QuestionBank scanSubjectDirectory(const QString &dirPath, const QString &subjectName);
    
    /**
     * @brief 验证题库文件是否有效
     * @param filePath 题库文件路径
     * @return 是否有效
     */
    static bool validateBankFile(const QString &filePath);
    
    /**
     * @brief 获取题库文件中的题目数量
     * @param filePath 题库文件路径
     * @return 题目数量，如果文件无效则返回0
     */
    static int getBankQuestionCount(const QString &filePath);
    
    /**
     * @brief 获取上次扫描的错误信息
     * @return 错误信息
     */
    static QString getLastError();
    
private:
    static QString s_lastError; // 上次扫描的错误信息
};

#endif // BANKSCANNER_H