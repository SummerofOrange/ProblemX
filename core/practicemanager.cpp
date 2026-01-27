#include "practicemanager.h"
#include "questionmanager.h"
#include "configmanager.h"
#include "../models/questionbank.h"
#include "../utils/jsonutils.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

PracticeManager::PracticeManager(QObject *parent)
    : QObject(parent)
    , m_wrongAnswerSet(nullptr)
{
    // Connect question manager signals
    connect(&m_questionManager, &QuestionManager::questionAnswered,
            this, &PracticeManager::onQuestionAnswered);
    connect(&m_questionManager, &QuestionManager::allQuestionsAnswered,
            this, &PracticeManager::onAllQuestionsAnswered);
    connect(&m_questionManager, &QuestionManager::currentQuestionChanged,
            this, [this](int index) {
                emit questionChanged(index, getTotalQuestions());
            });
}

bool PracticeManager::startNewPractice(const QString &subject, ConfigManager *configManager)
{
    if (!configManager) {
        return false;
    }
    
    QuestionBank bank = configManager->getQuestionBank(subject);
    if (!bank.hasSelectedBanks()) {
        qWarning() << "No question banks selected for subject:" << subject;
        return false;
    }
    
    QString subjectPath = configManager->getSubjectPath(subject);
    m_currentSubjectPath = subjectPath;  // 存储当前科目路径
    if (!m_questionManager.loadQuestions(bank, subjectPath, configManager->isShuffleQuestionsEnabled())) {
        qWarning() << "Failed to load questions for subject:" << subject;
        return false;
    }
    
    // Initialize session
    m_currentSession = PracticeSession();
    m_currentSession.mode = PracticeMode::Normal;
    m_currentSession.subject = subject;
    m_currentSession.startTime = QDateTime::currentDateTime();
    m_currentSession.totalQuestions = m_questionManager.getQuestionCount();
    
    setState(PracticeState::InProgress);
    emit practiceStarted(PracticeMode::Normal);
    
    return true;
}

bool PracticeManager::resumePractice(ConfigManager *configManager)
{
    if (!configManager || !configManager->hasCheckpoint()) {
        return false;
    }
    
    CheckpointData checkpoint = configManager->getCheckpoint();
    
    // 设置当前科目信息
    m_currentSession.subject = configManager->getCurrentSubject();
    
    // 设置当前科目路径
    m_currentSubjectPath = configManager->getSubjectPath(m_currentSession.subject);
    
    // Reconstruct questions from checkpoint
    QList<Question> allQuestions;
    allQuestions.append(checkpoint.trueOrFalseData);
    allQuestions.append(checkpoint.choiceData);
    allQuestions.append(checkpoint.fillBlankData);
    
    if (allQuestions.isEmpty()) {
        return false;
    }
    
    m_questionManager.setQuestions(allQuestions);
    
    // 恢复答题状态数据
    int questionCount = allQuestions.size();
    if (checkpoint.answeredFlags.size() == questionCount) {
        // 先清空错题列表，避免重复添加
        m_questionManager.clearWrongAnswers();
        
        // 恢复每道题的答题状态
        for (int i = 0; i < questionCount; ++i) {
            if (checkpoint.answeredFlags[i]) {
                m_questionManager.setAnswered(i, true);
                
                // 恢复用户答案（不进行答案检查，避免重复添加错题）
                if (!checkpoint.userAnswers[i].isEmpty()) {
                    m_questionManager.setUserAnswerWithoutCheck(i, checkpoint.userAnswers[i]);
                }
                if (!checkpoint.userMultiAnswers[i].isEmpty()) {
                    m_questionManager.setUserAnswersWithoutCheck(i, checkpoint.userMultiAnswers[i]);
                }
                
                // 如果答错了，添加到错题列表
                if (!checkpoint.correctFlags[i]) {
                    m_questionManager.addWrongAnswer(i);
                }
            }
        }
        
        // 恢复统计数据
        m_questionManager.setCorrectCount(checkpoint.correctCount);
        m_questionManager.setWrongCount(checkpoint.wrongCount);
    }
    
    // Initialize session
    m_currentSession = PracticeSession();
    m_currentSession.mode = PracticeMode::Resume;
    m_currentSession.subject = configManager->getCurrentSubject();
    m_currentSession.startTime = QDateTime::currentDateTime();
    m_currentSession.totalQuestions = allQuestions.size();
    
    // Set current position based on checkpoint
    int currentPos = checkpoint.trueOrFalseCheck + checkpoint.choiceCheck + checkpoint.fillBlankCheck;
    if (currentPos < allQuestions.size()) {
        m_questionManager.setCurrentIndex(currentPos);
    }
    
    setState(PracticeState::InProgress);
    emit practiceStarted(PracticeMode::Resume);
    
    return true;
}

bool PracticeManager::startReviewPractice(const QList<Question> &wrongQuestions, const QString &subjectPath)
{
    if (wrongQuestions.isEmpty()) {
        return false;
    }
    
    // 设置科目路径（如果提供的话）
    if (!subjectPath.isEmpty()) {
        m_currentSubjectPath = subjectPath;
    }
    
    m_questionManager.setQuestions(wrongQuestions);
    
    // Initialize session
    m_currentSession = PracticeSession();
    m_currentSession.mode = PracticeMode::Review;
    m_currentSession.subject = "错题复习";
    m_currentSession.startTime = QDateTime::currentDateTime();
    m_currentSession.totalQuestions = wrongQuestions.size();
    
    setState(PracticeState::InProgress);
    emit practiceStarted(PracticeMode::Review);
    
    return true;
}

void PracticeManager::pausePractice()
{
    if (m_currentSession.state == PracticeState::InProgress) {
        m_currentSession.pauseTime = QDateTime::currentDateTime();
        setState(PracticeState::Paused);
        emit practicePaused();
    }
}

void PracticeManager::resumePractice()
{
    if (m_currentSession.state == PracticeState::Paused) {
        if (m_currentSession.pauseTime.isValid()) {
            // 累计暂停时长
            qint64 pausedDuration = m_currentSession.pauseTime.secsTo(QDateTime::currentDateTime());
            m_currentSession.totalPausedTime += pausedDuration;
            m_currentSession.pauseTime = QDateTime(); // 清空暂停时间
        }
        setState(PracticeState::InProgress);
        emit practiceResumed();
    }
}

void PracticeManager::completePractice()
{
    if (m_currentSession.state == PracticeState::InProgress ||
        m_currentSession.state == PracticeState::Paused) {
        
        // 如果当前是暂停状态，先处理暂停时间
        if (m_currentSession.state == PracticeState::Paused && m_currentSession.pauseTime.isValid()) {
            qint64 pausedDuration = m_currentSession.pauseTime.secsTo(QDateTime::currentDateTime());
            m_currentSession.totalPausedTime += pausedDuration;
        }
        
        m_currentSession.endTime = QDateTime::currentDateTime();
        updateSessionStatistics();
        setState(PracticeState::Completed);
        
        // Save wrong answers if any
        QList<Question> wrongQuestions = m_questionManager.getWrongAnswers();
        qDebug() << "[DEBUG] completePractice: Found" << wrongQuestions.size() << "wrong answers";
        
        if (!wrongQuestions.isEmpty()) {
            saveWrongAnswers();
            
            // 询问用户是否要将错题导入错题系统
            qDebug() << "[DEBUG] completePractice: WrongAnswerSet pointer:" << (m_wrongAnswerSet ? "valid" : "null");
            if (m_wrongAnswerSet) {
                qDebug() << "[DEBUG] completePractice: Calling askUserToImportWrongAnswers";
                askUserToImportWrongAnswers();
            } else {
                qDebug() << "[DEBUG] completePractice: WrongAnswerSet is null, cannot import wrong answers";
            }
        } else {
            qDebug() << "[DEBUG] completePractice: No wrong answers to import";
        }
        
        emit practiceCompleted(m_currentSession);
    }
}

void PracticeManager::saveCheckpoint(ConfigManager *configManager)
{
    if (!configManager) {
        return;
    }
    
    // Create checkpoint data
    CheckpointData checkpoint;
    
    // For simplicity, we'll save all questions as choice type
    // In a real implementation, you'd separate by type
    QList<Question> allQuestions = m_questionManager.getAllQuestions();
    int currentIndex = m_questionManager.getCurrentIndex();
    
    checkpoint.choiceData = allQuestions;
    checkpoint.choiceCheck = currentIndex;
    checkpoint.wrongAnswers = m_questionManager.getWrongAnswers();
    
    // 保存答题状态数据
    int questionCount = allQuestions.size();
    checkpoint.answeredFlags.resize(questionCount);
    checkpoint.userAnswers.resize(questionCount);
    checkpoint.userMultiAnswers.resize(questionCount);
    checkpoint.correctFlags.resize(questionCount);
    
    for (int i = 0; i < questionCount; ++i) {
        checkpoint.answeredFlags[i] = m_questionManager.isAnswered(i);
        checkpoint.userAnswers[i] = m_questionManager.getUserAnswer(i);
        checkpoint.userMultiAnswers[i] = m_questionManager.getUserAnswers(i);
        checkpoint.correctFlags[i] = m_questionManager.checkAnswer(i);
    }
    
    checkpoint.correctCount = m_questionManager.getCorrectCount();
    checkpoint.wrongCount = m_questionManager.getWrongCount();
    
    configManager->setCheckpoint(checkpoint);
    configManager->saveConfig();
    
    setState(PracticeState::Saved);
}

int PracticeManager::getTotalQuestions() const
{
    return m_questionManager.getQuestionCount();
}

int PracticeManager::getAnsweredQuestions() const
{
    return m_questionManager.getAnsweredCount();
}

int PracticeManager::getRemainingQuestions() const
{
    return getTotalQuestions() - getAnsweredQuestions();
}

double PracticeManager::getProgress() const
{
    int total = getTotalQuestions();
    if (total == 0) {
        return 0.0;
    }
    return (double)getAnsweredQuestions() / total * 100.0;
}

int PracticeManager::getCorrectAnswers() const
{
    return m_questionManager.getCorrectCount();
}

int PracticeManager::getWrongAnswers() const
{
    return m_questionManager.getWrongCount();
}

double PracticeManager::getAccuracy() const
{
    return m_questionManager.getAccuracy();
}

QList<Question> PracticeManager::getWrongQuestions() const
{
    return m_questionManager.getWrongAnswers();
}

qint64 PracticeManager::getElapsedTime() const
{
    if (m_currentSession.startTime.isValid()) {
        QDateTime endTime;
        qint64 currentPausedTime = 0;
        
        if (m_currentSession.state == PracticeState::Paused && m_currentSession.pauseTime.isValid()) {
            // 如果当前处于暂停状态，计算到暂停时刻的时间
            endTime = m_currentSession.pauseTime;
        } else {
            // 如果已结束或正在进行中，使用结束时间或当前时间
            endTime = m_currentSession.endTime.isValid() ? 
                     m_currentSession.endTime : QDateTime::currentDateTime();
        }
        
        // 总时长减去累计暂停时长
        qint64 totalTime = m_currentSession.startTime.secsTo(endTime);
        return totalTime - m_currentSession.totalPausedTime;
    }
    return 0;
}

QString PracticeManager::getElapsedTimeString() const
{
    qint64 seconds = getElapsedTime();
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
               .arg(hours, 2, 10, QChar('0'))
               .arg(minutes, 2, 10, QChar('0'))
               .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
               .arg(minutes, 2, 10, QChar('0'))
               .arg(secs, 2, 10, QChar('0'));
    }
}

bool PracticeManager::submitAnswer(const QString &answer)
{
    if (!canSubmitAnswer()) {
        qDebug() << "Cannot submit answer: canSubmit=" << canSubmitAnswer();
        return false;
    }
    
    qDebug() << "PracticeManager::submitAnswer called with:" << answer;
    int currentIndex = m_questionManager.getCurrentIndex();
    m_questionManager.setUserAnswer(currentIndex, answer);
    
    return true;
}

bool PracticeManager::submitAnswers(const QStringList &answers)
{
    if (!canSubmitAnswer()) {
        return false;
    }
    
    int currentIndex = m_questionManager.getCurrentIndex();
    m_questionManager.setUserAnswers(currentIndex, answers);
    
    return true;
}

bool PracticeManager::canSubmitAnswer() const
{
    return m_currentSession.state == PracticeState::InProgress &&
           !m_questionManager.isEmpty() &&
           !m_questionManager.isAnswered(m_questionManager.getCurrentIndex());
}

bool PracticeManager::canGoToNext() const
{
    return m_questionManager.hasNext();
}

bool PracticeManager::canGoToPrevious() const
{
    return m_questionManager.hasPrevious();
}

void PracticeManager::goToNext()
{
    if (canGoToNext()) {
        m_questionManager.moveNext();
    }
}

void PracticeManager::goToPrevious()
{
    if (canGoToPrevious()) {
        m_questionManager.movePrevious();
    }
}

void PracticeManager::goToQuestion(int index)
{
    m_questionManager.setCurrentIndex(index);
}

bool PracticeManager::canFinishPractice() const
{
    return m_currentSession.state == PracticeState::InProgress;
}

QVector<int> PracticeManager::getUnansweredQuestions() const
{
    return m_questionManager.getUnansweredQuestions();
}

void PracticeManager::saveWrongAnswers(const QString &filePath)
{
    QList<Question> wrongQuestions = m_questionManager.getWrongAnswers();
    if (wrongQuestions.isEmpty()) {
        return;
    }
    
    QString fileName = filePath;
    if (fileName.isEmpty()) {
        fileName = generateWrongAnswersFileName();
    }
    
    // Create WA directory if it doesn't exist
    QDir waDir("WA");
    if (!waDir.exists()) {
        waDir.mkpath(".");
    }
    
    QJsonObject json;
    QJsonArray dataArray;
    
    for (const Question &question : wrongQuestions) {
        dataArray.append(question.toJson());
    }
    
    json["data"] = dataArray;
    json["subject"] = m_currentSession.subject;
    json["date"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    json["total_questions"] = wrongQuestions.size();
    
    JsonUtils::saveJsonToFile(json, fileName);
}

QList<Question> PracticeManager::loadWrongAnswers(const QString &filePath)
{
    QList<Question> questions;
    
    QJsonObject json = JsonUtils::loadJsonFromFile(filePath);
    if (json.isEmpty()) {
        return questions;
    }
    
    if (json.contains("data") && json["data"].isArray()) {
        QJsonArray dataArray = json["data"].toArray();
        for (const QJsonValue &value : dataArray) {
            if (value.isObject()) {
                Question question(value.toObject());
                questions.append(question);
            }
        }
    }
    
    return questions;
}

QStringList PracticeManager::getAvailableWrongAnswerFiles(const QString &directory)
{
    QStringList files;
    QDir dir(directory);
    
    if (dir.exists()) {
        QStringList filters;
        filters << "*.json";
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
        
        for (const QFileInfo &fileInfo : fileList) {
            files.append(fileInfo.absoluteFilePath());
        }
    }
    
    return files;
}

void PracticeManager::reset()
{
    m_questionManager.reset();
    m_currentSession = PracticeSession();
}

void PracticeManager::onQuestionAnswered(int index, bool correct)
{
    qDebug() << "PracticeManager::onQuestionAnswered called: index=" << index << ", correct=" << correct;
    updateSessionStatistics();
    qDebug() << "Emitting answerSubmitted signal";
    emit answerSubmitted(index, correct);
    emit progressChanged(getProgress());
    emit statisticsUpdated(getCorrectAnswers(), getWrongAnswers(), getAccuracy());
}

void PracticeManager::onAllQuestionsAnswered()
{
    completePractice();
}

void PracticeManager::updateSessionStatistics()
{
    m_currentSession.answeredQuestions = getAnsweredQuestions();
    m_currentSession.correctAnswers = getCorrectAnswers();
    m_currentSession.wrongAnswers = getWrongAnswers();
    m_currentSession.accuracy = getAccuracy();
    m_currentSession.wrongQuestionsList = getWrongQuestions();
}

void PracticeManager::setState(PracticeState state)
{
    m_currentSession.state = state;
}

QString PracticeManager::generateWrongAnswersFileName() const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString subject = m_currentSession.subject;
    subject.replace(" ", "_");
    subject.replace("/", "_");
    
    return QString("WA/%1_%2.json").arg(subject).arg(timestamp);
}

void PracticeManager::setWrongAnswerSet(WrongAnswerSet *wrongAnswerSet)
{
    m_wrongAnswerSet = wrongAnswerSet;
}

bool PracticeManager::askUserToImportWrongAnswers()
{
    QList<Question> wrongQuestions = m_questionManager.getWrongAnswers();
    qDebug() << "[DEBUG] askUserToImportWrongAnswers: Wrong questions count:" << wrongQuestions.size();
    
    if (wrongQuestions.isEmpty()) {
        qDebug() << "[DEBUG] askUserToImportWrongAnswers: No wrong answers, returning false";
        return false;
    }
    
    // 发出信号让UI层处理用户询问
    qDebug() << "[DEBUG] askUserToImportWrongAnswers: Emitting wrongAnswersImportRequested signal with subject:" << m_currentSession.subject;
    emit wrongAnswersImportRequested(wrongQuestions, m_currentSession.subject);
    qDebug() << "[DEBUG] askUserToImportWrongAnswers: Signal emitted successfully";
    
    // 这里返回false，实际的导入操作由UI层调用importWrongAnswersToSet()完成
    return false;
}

int PracticeManager::importWrongAnswersToSet()
{
    qDebug() << "[DEBUG] importWrongAnswersToSet: Starting import process";
    
    if (!m_wrongAnswerSet) {
        qDebug() << "[DEBUG] importWrongAnswersToSet: WrongAnswerSet not set, cannot import wrong answers";
        return -1;
    }
    
    QList<Question> wrongQuestions = m_questionManager.getWrongAnswers();
    QVector<int> wrongIndices = m_questionManager.getWrongAnswerIndices();
    
    qDebug() << "[DEBUG] importWrongAnswersToSet: Wrong questions count:" << wrongQuestions.size();
    qDebug() << "[DEBUG] importWrongAnswersToSet: Wrong indices count:" << wrongIndices.size();
    
    if (wrongQuestions.isEmpty() || wrongQuestions.size() != wrongIndices.size()) {
        qDebug() << "[DEBUG] importWrongAnswersToSet: No wrong answers to import or indices mismatch";
        return 0;
    }
    
    QVector<WrongAnswerItem> wrongItems;
    for (int i = 0; i < wrongQuestions.size(); ++i) {
        const Question &question = wrongQuestions[i];
        int questionIndex = wrongIndices[i];
        
        qDebug() << "[DEBUG] importWrongAnswersToSet: Processing question" << i << "with index" << questionIndex;
        
        // 从QuestionManager获取用户的具体答案
        QString userAnswer;
        QList<QString> userAnswers;
        
        if (question.getType() == QuestionType::FillBlank || question.getType() == QuestionType::MultipleChoice) {
            QStringList multiAnswers = m_questionManager.getUserAnswers(questionIndex);
            qDebug() << "[DEBUG] importWrongAnswersToSet: Multi answers for question" << i << ":" << multiAnswers;
            for (const QString &ans : multiAnswers) {
                userAnswers.append(ans);
            }
        } else {
            userAnswer = m_questionManager.getUserAnswer(questionIndex);
            qDebug() << "[DEBUG] importWrongAnswersToSet: Single answer for question" << i << ":" << userAnswer;
        }
        
        WrongAnswerItem item = WrongAnswerItem::fromQuestion(question, m_currentSession.subject, userAnswer, userAnswers);
        wrongItems.append(item);
        qDebug() << "[DEBUG] importWrongAnswersToSet: Created WrongAnswerItem for question" << i;
    }
    
    qDebug() << "[DEBUG] importWrongAnswersToSet: Adding" << wrongItems.size() << "items to WrongAnswerSet";
    const int beforeCount = m_wrongAnswerSet->getTotalCount();
    m_wrongAnswerSet->addWrongAnswers(wrongItems);
    const int afterCount = m_wrongAnswerSet->getTotalCount();
    const int addedCount = afterCount - beforeCount;
    
    qDebug() << "[DEBUG] importWrongAnswersToSet: Saving to file";
    bool saveResult = m_wrongAnswerSet->saveToFile();
    qDebug() << "[DEBUG] importWrongAnswersToSet: Save result:" << (saveResult ? "success" : "failed");
    
    qDebug() << "[DEBUG] importWrongAnswersToSet: Imported" << wrongItems.size() << "wrong answers to WA_SET.json";
    if (!saveResult) {
        return -1;
    }
    return addedCount;
}

void PracticeManager::savePractice(ConfigManager *configManager)
{
    if (!configManager) {
        return;
    }
    
    // 保存检查点数据
    saveCheckpoint(configManager);
    
    // 更新练习状态为已保存
    setState(PracticeState::Saved);
    
    // 保存配置
    configManager->saveConfig();
}
