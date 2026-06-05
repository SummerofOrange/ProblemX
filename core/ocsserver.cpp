#include "ocsserver.h"

#include "configmanager.h"
#include "../models/question.h"
#include "../utils/questionsearchindex.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QRegularExpression>

OcsServer::OcsServer(QuestionSearchIndex *searchIndex, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_searchIndex(searchIndex)
{
    connect(m_server, &QTcpServer::newConnection, this, &OcsServer::handleNewConnection);
}

OcsServer::~OcsServer()
{
    stop();
}

bool OcsServer::start(const OcsServerConfig &config, ConfigManager *configManager)
{
    m_lastError.clear();
    if (!m_searchIndex) {
        m_lastError = "题库索引未初始化";
        return false;
    }

    if (!m_searchIndex->isReady()) {
        if (!configManager || !m_searchIndex->buildFromConfig(configManager)) {
            m_lastError = m_searchIndex->lastError().isEmpty() ? "题库索引加载失败" : m_searchIndex->lastError();
            return false;
        }
    }

    if (m_server->isListening()) {
        stop();
    }

    m_config = config;
    m_config.threshold = qBound(0.0, m_config.threshold, 1.0);
    m_config.topK = qBound(1, m_config.topK, 50);

    const QHostAddress address(m_config.host.isEmpty() ? QStringLiteral("127.0.0.1") : m_config.host);
    if (!m_server->listen(address, m_config.port)) {
        m_lastError = m_server->errorString();
        return false;
    }

    emit logMessage(QString("服务已启动：%1，题库 %2 题").arg(baseUrl()).arg(m_searchIndex->documentCount()));
    emit runningChanged(true);
    return true;
}

void OcsServer::stop()
{
    if (!m_server->isListening()) {
        return;
    }

    m_server->close();
    emit logMessage("服务已停止");
    emit runningChanged(false);
}

bool OcsServer::isRunning() const
{
    return m_server->isListening();
}

QString OcsServer::lastError() const
{
    return m_lastError;
}

OcsServerConfig OcsServer::config() const
{
    return m_config;
}

QString OcsServer::baseUrl() const
{
    return QString("http://%1:%2").arg(m_config.host.isEmpty() ? QStringLiteral("127.0.0.1") : m_config.host).arg(m_config.port);
}

void OcsServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleSocketReadyRead(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void OcsServer::handleSocketReadyRead(QTcpSocket *socket)
{
    QByteArray buffer = socket->property("requestBuffer").toByteArray();
    buffer.append(socket->readAll());
    if (buffer.isEmpty()) {
        return;
    }

    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        socket->setProperty("requestBuffer", buffer);
        return;
    }

    int contentLength = 0;
    const QByteArray header = buffer.left(headerEnd);
    const QList<QByteArray> lines = header.split('\n');
    for (const QByteArray &line : lines) {
        const int colon = line.indexOf(':');
        if (colon <= 0) {
            continue;
        }
        const QByteArray key = line.left(colon).trimmed().toLower();
        if (key == "content-length") {
            contentLength = line.mid(colon + 1).trimmed().toInt();
            break;
        }
    }

    const int totalLength = headerEnd + 4 + contentLength;
    if (buffer.size() < totalLength) {
        socket->setProperty("requestBuffer", buffer);
        return;
    }

    socket->setProperty("requestBuffer", QByteArray());
    processRequest(socket, buffer.left(totalLength));
}

void OcsServer::processRequest(QTcpSocket *socket, const QByteArray &rawRequest)
{
    const int headerEnd = rawRequest.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        writeJsonResponse(socket, 400, QJsonObject{{"code", 400}, {"msg", "Bad Request"}});
        return;
    }

    const QByteArray header = rawRequest.left(headerEnd);
    const QByteArray bodyBytes = rawRequest.mid(headerEnd + 4);
    const QList<QByteArray> lines = header.split('\n');
    if (lines.isEmpty()) {
        writeJsonResponse(socket, 400, QJsonObject{{"code", 400}, {"msg", "Bad Request"}});
        return;
    }

    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        writeJsonResponse(socket, 400, QJsonObject{{"code", 400}, {"msg", "Bad Request"}});
        return;
    }

    const QString method = QString::fromLatin1(requestLine[0]).toUpper();
    const QString target = QString::fromUtf8(requestLine[1]);
    const QUrl url(target);
    const QString path = url.path();

    if (method == "OPTIONS") {
        writeTextResponse(socket, 204, QString(), "text/plain; charset=utf-8");
        return;
    }

    if (method == "GET" && (path == "/" || path == "/health")) {
        QJsonObject body;
        body["code"] = 0;
        body["msg"] = "ok";
        body["running"] = isRunning();
        body["questionCount"] = m_searchIndex ? m_searchIndex->documentCount() : 0;
        body["threshold"] = m_config.threshold;
        body["topK"] = m_config.topK;
        body["time"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        writeJsonResponse(socket, 200, body);
        return;
    }

    if (path != "/api/search") {
        writeJsonResponse(socket, 404, QJsonObject{{"code", 404}, {"msg", "Not Found"}});
        return;
    }

    QJsonObject requestBody;
    if (method == "POST") {
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(bodyBytes, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            writeJsonResponse(socket, 400, QJsonObject{{"code", 400}, {"msg", "请求 JSON 无效"}});
            return;
        }
        requestBody = doc.object();
    } else if (method == "GET") {
        const QUrlQuery query(url);
        requestBody["title"] = query.queryItemValue("title");
        requestBody["type"] = query.queryItemValue("type");
    } else {
        writeJsonResponse(socket, 405, QJsonObject{{"code", 405}, {"msg", "Method Not Allowed"}});
        return;
    }

    const QString title = extractQueryTitle(requestBody);
    const QStringList options = extractOptions(requestBody);
    const QString type = requestBody.value("type").toString();
    const QJsonObject response = buildSearchResponse(title, options, type);
    writeJsonResponse(socket, 200, response);
}

void OcsServer::writeJsonResponse(QTcpSocket *socket, int statusCode, const QJsonObject &body) const
{
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    writeTextResponse(socket, statusCode, QString::fromUtf8(payload), "application/json; charset=utf-8");
}

void OcsServer::writeTextResponse(QTcpSocket *socket, int statusCode, const QString &body, const QByteArray &contentType) const
{
    QByteArray reason = "OK";
    if (statusCode == 204) reason = "No Content";
    else if (statusCode == 400) reason = "Bad Request";
    else if (statusCode == 404) reason = "Not Found";
    else if (statusCode == 405) reason = "Method Not Allowed";

    const QByteArray payload = body.toUtf8();
    QByteArray response;
    response.append("HTTP/1.1 " + QByteArray::number(statusCode) + " " + reason + "\r\n");
    response.append("Content-Type: " + contentType + "\r\n");
    response.append("Content-Length: " + QByteArray::number(payload.size()) + "\r\n");
    response.append("Access-Control-Allow-Origin: *\r\n");
    response.append("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    response.append("Access-Control-Allow-Headers: Content-Type, Authorization\r\n");
    response.append("Connection: close\r\n\r\n");
    response.append(payload);
    socket->write(response);
    socket->disconnectFromHost();
}

QJsonObject OcsServer::buildSearchResponse(const QString &title, const QStringList &options, const QString &type)
{
    QJsonObject response;
    response["code"] = 0;

    if (!m_searchIndex || !m_searchIndex->isReady()) {
        response["code"] = 503;
        response["msg"] = "题库索引未加载";
        response["data"] = QJsonArray();
        return response;
    }

    QString query = title.trimmed();
    if (!options.isEmpty()) {
        query.append("\n");
        query.append(options.join("\n"));
    }

    if (query.trimmed().isEmpty()) {
        response["code"] = 400;
        response["msg"] = "题目为空";
        response["data"] = QJsonArray();
        return response;
    }

    const QVector<SearchHit> hits = m_searchIndex->searchTopK(query, m_config.topK);
    QJsonArray data;
    double bestScore = 0.0;
    for (const SearchHit &hit : hits) {
        if (hit.docIndex < 0 || hit.docIndex >= m_searchIndex->documentCount()) {
            continue;
        }
        if (hit.score > bestScore) {
            bestScore = hit.score;
        }
        if (hit.score < m_config.threshold) {
            continue;
        }

        const Question &question = m_searchIndex->documentQuestion(hit.docIndex);
        const QuestionSourceInfo &source = m_searchIndex->documentSource(hit.docIndex);
        const QString answerText = makeAnswerText(question.getAnswers());
        if (answerText.trimmed().isEmpty()) {
            continue;
        }

        QJsonArray rawAnswers;
        for (const QString &answer : question.getAnswers()) {
            rawAnswers.append(answer);
        }

        QJsonObject item;
        item["question"] = question.getQuestion();
        item["answer"] = answerText;
        item["score"] = hit.score;
        item["type"] = Question::typeToString(question.getType());
        item["source"] = source.bankSrc;
        item["subject"] = source.subject;
        item["bank"] = source.bankName;
        item["answers"] = rawAnswers;
        if (!type.trimmed().isEmpty()) {
            item["requestType"] = type.trimmed();
        }
        data.append(item);
    }

    response["data"] = data;
    response["count"] = data.size();
    response["bestScore"] = bestScore;
    response["threshold"] = m_config.threshold;
    response["msg"] = data.isEmpty() ? "未匹配到达到阈值的题目" : "ok";

    if (data.isEmpty() && !title.trimmed().isEmpty()) {
        emit unmatchedQuestion(title.trimmed(), options, type.trimmed(), bestScore, m_config.threshold);
    }

    emit logMessage(QString("检索：%1 -> %2 条结果，best=%3")
                    .arg(makeRequestLogTitle(title))
                    .arg(data.size())
                    .arg(QString::number(bestScore, 'f', 3)));
    return response;
}

QString OcsServer::extractQueryTitle(const QJsonObject &body) const
{
    const QStringList keys = {"title", "question", "q", "keyword", "query"};
    for (const QString &key : keys) {
        const QString value = body.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

QStringList OcsServer::extractOptions(const QJsonObject &body) const
{
    QStringList options;
    const QJsonValue value = body.value("options");
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array) {
            if (item.isString()) {
                const QString text = item.toString().trimmed();
                if (!text.isEmpty()) {
                    options.append(text);
                }
            } else if (item.isObject()) {
                const QJsonObject object = item.toObject();
                QString label = object.value("label").toString().trimmed();
                if (label.isEmpty()) {
                    label = object.value("tag").toString().trimmed();
                }
                QString text = object.value("text").toString().trimmed();
                if (text.isEmpty()) {
                    text = object.value("content").toString().trimmed();
                }
                if (!label.isEmpty() && !text.isEmpty()) {
                    options.append(label + "." + text);
                } else if (!text.isEmpty()) {
                    options.append(text);
                }
            }
        }
    }

    return options;
}

QString OcsServer::makeAnswerText(const QStringList &answers) const
{
    if (answers.isEmpty()) {
        return QString();
    }

    QStringList cleaned;
    for (const QString &answer : answers) {
        const QString text = answer.trimmed();
        if (!text.isEmpty()) {
            cleaned.append(text);
        }
    }
    return cleaned.join("#");
}

QString OcsServer::makeRequestLogTitle(const QString &title) const
{
    QString compact = title;
    compact.replace(QRegularExpression("\\s+"), " ");
    compact = compact.trimmed();
    if (compact.size() > 36) {
        compact = compact.left(36) + "...";
    }
    return compact;
}
