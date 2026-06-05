#ifndef OCSSERVER_H
#define OCSSERVER_H

#include <QObject>
#include <QHostAddress>
#include <QJsonObject>

class ConfigManager;
class QTcpServer;
class QTcpSocket;
class QuestionSearchIndex;

struct OcsServerConfig {
    QString host = "127.0.0.1";
    quint16 port = 27419;
    double threshold = 0.85;
    int topK = 5;
};

class OcsServer : public QObject
{
    Q_OBJECT

public:
    explicit OcsServer(QuestionSearchIndex *searchIndex, QObject *parent = nullptr);
    ~OcsServer();

    bool start(const OcsServerConfig &config, ConfigManager *configManager);
    void stop();
    bool isRunning() const;
    QString lastError() const;
    OcsServerConfig config() const;
    QString baseUrl() const;

signals:
    void logMessage(const QString &message);
    void runningChanged(bool running);
    void unmatchedQuestion(const QString &title, const QStringList &options, const QString &type, double bestScore, double threshold);

private slots:
    void handleNewConnection();

private:
    void handleSocketReadyRead(QTcpSocket *socket);
    void processRequest(QTcpSocket *socket, const QByteArray &rawRequest);
    void writeJsonResponse(QTcpSocket *socket, int statusCode, const QJsonObject &body) const;
    void writeTextResponse(QTcpSocket *socket, int statusCode, const QString &body, const QByteArray &contentType) const;
    QJsonObject buildSearchResponse(const QString &title, const QStringList &options, const QString &type);
    QString extractQueryTitle(const QJsonObject &body) const;
    QStringList extractOptions(const QJsonObject &body) const;
    QString makeAnswerText(const QStringList &answers) const;
    QString makeRequestLogTitle(const QString &title) const;

    QTcpServer *m_server;
    QuestionSearchIndex *m_searchIndex;
    OcsServerConfig m_config;
    QString m_lastError;
};

#endif // OCSSERVER_H
