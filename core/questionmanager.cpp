#include "questionmanager.h"
#include <QDebug>
#include <QDir>

QuestionManager::QuestionManager(QObject *parent)
    : QObject(parent)
    , m_currentIndex(0)
    , m_correctCount(0)
    , m_wrongCount(0)
{
}

bool QuestionManager::loadQuestions(const QuestionBank &bank, const QString &subjectPath, bool shuffleQuestions)
{
    m_questions = bank.loadSelectedQuestions(subjectPath, shuffleQuestions);
    initializeAnswerTracking();
    
    if (!m_questions.isEmpty()) {
        setCurrentIndex(0);
        return true;
    }
    
    return false;
}

bool QuestionManager::loadQuestionsFromFiles(const QStringList &filePaths)
{
    m_questions.clear();
    
    for (const QString &filePath : filePaths) {
        // This would need to be implemented based on the specific file format
        // For now, we'll leave it as a placeholder
        qDebug() << "Loading questions from:" << filePath;
    }
    
    initializeAnswerTracking();
    return !m_questions.isEmpty();
}

void QuestionManager::setQuestions(const QList<Question> &questions)
{
    m_questions = questions;
    initializeAnswerTracking();
    
    if (!m_questions.isEmpty()) {
        setCurrentIndex(0);
    }
}

Question QuestionManager::getQuestion(int index) const
{
    if (isValidIndex(index)) {
        return m_questions[index];
    }
    return Question();
}

void QuestionManager::setCurrentIndex(int index)
{
    if (isValidIndex(index) && index != m_currentIndex) {
        m_currentIndex = index;
        emit currentQuestionChanged(index);
    }
}

Question QuestionManager::getCurrentQuestion() const
{
    return getQuestion(m_currentIndex);
}

bool QuestionManager::hasNext() const
{
    return m_currentIndex < m_questions.size() - 1;
}

bool QuestionManager::hasPrevious() const
{
    return m_currentIndex > 0;
}

void QuestionManager::moveNext()
{
    if (hasNext()) {
        setCurrentIndex(m_currentIndex + 1);
    }
}

void QuestionManager::movePrevious()
{
    if (hasPrevious()) {
        setCurrentIndex(m_currentIndex - 1);
    }
}

void QuestionManager::moveToFirst()
{
    if (!m_questions.isEmpty()) {
        setCurrentIndex(0);
    }
}

void QuestionManager::moveToLast()
{
    if (!m_questions.isEmpty()) {
        setCurrentIndex(m_questions.size() - 1);
    }
}

bool QuestionManager::isAnswered(int index) const
{
    if (isValidIndex(index)) {
        return m_answeredFlags[index];
    }
    return false;
}

void QuestionManager::setAnswered(int index, bool answered)
{
    if (isValidIndex(index)) {
        m_answeredFlags[index] = answered;
    }
}

QVector<int> QuestionManager::getUnansweredQuestions() const
{
    QVector<int> unanswered;
    for (int i = 0; i < m_questions.size(); ++i) {
        if (!m_answeredFlags[i]) {
            unanswered.append(i + 1); // 1-based indexing for display
        }
    }
    return unanswered;
}

QVector<int> QuestionManager::getAnsweredQuestions() const
{
    QVector<int> answered;
    for (int i = 0; i < m_questions.size(); ++i) {
        if (m_answeredFlags[i]) {
            answered.append(i + 1); // 1-based indexing for display
        }
    }
    return answered;
}

int QuestionManager::getAnsweredCount() const
{
    int count = 0;
    for (bool answered : m_answeredFlags) {
        if (answered) count++;
    }
    return count;
}

int QuestionManager::getUnansweredCount() const
{
    return m_questions.size() - getAnsweredCount();
}

void QuestionManager::setUserAnswer(int index, const QString &answer)
{
    qDebug() << "QuestionManager::setUserAnswer called: index=" << index << ", answer=" << answer;
    
    if (isValidIndex(index)) {
        m_userAnswers[index] = answer;
        setAnswered(index, true);
        
        // Check if answer is correct
        bool correct = m_questions[index].checkAnswer(answer);
        qDebug() << "Answer check result: correct=" << correct;
        
        if (correct) {
            m_correctCount++;
        } else {
            m_wrongCount++;
            addWrongAnswer(index);
        }
        
        qDebug() << "Emitting questionAnswered signal from QuestionManager";
        emit questionAnswered(index, correct);
        
        // Check if all questions are answered
        if (getUnansweredCount() == 0) {
            emit allQuestionsAnswered();
        }
    } else {
        qDebug() << "Invalid index in setUserAnswers:" << index;
    }
}

void QuestionManager::setUserAnswerWithoutCheck(int index, const QString &answer)
{
    if (isValidIndex(index)) {
        m_userAnswers[index] = answer;
        setAnswered(index, true);
        // 不进行答案检查，不更新统计数据，不添加错题
    }
}

void QuestionManager::setUserAnswersWithoutCheck(int index, const QStringList &answers)
{
    if (isValidIndex(index)) {
        m_userMultiAnswers[index] = answers;
        setAnswered(index, true);
        // 不进行答案检查，不更新统计数据，不添加错题
    }
}

void QuestionManager::setUserAnswers(int index, const QStringList &answers)
{
    if (isValidIndex(index)) {
        m_userMultiAnswers[index] = answers;
        setAnswered(index, true);
        
        // Check if answers are correct
        bool correct = m_questions[index].checkAnswers(answers);
        if (correct) {
            m_correctCount++;
        } else {
            m_wrongCount++;
            addWrongAnswer(index);
        }
        
        emit questionAnswered(index, correct);
        
        // Check if all questions are answered
        if (getUnansweredCount() == 0) {
            emit allQuestionsAnswered();
        }
    }
}

QString QuestionManager::getUserAnswer(int index) const
{
    if (isValidIndex(index)) {
        return m_userAnswers[index];
    }
    return QString();
}

QStringList QuestionManager::getUserAnswers(int index) const
{
    if (isValidIndex(index)) {
        return m_userMultiAnswers[index];
    }
    return QStringList();
}

QStringList QuestionManager::getUserMultiAnswers(int index) const
{
    if (isValidIndex(index)) {
        return m_userMultiAnswers[index];
    }
    return QStringList();
}

bool QuestionManager::checkAnswer(int index) const
{
    if (!isValidIndex(index) || !isAnswered(index)) {
        return false;
    }
    
    const Question &question = m_questions[index];
    
    if (question.getType() == QuestionType::FillBlank) {
        return question.checkAnswers(m_userMultiAnswers[index]);
    } else {
        return question.checkAnswer(m_userAnswers[index]);
    }
}

bool QuestionManager::checkCurrentAnswer() const
{
    return checkAnswer(m_currentIndex);
}

void QuestionManager::addWrongAnswer(int index)
{
    if (isValidIndex(index)) {
        m_wrongAnswers.append(m_questions[index]);
        m_wrongAnswerIndices.append(index);
    }
}

double QuestionManager::getAccuracy() const
{
    int total = m_correctCount + m_wrongCount;
    if (total == 0) {
        return 0.0;
    }
    return (double)m_correctCount / total * 100.0;
}

void QuestionManager::reset()
{
    m_questions.clear();
    m_answeredFlags.clear();
    m_userAnswers.clear();
    m_userMultiAnswers.clear();
    m_wrongAnswers.clear();
    m_wrongAnswerIndices.clear();
    m_currentIndex = 0;
    m_correctCount = 0;
    m_wrongCount = 0;
}

void QuestionManager::resetAnswers()
{
    m_answeredFlags.fill(false);
    m_userAnswers.fill(QString());
    m_userMultiAnswers.fill(QStringList());
    m_wrongAnswers.clear();
    m_wrongAnswerIndices.clear();  // 修复：同时清空错题索引列表
    m_correctCount = 0;
    m_wrongCount = 0;
}

QString QuestionManager::getQuestionTypeString(int index) const
{
    if (!isValidIndex(index)) {
        return QString();
    }
    
    switch (m_questions[index].getType()) {
    case QuestionType::Choice:
        return "选择题";
    case QuestionType::TrueOrFalse:
        return "判断题";
    case QuestionType::FillBlank:
        return "填空题";
    case QuestionType::MultipleChoice:
        return "多选题";
    }
    
    return "未知题型";
}

void QuestionManager::initializeAnswerTracking()
{
    int size = m_questions.size();
    m_answeredFlags.resize(size);
    m_answeredFlags.fill(false);
    m_userAnswers.resize(size);
    m_userAnswers.fill(QString());
    m_userMultiAnswers.resize(size);
    m_userMultiAnswers.fill(QStringList());
    m_wrongAnswers.clear();
    m_wrongAnswerIndices.clear();  // 修复：同时清空错题索引列表
    m_currentIndex = 0;
    m_correctCount = 0;
    m_wrongCount = 0;
}

bool QuestionManager::isValidIndex(int index) const
{
    return index >= 0 && index < m_questions.size();
}
