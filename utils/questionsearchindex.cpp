#include "questionsearchindex.h"

#include "../core/configmanager.h"
#include "../models/questionbank.h"
#include "textnormalize.h"

#include <QCryptographicHash>
#include <QSet>
#include <algorithm>

QuestionSearchIndex::QuestionSearchIndex(QObject *parent)
    : QObject(parent)
{
}

void QuestionSearchIndex::clear()
{
    m_docs.clear();
    m_inverted.clear();
    m_lastError.clear();
    m_ready = false;
}

bool QuestionSearchIndex::isReady() const
{
    return m_ready;
}

int QuestionSearchIndex::documentCount() const
{
    return m_docs.size();
}

QString QuestionSearchIndex::lastError() const
{
    return m_lastError;
}

const Question &QuestionSearchIndex::documentQuestion(int docIndex) const
{
    return m_docs[docIndex].question;
}

const QuestionSourceInfo &QuestionSearchIndex::documentSource(int docIndex) const
{
    return m_docs[docIndex].source;
}

static QString buildDocText(const Question &q)
{
    QString text = q.getQuestion();
    const QStringList choices = q.getChoices();
    if (!choices.isEmpty()) {
        text.append("\n");
        text.append(choices.join("\n"));
    }
    return text;
}

static QString fingerprintForQuestion(const Question &q)
{
    const QString normalized = TextNormalize::normalizeForSearch(buildDocText(q));
    const QString key = QString::number(static_cast<int>(q.getType())) + ":" + normalized;
    const QByteArray bytes = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString::fromLatin1(bytes);
}

void QuestionSearchIndex::addDocument(const Question &question, const QuestionSourceInfo &source)
{
    Doc d;
    d.question = question;
    d.source = source;
    d.normalized = TextNormalize::normalizeForSearch(buildDocText(question));
    m_docs.append(d);
}

void QuestionSearchIndex::rebuildInvertedIndex()
{
    m_inverted.clear();

    for (int docId = 0; docId < m_docs.size(); ++docId) {
        const QStringList grams = TextNormalize::makeSearchGrams(m_docs[docId].normalized);
        QSet<QString> uniq(grams.begin(), grams.end());
        m_docs[docId].gramCount = uniq.size();

        for (auto it = uniq.begin(); it != uniq.end(); ++it) {
            m_inverted[*it].append(docId);
        }
    }
}

bool QuestionSearchIndex::buildFromConfig(ConfigManager *configManager)
{
    clear();
    if (!configManager) {
        m_lastError = "ConfigManager 为空";
        return false;
    }

    const QStringList subjects = configManager->getAvailableSubjects();
    if (subjects.isEmpty()) {
        m_lastError = "未配置任何科目";
        return false;
    }

    QHash<QString, int> seenFingerprintToDoc;

    for (const QString &subject : subjects) {
        if (!configManager->hasQuestionBank(subject)) {
            continue;
        }

        const QString subjectPath = configManager->getSubjectPath(subject);
        const QuestionBank bank = configManager->getQuestionBank(subject);
        const QVector<QuestionBankInfo> banks = bank.getAllBanks();

        for (const QuestionBankInfo &bankInfo : banks) {
            const QList<Question> questions = bank.loadAllQuestionsFromBank(subjectPath, bankInfo);
            if (questions.isEmpty()) {
                continue;
            }

            for (const Question &q : questions) {
                const QString fp = fingerprintForQuestion(q);
                if (seenFingerprintToDoc.contains(fp)) {
                    continue;
                }

                QuestionSourceInfo src;
                src.subject = subject;
                src.bankName = bankInfo.name;
                src.bankSrc = bankInfo.src;
                addDocument(q, src);
                seenFingerprintToDoc.insert(fp, m_docs.size() - 1);
            }
        }
    }

    if (m_docs.isEmpty()) {
        m_lastError = "未加载到任何题目";
        return false;
    }

    rebuildInvertedIndex();
    m_ready = true;
    return true;
}

QVector<SearchHit> QuestionSearchIndex::searchTopK(const QString &queryText, int topK, int candidateLimit) const
{
    QVector<SearchHit> hits;
    if (!m_ready || topK <= 0) {
        return hits;
    }

    const QString normalizedQuery = TextNormalize::normalizeForSearch(queryText);
    if (normalizedQuery.isEmpty()) {
        return hits;
    }

    const QStringList grams = TextNormalize::makeSearchGrams(normalizedQuery);
    QSet<QString> uniq(grams.begin(), grams.end());
    if (uniq.isEmpty()) {
        return hits;
    }

    const int qGramCount = uniq.size();
    QHash<int, int> docHits;
    docHits.reserve(candidateLimit * 2);

    for (auto it = uniq.begin(); it != uniq.end(); ++it) {
        const auto found = m_inverted.constFind(*it);
        if (found == m_inverted.constEnd()) {
            continue;
        }
        const QVector<int> &docIds = found.value();
        for (int docId : docIds) {
            docHits[docId] = docHits.value(docId) + 1;
        }
    }

    if (docHits.isEmpty()) {
        return hits;
    }

    QVector<QPair<int, int>> scored;
    scored.reserve(docHits.size());
    for (auto it = docHits.constBegin(); it != docHits.constEnd(); ++it) {
        scored.append(qMakePair(it.key(), it.value()));
    }

    std::sort(scored.begin(), scored.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    const int take = qMin(candidateLimit, scored.size());
    hits.reserve(qMin(topK, take));

    QVector<SearchHit> candidates;
    candidates.reserve(take);
    for (int i = 0; i < take; ++i) {
        const int docId = scored[i].first;
        const int intersection = scored[i].second;
        const int dGramCount = m_docs[docId].gramCount;
        if (dGramCount <= 0) {
            continue;
        }
        const double score = (2.0 * static_cast<double>(intersection)) / (static_cast<double>(qGramCount + dGramCount));
        SearchHit h;
        h.docIndex = docId;
        h.score = score;
        candidates.append(h);
    }

    std::sort(candidates.begin(), candidates.end(), [](const SearchHit &a, const SearchHit &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.docIndex < b.docIndex;
    });

    const int out = qMin(topK, candidates.size());
    for (int i = 0; i < out; ++i) {
        hits.append(candidates[i]);
    }
    return hits;
}
