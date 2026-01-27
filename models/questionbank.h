#ifndef QUESTIONBANK_H
#define QUESTIONBANK_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>
#include "question.h"

struct QuestionBankInfo {
    QString name;
    QString src;
    int size;
    int chosennum;
    bool chosen;
    QuestionType type;
    
    QuestionBankInfo() : size(0), chosennum(0), chosen(false), type(QuestionType::Choice) {}
};

class QuestionBank
{
public:
    QuestionBank();
    QuestionBank(const QString &subject);
    
    // Getters
    QString getSubject() const { return m_subject; }
    QVector<QuestionBankInfo> getChoiceBanks() const { return m_choiceBanks; }
    QVector<QuestionBankInfo> getTrueOrFalseBanks() const { return m_trueOrFalseBanks; }
    QVector<QuestionBankInfo> getFillBlankBanks() const { return m_fillBlankBanks; }
    QVector<QuestionBankInfo> getAllBanks() const;
    
    // Setters
    void setSubject(const QString &subject) { m_subject = subject; }
    void setChoiceBanks(const QVector<QuestionBankInfo> &banks) { m_choiceBanks = banks; }
    void setTrueOrFalseBanks(const QVector<QuestionBankInfo> &banks) { m_trueOrFalseBanks = banks; }
    void setFillBlankBanks(const QVector<QuestionBankInfo> &banks) { m_fillBlankBanks = banks; }
    
    // Bank management
    void addChoiceBank(const QuestionBankInfo &bank);
    void addTrueOrFalseBank(const QuestionBankInfo &bank);
    void addFillBlankBank(const QuestionBankInfo &bank);
    
    void updateBankInfo(QuestionType type, int index, const QuestionBankInfo &bank);
    void setBankChosen(QuestionType type, int index, bool chosen);
    void setBankChosenNum(QuestionType type, int index, int chosenNum);
    
    // Question loading
    QList<Question> loadSelectedQuestions(const QString &subjectPath, bool shuffleQuestions = true) const;
    QList<Question> loadQuestionsFromBank(const QString &subjectPath, const QuestionBankInfo &bank, bool shuffleQuestions = true) const;
    
    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    // Utility
    int getTotalSelectedQuestions() const;
    bool hasSelectedBanks() const;
    
private:
    QString m_subject;
    QVector<QuestionBankInfo> m_choiceBanks;
    QVector<QuestionBankInfo> m_trueOrFalseBanks;
    QVector<QuestionBankInfo> m_fillBlankBanks;
    
    QList<Question> loadQuestionsFromFile(const QString &filePath) const;
    QVector<QuestionBankInfo>& getBanksByType(QuestionType type);
    const QVector<QuestionBankInfo>& getBanksByType(QuestionType type) const;
};

#endif // QUESTIONBANK_H
