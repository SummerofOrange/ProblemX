#include "markdownrenderer.h"
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QRegularExpression>
#include <QDebug>
#include <QCoreApplication>
#include <QUrl>
#include <QStack>

MarkdownRenderer::MarkdownRenderer(QWidget *parent)
    : QWidget(parent)
    , m_webView(nullptr)
    , m_layout(nullptr)
    , m_currentTheme("default")
    , m_autoResize(false)
    , m_maxAutoHeight(0)
    , m_parentMaxHeight(0)
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
    <link rel="stylesheet" href="katex/katex.css">
    
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
    <script src="katex/katex.min.js"></script>
    <script defer src="katex/contrib/auto-render.min.js"></script>
    
    <script>
        document.addEventListener('DOMContentLoaded', function() {
            if (typeof renderMathInElement !== 'undefined') {
                var options = {};
                options.delimiters = [];
                options.delimiters.push({left: '$$', right: '$$', display: true});
                options.delimiters.push({left: '$', right: '$', display: false});
                options.delimiters.push({left: '\[', right: '\]', display: true});
                options.delimiters.push({left: '\(', right: '\)', display: false});
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
    
    // 直接接处理代码不使用占位
    html = processCodeBlocks(html);
    
    // 直接处理LaTeX公式，不使用占位符
    
    // 直接处理LaTeX公式，不使用占位符
    html = processLatexBlocks(html);
    
    // 处理多行空格：将多个连续空行合并为一个
    html.replace(QRegularExpression("\n\s*\n\s*\n+"), "\n\n");
    // 处理行内多个空格：将多个连续空格合并为一个
    html.replace(QRegularExpression(" {2,}"), " ");
    
    // 标题 (支持1-6级标题)
    html.replace(QRegularExpression("^###### (.+)$", QRegularExpression::MultilineOption), "<h6>\\1</h6>");
    html.replace(QRegularExpression("^##### (.+)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    html.replace(QRegularExpression("^#### (.+)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>");
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");
    
    // 粗斜体 (必须在粗体和斜体之前处理)
    html.replace(QRegularExpression("\\*\\*\\*(.+?)\\*\\*\\*"), "<strong><em>\\1</em></strong>");
    // html.replace(QRegularExpression("___(.+?)___"), "<strong><em>\\1</em></strong>");
    
    // 粗体
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<strong>\\1</strong>");
    // html.replace(QRegularExpression("__(.+?)__"), "<strong>\\1</strong>");
    
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<em>\\1</em>");
    // html.replace(QRegularExpression("_(.+?)_"), "<em>\\1</em>");
    
    // 删除线
    html.replace(QRegularExpression("~~(.+?)~~"), "<del>\\1</del>");
    
    // 行内代码
    html.replace(QRegularExpression("`(.+?)`"), "<code>\\1</code>");
    
    // 链接 [text](url)
    html.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^\\)]+)\\)"), "<a href=\"\\2\">\\1</a>");
    
    // 图片 ![alt](src)
    html.replace(QRegularExpression("!\\[([^\\]]*)\\]\\(([^\\)]+)\\)"), "<img src=\"\\2\" alt=\"\\1\" />");
    
    // 引用块
    QRegularExpression quoteRegex("^> (.+)$", QRegularExpression::MultilineOption);
    html.replace(quoteRegex, "<blockquote>\\1</blockquote>");
    
    // 水平分割线
    html.replace(QRegularExpression("^---+$", QRegularExpression::MultilineOption), "<hr>");
    html.replace(QRegularExpression("^\\*\\*\\*+$", QRegularExpression::MultilineOption), "<hr>");
    
    // 处理列表
    html = processLists(html);
    
    // 处理表格
    html = processTables(html);
    
    // 段落处理
    html = processParagraphs(html);
    
    return html;
}

QString MarkdownRenderer::processCodeBlocks(const QString &html)
{
    QString result = html;
    QRegularExpression codeBlockRegex("```([^`]*?)```", QRegularExpression::DotMatchesEverythingOption);
    
    // 从后往前替换，避免位置偏移问题
    QList<QRegularExpressionMatch> matches;
    QRegularExpressionMatchIterator iterator = codeBlockRegex.globalMatch(result);
    while (iterator.hasNext()) {
        matches.append(iterator.next());
    }
    
    for (int i = matches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = matches[i];
        QString codeContent = match.captured(1).trimmed();
        
        // 检查是否有语言标识
        QStringList lines = codeContent.split('\n');
        QString language = "";
        QString code = codeContent;
        
        if (!lines.isEmpty()) {
            QString firstLine = lines.first().trimmed();
            // 如果第一行看起来像语言标识符
            if (!firstLine.isEmpty() && !firstLine.contains(' ') && 
                firstLine.length() < 20 && firstLine.length() > 0) {
                language = firstLine;
                lines.removeFirst();
                code = lines.join('\n');
            }
        }
        
        QString escapedCode = code.toHtmlEscaped();
        QString codeBlock;
        if (!language.isEmpty()) {
            codeBlock = QString("<pre><code class=\"language-%1\">%2</code></pre>")
                       .arg(language, escapedCode);
        } else {
            codeBlock = QString("<pre><code>%1</code></pre>").arg(escapedCode);
        }
        
        result.replace(match.capturedStart(), match.capturedLength(), codeBlock);
    }
    
    return result;
}

QString MarkdownRenderer::processLatexBlocks(const QString &html)
{
    QString result = html;
    QRegularExpression latexBlockRegex("\\$\\$([^$]*?)\\$\\$", QRegularExpression::DotMatchesEverythingOption);
    
    // 从后往前替换，避免位置偏移问题
    QList<QRegularExpressionMatch> matches;
    QRegularExpressionMatchIterator iterator = latexBlockRegex.globalMatch(result);
    while (iterator.hasNext()) {
        matches.append(iterator.next());
    }
    
    for (int i = matches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = matches[i];
        QString latexContent = match.captured(1).trimmed();
        QString latexBlock = QString("$$%1$$").arg(latexContent);
        
        result.replace(match.capturedStart(), match.capturedLength(), latexBlock);
    }
    
    return result;
}

QString MarkdownRenderer::processParagraphs(const QString &html)
{
    QStringList lines = html.split('\n');
    QStringList processedLines;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        
        // 跳过空行
        if (line.isEmpty()) {
            processedLines.append("");
            continue;
        }
        
        // 检查是否是已经处理过的HTML标签
        if (line.startsWith("<h") || 
            line.startsWith("<ul") || 
            line.startsWith("<ol") || 
            line.startsWith("<li") || 
            line.startsWith("<blockquote") || 
            line.startsWith("<pre") || 
            line.startsWith("<table") || 
            line.startsWith("<tr") || 
            line.startsWith("<th") || 
            line.startsWith("<td") || 
            line.startsWith("<hr") ||
            line.startsWith("</")) {
            processedLines.append(line);
            continue;
        }
        
        // 检查是否是段落的开始
        bool isParagraphStart = true;
        if (i > 0) {
            QString prevLine = lines[i-1].trimmed();
            if (!prevLine.isEmpty() && 
                !prevLine.startsWith("<h") && 
                !prevLine.startsWith("</h") &&
                !prevLine.startsWith("<ul") && 
                !prevLine.startsWith("</ul") &&
                !prevLine.startsWith("<ol") && 
                !prevLine.startsWith("</ol") &&
                !prevLine.startsWith("<blockquote") && 
                !prevLine.startsWith("</blockquote") &&
                !prevLine.startsWith("<pre") && 
                !prevLine.startsWith("</pre") &&
                !prevLine.startsWith("<table") && 
                !prevLine.startsWith("</table") &&
                !prevLine.startsWith("<hr")) {
                isParagraphStart = false;
            }
        }
        
        if (isParagraphStart) {
            processedLines.append("<p>" + line);
        } else {
            processedLines.append(line);
        }
        
        // 检查是否是段落的结束
        bool isParagraphEnd = true;
        if (i < lines.size() - 1) {
            QString nextLine = lines[i+1].trimmed();
            if (!nextLine.isEmpty() && 
                !nextLine.startsWith("<h") && 
                !nextLine.startsWith("<ul") && 
                !nextLine.startsWith("<ol") && 
                !nextLine.startsWith("<blockquote") && 
                !nextLine.startsWith("<pre") && 
                !nextLine.startsWith("<table") && 
                !nextLine.startsWith("<hr")) {
                isParagraphEnd = false;
            }
        }
        
        if (isParagraphEnd && isParagraphStart) {
            processedLines[processedLines.size()-1] += "</p>";
        }
    }
    
    return processedLines.join('\n');
}

QString MarkdownRenderer::processLists(const QString &html)
{
    QString result = html;
    
    // 处理列表（支持嵌套）
    QStringList lines = result.split('\n');
    QStringList processedLines;
    QStack<QString> listStack; // 存储当前嵌套的列表类型
    QStack<int> indentStack;   // 存储当前嵌套的缩进级别
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        // 检查是否是列表项
        QRegularExpression ulRegex("^[*+-]\\s+(.+)$");
        QRegularExpression olRegex("^\\d+\\.\\s+(.+)$");
        QRegularExpressionMatch ulMatch = ulRegex.match(trimmedLine);
        QRegularExpressionMatch olMatch = olRegex.match(trimmedLine);
        
        if (ulMatch.hasMatch() || olMatch.hasMatch()) {
            // 计算当前行的缩进级别
            int currentIndent = line.length() - line.trimmed().length();
            QString listType = ulMatch.hasMatch() ? "ul" : "ol";
            QString content = ulMatch.hasMatch() ? ulMatch.captured(1) : olMatch.captured(1);
            
            // 处理嵌套逻辑
            while (!indentStack.isEmpty() && currentIndent <= indentStack.top()) {
                QString closingTag = "</" + listStack.pop() + ">";
                processedLines.append(closingTag);
                indentStack.pop();
            }
            
            // 如果是新的嵌套级别或不同类型的列表
            if (indentStack.isEmpty() || currentIndent > indentStack.top() || 
                (!listStack.isEmpty() && listStack.top() != listType)) {
                if (!indentStack.isEmpty() && listStack.top() != listType) {
                    // 关闭当前列表，开始新类型的列表
                    QString closingTag = "</" + listStack.pop() + ">";
                    processedLines.append(closingTag);
                    indentStack.pop();
                }
                
                QString openingTag = "<" + listType + ">";
                processedLines.append(openingTag);
                listStack.push(listType);
                indentStack.push(currentIndent);
            }
            
            processedLines.append("<li>" + content + "</li>");
        } else {
            // 非列表行，关闭所有打开的列表
            while (!listStack.isEmpty()) {
                QString closingTag = "</" + listStack.pop() + ">";
                processedLines.append(closingTag);
                indentStack.pop();
            }
            
            if (!trimmedLine.isEmpty()) {
                processedLines.append(line);
            }
        }
    }
    
    // 关闭剩余的列表
    while (!listStack.isEmpty()) {
        QString closingTag = "</" + listStack.pop() + ">";
        processedLines.append(closingTag);
        indentStack.pop();
    }
    
    return processedLines.join('\n');
}

QString MarkdownRenderer::processTables(const QString &html)
{
    QString result = html;
    QRegularExpression tableRegex("(^\\|.+\\|$\n)+", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator tableIterator = tableRegex.globalMatch(result);
    QStringList tableMatches;
    
    while (tableIterator.hasNext()) {
        QRegularExpressionMatch match = tableIterator.next();
        QString matchedText = match.captured(0);
        QString placeholder = QString("__TABLE_%1__").arg(tableMatches.size());
        tableMatches.append(matchedText);
        result.replace(matchedText, placeholder);
    }
    
    for (int i = 0; i < tableMatches.size(); ++i) {
        QString tableText = tableMatches[i];
        QStringList rows = tableText.split('\n', Qt::SkipEmptyParts);
        
        if (rows.size() < 2) {
            // 不是有效的表格，至少需要标题行和分隔行
            result.replace(QString("__TABLE_%1__").arg(i), tableText);
            continue;
        }
        
        // 检查是否有分隔行（第二行）
        QRegularExpression separatorRegex("^\\|\\s*(:?-+:?\\s*\\|)+\\s*$");
        if (!separatorRegex.match(rows[1]).hasMatch()) {
            // 不是有效的表格分隔行
            result.replace(QString("__TABLE_%1__").arg(i), tableText);
            continue;
        }
        
        QString htmlTable = "<table>\n";
        
        // 处理表头
        htmlTable += "<thead>\n<tr>\n";
        QStringList headerCells = rows[0].split('|');
        // 移除首尾空单元格（如果存在）
        if (!headerCells.isEmpty() && headerCells.first().trimmed().isEmpty()) {
            headerCells.removeFirst();
        }
        if (!headerCells.isEmpty() && headerCells.last().trimmed().isEmpty()) {
            headerCells.removeLast();
        }
        
        for (const QString &cell : headerCells) {
            htmlTable += "<th>" + cell.trimmed() + "</th>\n";
        }
        
        htmlTable += "</tr>\n</thead>\n";
        
        // 处理表格内容
        if (rows.size() > 2) {
            htmlTable += "<tbody>\n";
            
            for (int j = 2; j < rows.size(); ++j) {
                htmlTable += "<tr>\n";
                QStringList cells = rows[j].split('|');
                
                // 移除首尾空单元格（如果存在）
                if (!cells.isEmpty() && cells.first().trimmed().isEmpty()) {
                    cells.removeFirst();
                }
                if (!cells.isEmpty() && cells.last().trimmed().isEmpty()) {
                    cells.removeLast();
                }
                
                for (const QString &cell : cells) {
                    htmlTable += "<td>" + cell.trimmed() + "</td>\n";
                }
                
                htmlTable += "</tr>\n";
            }
            
            htmlTable += "</tbody>\n";
        }
        
        htmlTable += "</table>";
        result.replace(QString("__TABLE_%1__").arg(i), htmlTable);
    }
    
    return result;
}

QString MarkdownRenderer::getCustomCss() const
{
    return R"(
        body {
            font-family: 'Microsoft YaHei', Arial, sans-serif;
            font-size: 14px;
            line-height: 1.6;
            margin: 10px;
        }
        
        h1, h2, h3, h4, h5, h6 {
            color: #333;
            margin-top: 12px;
            margin-bottom: 8px;
        }
        
        h1 { font-size: 22px; border-bottom: 2px solid #eee; padding-bottom: 6px; }
        h2 { font-size: 18px; border-bottom: 1px solid #eee; padding-bottom: 5px; }
        h3 { font-size: 16px; }
        h4 { font-size: 15px; }
        h5 { font-size: 14px; }
        h6 { font-size: 12px; }
        
        p {
            margin: 8px 0;
        }
        
        strong {
            font-weight: bold;
            color: #2c3e50;
        }
        
        em {
            font-style: italic;
            color: #7f8c8d;
        }
        
        del {
            text-decoration: line-through;
            color: #95a5a6;
        }
        
        code {
            background-color: #f8f9fa;
            padding: 2px 4px;
            border-radius: 3px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 13px;
            color: #e74c3c;
        }
        
        pre {
            background-color: #f8f9fa;
            padding: 15px;
            border-radius: 5px;
            overflow-x: auto;
            border-left: 4px solid #3498db;
            margin: 10px 0;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        
        pre code {
            background-color: transparent;
            padding: 0;
            color: #2c3e50;
            font-size: 13px;
            white-space: pre-wrap;
        }
        
        blockquote {
            border-left: 4px solid #bdc3c7;
            margin: 15px 0;
            padding: 10px 20px;
            background-color: #ecf0f1;
            font-style: italic;
        }
        
        ul, ol {
             margin: 8px 0;
             padding-left: 25px;
             list-style-position: outside;
         }
         
         li {
             margin: 2px 0;
             line-height: 1.4;
             padding-left: 0;
         }
         
         ul ul, ol ol, ul ol, ol ul {
             margin: 2px 0;
             padding-left: 20px;
         }
         
         ul {
             list-style-type: disc;
         }
         
         ul ul {
             list-style-type: circle;
         }
         
         ul ul ul {
             list-style-type: square;
         }
         
         ol {
             list-style-type: decimal;
             counter-reset: item;
         }
         
         ol ol {
             list-style-type: lower-alpha;
         }
         
         ol ol ol {
             list-style-type: lower-roman;
         }
         
         ol > li {
             display: list-item;
             list-style-type: decimal;
         }
        
        a {
            color: #3498db;
            text-decoration: none;
        }
        
        a:hover {
            text-decoration: underline;
        }
        
        img {
            max-width: 100%;
            height: auto;
            border-radius: 5px;
            margin: 10px 0;
        }
        
        hr {
            border: none;
            height: 2px;
            background-color: #bdc3c7;
            margin: 20px 0;
        }
        
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 15px 0;
        }
        
        th, td {
            border: 1px solid #bdc3c7;
            padding: 8px 12px;
            text-align: left;
        }
        
        th {
            background-color: #ecf0f1;
            font-weight: bold;
        }
        
        tr:nth-child(even) {
            background-color: #f8f9fa;
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
    
    // 获取应用程序的路径作为baseUrl，以便正确加载相对路径的资源
    QString appDirPath = QCoreApplication::applicationDirPath();
    QUrl baseUrl = QUrl::fromLocalFile(appDirPath + "/resources/");
    m_webView->setHtml(finalHtml, baseUrl);
}

void MarkdownRenderer::setMinimumHeight(int height)
{
    m_webView->setMinimumHeight(height);
}

void MarkdownRenderer::setMaximumHeight(int height)
{
    m_webView->setMaximumHeight(height);
}

void MarkdownRenderer::setAutoResize(bool enabled, int maxHeight)
{
    m_autoResize = enabled;
    m_maxAutoHeight = maxHeight;
    
    if (enabled) {
        // 获取父窗口的最大高度作为参考
        if (parentWidget()) {
            m_parentMaxHeight = parentWidget()->height() * 0.8; // 不超过父窗口的80%
        }
        
        // 如果指定了最大高度，使用较小的那个
        if (maxHeight > 0) {
            m_maxAutoHeight = (m_parentMaxHeight > 0) ? 
                qMin(maxHeight, m_parentMaxHeight) : maxHeight;
        } else {
            m_maxAutoHeight = m_parentMaxHeight;
        }
        
        // 连接页面加载完成信号到调整大小槽
        connect(m_webView, &QWebEngineView::loadFinished,
                this, &MarkdownRenderer::adjustSizeToContent, Qt::UniqueConnection);
    } else {
        // 断开连接
        disconnect(m_webView, &QWebEngineView::loadFinished,
                   this, &MarkdownRenderer::adjustSizeToContent);
    }
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

void MarkdownRenderer::adjustSizeToContent()
{
    if (!m_autoResize) {
        return;
    }
    
    // 使用JavaScript获取内容的实际高度
    m_webView->page()->runJavaScript(
        "document.body.scrollHeight;",
        [this](const QVariant &result) {
            bool ok;
            int contentHeight = result.toInt(&ok);
            if (ok && contentHeight > 0) {
                // 添加一些边距
                int targetHeight = contentHeight + 20;
                
                // 确保不超过最大高度限制
                if (m_maxAutoHeight > 0) {
                    targetHeight = qMin(targetHeight, m_maxAutoHeight);
                }
                
                // 设置最小高度为40像素
                targetHeight = qMax(targetHeight, 40);
                
                // 应用新的高度
                m_webView->setFixedHeight(targetHeight);
                
                qDebug() << "Auto-resized MarkdownRenderer to height:" << targetHeight;
            }
        }
    );
}
