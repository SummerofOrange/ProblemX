#ifndef QUESTIONSEARCHINDEX_H
#define QUESTIONSEARCHINDEX_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVector>

#include "../models/question.h"

class ConfigManager;

struct QuestionSourceInfo {
    QString subject;
    QString bankName;
    QString bankSrc;
};

struct SearchHit {
    int docIndex = -1;
    double score = 0.0;
};

class QuestionSearchIndex : public QObject
{
    Q_OBJECT

public:
    explicit QuestionSearchIndex(QObject *parent = nullptr);

    void clear();
    bool isReady() const;
    int documentCount() const;

    bool buildFromConfig(ConfigManager *configManager);
    QString lastError() const;

    QVector<SearchHit> searchTopK(const QString &queryText, int topK, int candidateLimit = 2000) const;

    const Question &documentQuestion(int docIndex) const;
    const QuestionSourceInfo &documentSource(int docIndex) const;

private:
    struct Doc {
        Question question;
        QuestionSourceInfo source;
        QString normalized;
        int gramCount = 0;
    };

    void addDocument(const Question &question, const QuestionSourceInfo &source);
    void rebuildInvertedIndex();

    QVector<Doc> m_docs;
    QHash<QString, QVector<int>> m_inverted;
    QString m_lastError;
    bool m_ready = false;
};

#endif // QUESTIONSEARCHINDEX_H
