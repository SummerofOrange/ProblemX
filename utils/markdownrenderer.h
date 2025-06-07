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
    
    // 自动适配大小功能
    void setAutoResize(bool enabled, int maxHeight = 0);
    
    // 设置样式主题
    void setStyleTheme(const QString &theme = "default");
    
private slots:
    void onLoadFinished(bool success);
    void adjustSizeToContent();
    
private:
    void setupWebEngine();
    void createHtmlTemplate();
    QString convertMarkdownToHtml(const QString &markdown);
    QString protectSpecialContent(const QString &html);
    QString restoreAndProcessProtectedContent(const QString &html);
    QString processCodeBlocks(const QString &html);
    QString processLatexBlocks(const QString &html);
    QString escapeHtmlSpecialChars(const QString &html);
    QString processParagraphs(const QString &html);
    QString processLists(const QString &html);
    QString processTables(const QString &html);
    QString getKatexHtml() const;
    QString getCustomCss() const;
    
    QWebEngineView *m_webView;
    QVBoxLayout *m_layout;
    QString m_htmlTemplate;
    QString m_currentTheme;
    
    // 自动适配相关成员变量
    bool m_autoResize;
    int m_maxAutoHeight;
    int m_parentMaxHeight;
    
    // 用于保护特殊内容的成员变量
    QStringList m_protectedBlocks;
    QStringList m_placeholders;
};

#endif // MARKDOWNRENDERER_H