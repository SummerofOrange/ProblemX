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
                // 移除 \( 和 \) 分隔符，避免普通括号被误渲染为数学公式
                // options.delimiters.push({left: '\(', right: '\)', display: false});
                options.throwOnError = false;
                // 设置trust为false，确保<和>被视为文本而不是HTML标签
                options.trust = false;
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
    
    // 使用统一的保护机制处理所有需要避免HTML转义的内容
    html = protectSpecialContent(html);
    
    // HTML特殊字符转义（在保护特殊内容之后进行）
    html = escapeHtmlSpecialChars(html);
    
    // 恢复并处理被保护的内容
    html = restoreAndProcessProtectedContent(html);
    
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
    
    // 先处理双美元符号的LaTeX公式
    QRegularExpression latexBlockRegex("\\$\\$([^$]*?)\\$\\$", QRegularExpression::DotMatchesEverythingOption);
    QList<QRegularExpressionMatch> blockMatches;
    QRegularExpressionMatchIterator blockIterator = latexBlockRegex.globalMatch(result);
    while (blockIterator.hasNext()) {
        blockMatches.append(blockIterator.next());
    }
    
    // 从后往前替换，避免位置偏移问题
    for (int i = blockMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = blockMatches[i];
        QString latexContent = match.captured(1).trimmed();
        QString latexBlock = QString("$$%1$$").arg(latexContent);
        result.replace(match.capturedStart(), match.capturedLength(), latexBlock);
    }
    
    // 再处理单美元符号的LaTeX公式
    QRegularExpression singleLatexRegex("\\$([^$\\n]*?)\\$");
    QList<QRegularExpressionMatch> singleMatches;
    QRegularExpressionMatchIterator singleIterator = singleLatexRegex.globalMatch(result);
    while (singleIterator.hasNext()) {
        singleMatches.append(singleIterator.next());
    }
    
    // 从后往前替换，避免位置偏移问题
    for (int i = singleMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = singleMatches[i];
        QString latexContent = match.captured(1);
        QString latexBlock = QString("$%1$").arg(latexContent);
        result.replace(match.capturedStart(), match.capturedLength(), latexBlock);
    }
    
    return result;
}

QString MarkdownRenderer::escapeHtmlSpecialChars(const QString &html)
{
    QString result = html;
    
    // 转义HTML特殊字符
    result.replace("&", "&amp;");  // 必须首先处理&，避免重复转义
    result.replace("<", "&lt;");
    result.replace(">", "&gt;");
    result.replace('"', "&quot;");
    
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
    QStack<int> olStartStack;  // 存储有序列表的起始编号
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        // 检查是否是列表项
        QRegularExpression ulRegex("^[*+-]\\s+(.+)$");
        QRegularExpression olRegex("^(\\d+)\\.\\s+(.+)$");
        QRegularExpressionMatch ulMatch = ulRegex.match(trimmedLine);
        QRegularExpressionMatch olMatch = olRegex.match(trimmedLine);
        
        if (ulMatch.hasMatch() || olMatch.hasMatch()) {
            // 计算当前行的缩进级别
            int currentIndent = line.length() - line.trimmed().length();
            QString listType = ulMatch.hasMatch() ? "ul" : "ol";
            QString content = ulMatch.hasMatch() ? ulMatch.captured(1) : olMatch.captured(2);
            int olNumber = olMatch.hasMatch() ? olMatch.captured(1).toInt() : 1;
            
            // 处理嵌套逻辑
            while (!indentStack.isEmpty() && currentIndent <= indentStack.top()) {
                QString closingTag = "</" + listStack.pop() + ">";
                processedLines.append(closingTag);
                indentStack.pop();
                if (!olStartStack.isEmpty()) {
                    olStartStack.pop();
                }
            }
            
            // 如果是新的嵌套级别或不同类型的列表
            if (indentStack.isEmpty() || currentIndent > indentStack.top() || 
                (!listStack.isEmpty() && listStack.top() != listType)) {
                if (!indentStack.isEmpty() && listStack.top() != listType) {
                    // 关闭当前列表，开始新类型的列表
                    QString closingTag = "</" + listStack.pop() + ">";
                    processedLines.append(closingTag);
                    indentStack.pop();
                    if (!olStartStack.isEmpty()) {
                        olStartStack.pop();
                    }
                }
                
                QString openingTag;
                if (listType == "ol") {
                    // 对于有序列表，添加start属性
                    openingTag = QString("<ol start=\"%1\">").arg(olNumber);
                    olStartStack.push(olNumber);
                } else {
                    openingTag = "<ul>";
                }
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
                if (!olStartStack.isEmpty()) {
                    olStartStack.pop();
                }
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
        if (!olStartStack.isEmpty()) {
            olStartStack.pop();
        }
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
             margin: 10px 0;
             padding-left: 30px;
             list-style-type: decimal;
         }
         
         ol ol {
             list-style-type: lower-alpha;
         }
         
         ol ol ol {
             list-style-type: lower-roman;
         }
         
         ol > li {
             display: list-item;
             margin-bottom: 0.5em;
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

QString MarkdownRenderer::protectSpecialContent(const QString &html)
{
    QString result = html;
    
    // 存储需要保护的内容
    QStringList protectedBlocks;
    QStringList placeholders;
    
    // 保护代码块 - 使用原始markdown语法
    QRegularExpression codeBlockRegex("```([^`]*?)```", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator codeIterator = codeBlockRegex.globalMatch(result);
    QList<QRegularExpressionMatch> codeMatches;
    while (codeIterator.hasNext()) {
        codeMatches.append(codeIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = codeMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = codeMatches[i];
        QString placeholder = QString("__PROTECTED_CODE_%1__").arg(protectedBlocks.size());
        protectedBlocks.append(match.captured(0));
        placeholders.append(placeholder);
        result.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }
    
    // 保护行内代码
    QRegularExpression inlineCodeRegex("`([^`]+)`");
    QRegularExpressionMatchIterator inlineCodeIterator = inlineCodeRegex.globalMatch(result);
    QList<QRegularExpressionMatch> inlineCodeMatches;
    while (inlineCodeIterator.hasNext()) {
        inlineCodeMatches.append(inlineCodeIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = inlineCodeMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = inlineCodeMatches[i];
        QString placeholder = QString("__PROTECTED_INLINE_CODE_%1__").arg(protectedBlocks.size());
        protectedBlocks.append(match.captured(0));
        placeholders.append(placeholder);
        result.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }
    
    // 保护双美元符号LaTeX公式
    QRegularExpression latexBlockRegex("\\$\\$([^$]*?)\\$\\$", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator latexBlockIterator = latexBlockRegex.globalMatch(result);
    QList<QRegularExpressionMatch> latexBlockMatches;
    while (latexBlockIterator.hasNext()) {
        latexBlockMatches.append(latexBlockIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = latexBlockMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = latexBlockMatches[i];
        // 获取原始内容并进行特殊处理
        QString latexContent = match.captured(0);
        // 使用HTML实体编码替换<和>，确保它们在LaTeX处理时被正确解释
        latexContent.replace("<", "&lt;");
        latexContent.replace(">", "&gt;");
        QString placeholder = QString("__PROTECTED_LATEX_BLOCK_%1__").arg(protectedBlocks.size());
        protectedBlocks.append(latexContent);
        placeholders.append(placeholder);
        result.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }
    
    // 保护单美元符号LaTeX公式
    QRegularExpression latexInlineRegex("\\$([^$]+?)\\$");
    QRegularExpressionMatchIterator latexInlineIterator = latexInlineRegex.globalMatch(result);
    QList<QRegularExpressionMatch> latexInlineMatches;
    while (latexInlineIterator.hasNext()) {
        latexInlineMatches.append(latexInlineIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = latexInlineMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = latexInlineMatches[i];
        // 获取原始内容并进行特殊处理
        QString latexContent = match.captured(0);
        // 使用HTML实体编码替换<和>，确保它们在LaTeX处理时被正确解释
        latexContent.replace("<", "&lt;");
        latexContent.replace(">", "&gt;");
        QString placeholder = QString("__PROTECTED_LATEX_INLINE_%1__").arg(protectedBlocks.size());
        protectedBlocks.append(latexContent);
        placeholders.append(placeholder);
        result.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }
    
    // 保护引用块
    QRegularExpression quoteRegex("^> (.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator quoteIterator = quoteRegex.globalMatch(result);
    QList<QRegularExpressionMatch> quoteMatches;
    while (quoteIterator.hasNext()) {
        quoteMatches.append(quoteIterator.next());
    }
    // 从后往前替换，避免位置偏移
    for (int i = quoteMatches.size() - 1; i >= 0; --i) {
        QRegularExpressionMatch match = quoteMatches[i];
        QString placeholder = QString("__PROTECTED_QUOTE_%1__").arg(protectedBlocks.size());
        protectedBlocks.append(match.captured(0));
        placeholders.append(placeholder);
        result.replace(match.capturedStart(), match.capturedLength(), placeholder);
    }
    
    // 将保护信息存储到成员变量中，供后续恢复使用
    m_protectedBlocks = protectedBlocks;
    m_placeholders = placeholders;
    
    return result;
}

QString MarkdownRenderer::restoreAndProcessProtectedContent(const QString &html)
{
    QString result = html;
    
    // 恢复并处理被保护的内容
    for (int i = 0; i < m_protectedBlocks.size(); ++i) {
        QString content = m_protectedBlocks[i];
        QString placeholder = m_placeholders[i];
        
        if (placeholder.startsWith("__PROTECTED_CODE_")) {
            // 处理代码块
            content = processCodeBlocks(content);
        } else if (placeholder.startsWith("__PROTECTED_INLINE_CODE_")) {
            // 处理行内代码
            QRegularExpression inlineCodeRegex("`([^`]+)`");
            content.replace(inlineCodeRegex, "<code>\\1</code>");
        } else if (placeholder.startsWith("__PROTECTED_LATEX_BLOCK_")) {
            // 处理LaTeX块公式 - 直接使用原始内容，不需要额外处理
        } else if (placeholder.startsWith("__PROTECTED_LATEX_INLINE_")) {
            // 处理LaTeX行内公式 - 直接使用原始内容，不需要额外处理
        } else if (placeholder.startsWith("__PROTECTED_QUOTE_")) {
            // 处理引用块
            QRegularExpression quoteRegex("^> (.+)$", QRegularExpression::MultilineOption);
            content.replace(quoteRegex, "<blockquote>\\1</blockquote>");
        }
        
        result.replace(placeholder, content);
    }
    
    // 清理保护信息
    m_protectedBlocks.clear();
    m_placeholders.clear();
    
    return result;
}
