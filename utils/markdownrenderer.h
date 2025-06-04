#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QWidget>
#include <QWebEngineView>
#include <QVBoxLayout>
#include <QString>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>

class MarkdownRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(QWidget *parent = nullptr);
    ~MarkdownRenderer();
    
    void setContent(const QString &markdownText);
    void setMinimumHeight(int height);
    void setMaximumHeight(int height);
    
    // 设置样式主题
    void setStyleTheme(const QString &theme = "default");
    
private slots:
    void onLoadFinished(bool success);
    
private:
    void setupWebEngine();
    void createHtmlTemplate();
    QString convertMarkdownToHtml(const QString &markdown);
    QString getKatexHtml() const;
    QString getCustomCss() const;
    
    QWebEngineView *m_webView;
    QVBoxLayout *m_layout;
    QString m_htmlTemplate;
    QString m_currentTheme;
};

#endif // MARKDOWNRENDERER_H