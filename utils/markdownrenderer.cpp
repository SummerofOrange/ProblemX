#include "markdownrenderer.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QRegularExpression>
#include <QDebug>

MarkdownRenderer::MarkdownRenderer(QWidget *parent)
    : QWidget(parent)
    , m_webView(nullptr)
    , m_layout(nullptr)
    , m_currentTheme("default")
{
    setupWebEngine();
    createHtmlTemplate();
}

MarkdownRenderer::~MarkdownRenderer()
{
}

void MarkdownRenderer::setupWebEngine()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    m_webView = new QWebEngineView(this);
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    
    // 配置WebEngine设置
    QWebEngineSettings *settings = m_webView->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    
    m_layout->addWidget(m_webView);
    
    connect(m_webView, &QWebEngineView::loadFinished,
            this, &MarkdownRenderer::onLoadFinished);
}

void MarkdownRenderer::createHtmlTemplate()
{
    m_htmlTemplate = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Question Renderer</title>
    
    <!-- KaTeX CSS -->
    <link rel="stylesheet" href="https://cdn.bootcdn.net/ajax/libs/KaTeX/0.16.8/katex.min.css">
    
    <!-- Custom CSS -->
    <style>
        %CUSTOM_CSS%
    </style>
</head>
<body>
    <div id="content">
        %CONTENT%
    </div>
    
    <!-- KaTeX JS -->
    <script src="https://cdn.bootcdn.net/ajax/libs/KaTeX/0.16.8/katex.min.js"></script>
    <script src="https://cdn.bootcdn.net/ajax/libs/KaTeX/0.16.8/contrib/auto-render.min.js"></script>
    
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            // 确保KaTeX和auto-render已加载
            if (typeof renderMathInElement !== 'undefined') {
                var options = {};
                options.delimiters = [];
                options.delimiters.push({left: '$$', right: '$$', display: true});
                options.delimiters.push({left: '$', right: '$', display: false});
                options.delimiters.push({left: '\\[', right: '\\]', display: true});
                options.delimiters.push({left: '\\(', right: '\\)', display: false});
                options.throwOnError = false;
                renderMathInElement(document.body, options);
            } else {
                console.error('renderMathInElement is not defined. KaTeX auto-render may not be loaded.');
            }
        });
    </script>
</body>
</html>
    )";
}

QString MarkdownRenderer::convertMarkdownToHtml(const QString &markdown)
{
    QString html = markdown;
    
    // 基本Markdown转换
    // 标题
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");
    
    // 粗体和斜体
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");
    
    // 代码
    html.replace(QRegularExpression("`(.+?)`"), "<code>\\1</code>");
    
    // 换行
    html.replace("\n", "<br>");
    
    return html;
}

QString MarkdownRenderer::getCustomCss() const
{
    return R"(
        body {
            font-family: 'Microsoft YaHei', 'SimHei', Arial, sans-serif;
            font-size: 14px;
            line-height: 1.6;
            color: #2c3e50;
            margin: 0;
            padding: 15px;
            background-color: white;
        }
        
        h1, h2, h3 {
            color: #2c3e50;
            margin-top: 0;
            margin-bottom: 10px;
        }
        
        h1 { font-size: 18px; }
        h2 { font-size: 16px; }
        h3 { font-size: 15px; }
        
        p {
            margin: 8px 0;
        }
        
        code {
            background-color: #f8f9fa;
            padding: 2px 4px;
            border-radius: 3px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 13px;
        }
        
        strong {
            font-weight: bold;
            color: #2c3e50;
        }
        
        em {
            font-style: italic;
            color: #6c757d;
        }
        
        .katex {
            font-size: 1.1em;
        }
        
        .katex-display {
            margin: 10px 0;
        }
    )";
}

void MarkdownRenderer::setContent(const QString &markdownText)
{
    QString htmlContent = convertMarkdownToHtml(markdownText);
    QString finalHtml = m_htmlTemplate;
    finalHtml.replace("%CONTENT%", htmlContent);
    finalHtml.replace("%CUSTOM_CSS%", getCustomCss());
    
    m_webView->setHtml(finalHtml);
}

void MarkdownRenderer::setMinimumHeight(int height)
{
    m_webView->setMinimumHeight(height);
}

void MarkdownRenderer::setMaximumHeight(int height)
{
    m_webView->setMaximumHeight(height);
}

void MarkdownRenderer::setStyleTheme(const QString &theme)
{
    m_currentTheme = theme;
    // 可以根据主题调整CSS样式
}

void MarkdownRenderer::onLoadFinished(bool success)
{
    if (!success) {
        qDebug() << "Failed to load content in MarkdownRenderer";
    }
}