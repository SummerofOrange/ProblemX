#ifndef PTAASSISTCONTROLLER_H
#define PTAASSISTCONTROLLER_H

#include <QObject>
#include <QString>

class QWebEngineView;
class QJsonObject;
class Question;

class PtaAssistController : public QObject
{
    Q_OBJECT

public:
    explicit PtaAssistController(QObject *parent = nullptr);

    void setWebView(QWebEngineView *view);
    QWebEngineView *webView() const;

    void parseVisibleQuestions();
    void scrollToQuestionId(const QString &id);
    void fillFromBankQuestion(const QString &ptaId, const Question &bankQuestion);

signals:
    void parsedJsonReady(const QString &jsonText);
    void fillFinished(const QString &ptaId, bool ok, const QString &message);

private:
    QString buildParseScript() const;
    QString buildScrollScript(const QString &id) const;
    QString buildFillScript(const QString &ptaId, const QJsonObject &payload) const;

    QWebEngineView *m_view;
};

#endif // PTAASSISTCONTROLLER_H
