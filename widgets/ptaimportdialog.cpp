#include "ptaimportdialog.h"
#include "bankeditorwidget.h"
#include "../core/configmanager.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QUrl>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

static QString buildExtractScript(const QString &typeStr)
{
    if (typeStr == "Choice") {
        return R"JS(
(() => {
  function extractLabel(container) {
    const btn = container.querySelector('button .pc-text-raw.text-xs.ellipsis') || container.querySelector('button .pc-text-raw');
    return btn ? (btn.innerText || '').trim() : '';
  }

  function extractEval(container) {
    const nodes = Array.from(container.querySelectorAll('div'));
    const hit = nodes.find(n => (n.textContent || '').includes('评测结果') && (n.textContent || '').includes('得分'));
    if (!hit) return { state: 3, text: '' };
    const t = (hit.textContent || '').trim();
    if (t.includes('答案正确')) return { state: 1, text: t };
    return { state: 2, text: t };
  }

  function extractQuestionAndImages(questionElem) {
    const clone = questionElem.cloneNode(true);
    const images = {};
    let idx = 1;
    for (const img of Array.from(clone.querySelectorAll('img'))) {
      const src = (img.getAttribute('src') || img.getAttribute('data-src') || '').trim();
      if (!src) continue;
      const key = `img${idx++}`;
      images[key] = src;
      img.replaceWith(document.createTextNode(`![](${key})`));
    }
    const text = (clone.innerText || '').trim();
    return { text, images };
  }

  const result = [];
  const states = [];
  const labels = [];
  const wrongLabels = [];
  const unjudgedLabels = [];

  const containers = document.querySelectorAll('div.pc-x[id]');
  for (const container of containers) {
    const questionElem = container.querySelector('div.markdownBlock_tErSz > div.rendered-markdown') || container.querySelector('div.rendered-markdown');
    if (!questionElem) continue;
    const extracted = extractQuestionAndImages(questionElem);
    const questionText = extracted.text;
    if (!questionText) continue;

    const choices = [];
    let correct = '';
    const optionLabels = container.querySelectorAll('label.w-full');
    for (const label of optionLabels) {
      const input = label.querySelector('input[type=radio]');
      const optionTagElem = label.querySelector('span');
      const optionTextBlock = label.querySelector('div.rendered-markdown');
      if (!input || !optionTagElem || !optionTextBlock) continue;
      const optionTag = (optionTagElem.innerText || '').trim().replace('.', '');
      const optionText = (optionTextBlock.innerText || '').trim();
      if (!optionTag || !optionText) continue;
      choices.push(`${optionTag}.${optionText}`);
      if (input.checked) correct = optionTag;
    }
    if (!choices.length) continue;

    const labelText = extractLabel(container);
    const evalInfo = extractEval(container);
    states.push(evalInfo.state);
    labels.push(labelText);
    if (evalInfo.state === 2) wrongLabels.push(labelText || String(result.length + 1));
    if (evalInfo.state === 3) unjudgedLabels.push(labelText || String(result.length + 1));

    const evalStr = (evalInfo.state === 1 ? 'Correct' : (evalInfo.state === 2 ? 'Wrong' : 'Unjudged'));
    result.push({ type: 'Choice', question: questionText, choices, answer: correct, image: extracted.images, _pta_eval: evalStr, _pta_label: labelText });
  }

  return JSON.stringify({ data: result, meta: { states, labels, wrongLabels, unjudgedLabels } });
})()
)JS";
    }

    if (typeStr == "TrueorFalse") {
        return R"JS(
(() => {
  function extractLabel(container) {
    const btn = container.querySelector('button .pc-text-raw.text-xs.ellipsis') || container.querySelector('button .pc-text-raw');
    return btn ? (btn.innerText || '').trim() : '';
  }

  function extractEval(container) {
    const nodes = Array.from(container.querySelectorAll('div'));
    const hit = nodes.find(n => (n.textContent || '').includes('评测结果') && (n.textContent || '').includes('得分'));
    if (!hit) return { state: 3, text: '' };
    const t = (hit.textContent || '').trim();
    if (t.includes('答案正确')) return { state: 1, text: t };
    return { state: 2, text: t };
  }

  function extractQuestionAndImages(questionElem) {
    const clone = questionElem.cloneNode(true);
    const images = {};
    let idx = 1;
    for (const img of Array.from(clone.querySelectorAll('img'))) {
      const src = (img.getAttribute('src') || img.getAttribute('data-src') || '').trim();
      if (!src) continue;
      const key = `img${idx++}`;
      images[key] = src;
      img.replaceWith(document.createTextNode(`![](${key})`));
    }
    const text = (clone.innerText || '').trim();
    return { text, images };
  }

  const result = [];
  const states = [];
  const labels = [];
  const wrongLabels = [];
  const unjudgedLabels = [];

  const containers = document.querySelectorAll('div.pc-x[id]');
  for (const container of containers) {
    const questionElem = container.querySelector('div.rendered-markdown');
    if (!questionElem) continue;
    const extracted = extractQuestionAndImages(questionElem);
    const questionText = extracted.text;
    if (!questionText) continue;

    let correct = '';
    const found = [];
    const radios = container.querySelectorAll('input[type=radio]');
    for (const radio of radios) {
      const label = radio.closest('label');
      if (!label) continue;
      const txt = (label.innerText || '').trim();
      if (!txt) continue;
      found.push(txt);
      if (radio.checked) correct = txt;
    }
    if (!found.includes('T') || !found.includes('F')) continue;

    const labelText = extractLabel(container);
    const evalInfo = extractEval(container);
    states.push(evalInfo.state);
    labels.push(labelText);
    if (evalInfo.state === 2) wrongLabels.push(labelText || String(result.length + 1));
    if (evalInfo.state === 3) unjudgedLabels.push(labelText || String(result.length + 1));

    const evalStr = (evalInfo.state === 1 ? 'Correct' : (evalInfo.state === 2 ? 'Wrong' : 'Unjudged'));
    result.push({ type: 'TrueorFalse', question: questionText, answer: correct, image: extracted.images, _pta_eval: evalStr, _pta_label: labelText });
  }

  return JSON.stringify({ data: result, meta: { states, labels, wrongLabels, unjudgedLabels } });
})()
)JS";
    }

    if (typeStr == "FillBlank") {
        return R"JS(
(() => {
  function extractLabel(container) {
    const btn = container.querySelector('button .pc-text-raw.text-xs.ellipsis') || container.querySelector('button .pc-text-raw');
    return btn ? (btn.innerText || '').trim() : '';
  }

  function extractEval(container) {
    const nodes = Array.from(container.querySelectorAll('div'));
    const hit = nodes.find(n => (n.textContent || '').includes('评测结果') && (n.textContent || '').includes('得分'));
    if (!hit) return { state: 3, text: '' };
    const t = (hit.textContent || '').trim();
    if (t.includes('答案正确')) return { state: 1, text: t };
    return { state: 2, text: t };
  }

  function extractImagesInPlace(root) {
    const images = {};
    let idx = 1;
    for (const img of Array.from(root.querySelectorAll('img'))) {
      const src = (img.getAttribute('src') || img.getAttribute('data-src') || '').trim();
      if (!src) continue;
      const key = `img${idx++}`;
      images[key] = src;
      img.replaceWith(document.createTextNode(`![](${key})`));
    }
    return images;
  }

  const result = [];
  const states = [];
  const labels = [];
  const wrongLabels = [];
  const unjudgedLabels = [];

  const containers = document.querySelectorAll('div.pc-x[id]');
  for (const container of containers) {
    const originElem = container.querySelector('div.rendered-markdown');
    if (!originElem) continue;
    const clone = originElem.cloneNode(true);
    const images = extractImagesInPlace(clone);

    const blanks = Array.from(clone.querySelectorAll('span[data-blank="true"]'));
    if (!blanks.length) continue;

    const answers = [];
    blanks.forEach((span, idx) => {
      const i = idx + 1;
      const input = span.querySelector('input[data-blank="true"]');
      answers.push(input ? (input.value || '') : '');
      const scoreElem = span.querySelector('.pc-text-raw');
      let score = 'x';
      if (scoreElem) {
        const s = (scoreElem.innerText || '').trim();
        const digits = (s.match(/\d+/g) || []).join('');
        if (digits) score = digits;
      }
      const placeholder = document.createTextNode(`【第${i}空(${score}分)】`);
      span.replaceWith(placeholder);
    });

    const questionText = (clone.innerText || '').trim();
    if (!questionText) continue;
    if (!answers.length) continue;

    const labelText = extractLabel(container);
    const evalInfo = extractEval(container);
    states.push(evalInfo.state);
    labels.push(labelText);
    if (evalInfo.state === 2) wrongLabels.push(labelText || String(result.length + 1));
    if (evalInfo.state === 3) unjudgedLabels.push(labelText || String(result.length + 1));

    const evalStr = (evalInfo.state === 1 ? 'Correct' : (evalInfo.state === 2 ? 'Wrong' : 'Unjudged'));
    result.push({ type: 'FillBlank', question: questionText, BlankNum: answers.length, answer: answers, image: images, _pta_eval: evalStr, _pta_label: labelText });
  }

  return JSON.stringify({ data: result, meta: { states, labels, wrongLabels, unjudgedLabels } });
})()
)JS";
    }

    return "JSON.stringify({data: []})";
}

PtaImportDialog::PtaImportDialog(ConfigManager *configManager, const QString &subjectName, QWidget *parent)
    : QDialog(parent)
    , m_configManager(configManager)
    , m_subjectName(subjectName)
    , m_subjectPath(configManager ? configManager->getSubjectPath(subjectName) : QString())
    , m_splitter(nullptr)
    , m_leftPanel(nullptr)
    , m_leftLayout(nullptr)
    , m_webView(nullptr)
    , m_rightPanel(nullptr)
    , m_rightLayout(nullptr)
    , m_parseButtonLayout(nullptr)
    , m_parseChoiceButton(nullptr)
    , m_parseTrueOrFalseButton(nullptr)
    , m_parseFillBlankButton(nullptr)
    , m_output(nullptr)
    , m_editor(nullptr)
    , m_bottomLayout(nullptr)
    , m_downloadImagesCheckBox(nullptr)
    , m_saveButton(nullptr)
    , m_exitButton(nullptr)
{
    setWindowTitle(QString("自动获取题库 - %1").arg(subjectName));
    resize(1600, 900);
    setMinimumSize(1400, 800);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_leftPanel = new QWidget(m_splitter);
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(10, 10, 10, 10);
    m_leftLayout->setSpacing(8);

    m_webView = new QWebEngineView(m_leftPanel);
    m_webView->setUrl(QUrl("https://pintia.cn/auth/login"));

    m_rightPanel = new QWidget(m_splitter);
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(10, 10, 10, 10);
    m_rightLayout->setSpacing(10);

    m_parseButtonLayout = new QHBoxLayout();
    m_parseChoiceButton = new QPushButton("解析当前页面的选择题", m_leftPanel);
    m_parseTrueOrFalseButton = new QPushButton("解析当前页面的判断题", m_leftPanel);
    m_parseFillBlankButton = new QPushButton("解析当前页面的填空题", m_leftPanel);
    m_parseButtonLayout->addWidget(m_parseChoiceButton);
    m_parseButtonLayout->addWidget(m_parseTrueOrFalseButton);
    m_parseButtonLayout->addWidget(m_parseFillBlankButton);

    m_leftLayout->addWidget(m_webView, 1);
    m_leftLayout->addLayout(m_parseButtonLayout);

    m_output = new QPlainTextEdit(m_rightPanel);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(2000);
    m_output->setMinimumHeight(120);
    m_rightLayout->addWidget(m_output);

    m_editor = new BankEditorWidget(m_rightPanel);
    m_editor->setEmbeddedMode(true);
    m_rightLayout->addWidget(m_editor, 1);

    m_bottomLayout = new QHBoxLayout();
    m_downloadImagesCheckBox = new QCheckBox("保存时下载图片", m_rightPanel);
    m_downloadImagesCheckBox->setChecked(true);
    m_saveButton = new QPushButton("保存当前题库", m_rightPanel);
    m_exitButton = new QPushButton("退出界面", m_rightPanel);
    m_bottomLayout->addWidget(m_downloadImagesCheckBox);
    m_bottomLayout->addStretch();
    m_bottomLayout->addWidget(m_saveButton);
    m_bottomLayout->addWidget(m_exitButton);
    m_rightLayout->addLayout(m_bottomLayout);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_splitter);
    setLayout(mainLayout);

    m_splitter->setChildrenCollapsible(false);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes(QList<int>() << 800 << 800);

    connect(m_parseChoiceButton, &QPushButton::clicked, this, &PtaImportDialog::parseChoice);
    connect(m_parseTrueOrFalseButton, &QPushButton::clicked, this, &PtaImportDialog::parseTrueOrFalse);
    connect(m_parseFillBlankButton, &QPushButton::clicked, this, &PtaImportDialog::parseFillBlank);
    connect(m_saveButton, &QPushButton::clicked, this, &PtaImportDialog::saveCurrentBank);
    connect(m_exitButton, &QPushButton::clicked, this, &QDialog::reject);

    logLine(QString("当前科目：%1").arg(m_subjectName));
    logLine(QString("科目路径：%1").arg(QDir::toNativeSeparators(m_subjectPath)));
}

PtaImportDialog::~PtaImportDialog()
{
}

void PtaImportDialog::logLine(const QString &line)
{
    m_output->appendPlainText(line);
}

QString PtaImportDialog::createTempBankFilePath(const QString &typeStr) const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    const QString name = QString("problemx_pta_%1_%2.json").arg(typeStr, ts);
    return QDir(base).filePath(name);
}

QString PtaImportDialog::typeToFolder(const QString &typeStr) const
{
    if (typeStr == "Choice") {
        return "Choice";
    }
    if (typeStr == "TrueorFalse") {
        return "TrueorFalse";
    }
    if (typeStr == "FillBlank") {
        return "FillBlank";
    }
    return "Choice";
}

void PtaImportDialog::loadParsedJsonToEditor(const QString &jsonText, const QString &typeStr)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logLine(QString("解析失败：%1").arg(err.errorString()));
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("data") || !root.value("data").isArray()) {
        logLine("解析失败：缺少 data 数组");
        return;
    }

    QJsonArray data = root.value("data").toArray();
    logLine(QString("解析完成：共 %1 题").arg(data.size()));
    if (data.isEmpty()) {
        return;
    }

    QVector<int> states;
    QStringList labels;
    if (root.value("meta").isObject()) {
        const QJsonObject meta = root.value("meta").toObject();
        if (meta.value("states").isArray()) {
            const QJsonArray arr = meta.value("states").toArray();
            states.reserve(arr.size());
            for (const auto &v : arr) {
                states.append(v.toInt());
            }
        }
        if (meta.value("labels").isArray()) {
            const QJsonArray arr = meta.value("labels").toArray();
            for (const auto &v : arr) {
                labels.append(v.toString());
            }
        }
    }

    QStringList wrongIndices;
    QStringList unjudgedIndices;
    for (int i = 0; i < states.size(); ++i) {
        if (states[i] == 2) {
            wrongIndices.append(QString::number(i + 1));
        } else if (states[i] == 3) {
            unjudgedIndices.append(QString::number(i + 1));
        }
    }

    if (!wrongIndices.isEmpty()) {
        logLine(QString("发现错误题目：%1 题").arg(wrongIndices.size()));
        logLine(QString("错误题号：%1").arg(wrongIndices.join(", ")));
        logLine("请优先更正错误题目的答案。");
    }
    if (!unjudgedIndices.isEmpty()) {
        logLine(QString("发现未评测题目：%1 题").arg(unjudgedIndices.size()));
        logLine(QString("未评测题号：%1").arg(unjudgedIndices.join(", ")));
        logLine("请确认并核对未评测题目的答案正确性。");
    }
    if (wrongIndices.isEmpty() && unjudgedIndices.isEmpty() && !states.isEmpty()) {
        logLine("所有题目已评测且答案正确。");
    }

    m_currentType = typeStr;
    m_tempFilePath = createTempBankFilePath(typeStr);

    QFile f(m_tempFilePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logLine("无法写入临时文件。");
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    m_editor->openBankFile(m_tempFilePath, QString("PTA抓取 - %1").arg(typeStr));
    if (!states.isEmpty()) {
        m_editor->setQuestionEvalStates(states, labels);
    }
}

void PtaImportDialog::parseChoice()
{
    logLine("开始解析：选择题");
    const QString script = buildExtractScript("Choice");
    m_webView->page()->runJavaScript(script, [this](const QVariant &result) {
        loadParsedJsonToEditor(result.toString(), "Choice");
    });
}

void PtaImportDialog::parseTrueOrFalse()
{
    logLine("开始解析：判断题");
    const QString script = buildExtractScript("TrueorFalse");
    m_webView->page()->runJavaScript(script, [this](const QVariant &result) {
        loadParsedJsonToEditor(result.toString(), "TrueorFalse");
    });
}

void PtaImportDialog::parseFillBlank()
{
    logLine("开始解析：填空题");
    const QString script = buildExtractScript("FillBlank");
    m_webView->page()->runJavaScript(script, [this](const QVariant &result) {
        loadParsedJsonToEditor(result.toString(), "FillBlank");
    });
}

void PtaImportDialog::saveCurrentBank()
{
    if (!m_configManager) {
        return;
    }
    if (m_currentType.isEmpty() || m_tempFilePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先解析并加载题库后再保存。");
        return;
    }

    bool ok = false;
    const QString bankName = QInputDialog::getText(this, "保存题库", "题库名字：", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok) {
        return;
    }
    if (bankName.isEmpty()) {
        QMessageBox::warning(this, "保存失败", "题库名字不能为空。");
        return;
    }

    const QString folder = typeToFolder(m_currentType);
    QDir subjectDir(m_subjectPath);
    if (!subjectDir.exists()) {
        QMessageBox::warning(this, "保存失败", "科目路径不存在或无法访问。");
        return;
    }
    if (!subjectDir.mkpath(folder)) {
        QMessageBox::warning(this, "保存失败", "无法创建题型目录。");
        return;
    }

    const QString targetPath = subjectDir.filePath(folder + "/" + bankName + ".json");
    if (QFileInfo::exists(targetPath)) {
        const int ret = QMessageBox::question(this, "文件已存在",
            QString("题库文件已存在：\n%1\n\n是否覆盖？").arg(QDir::toNativeSeparators(targetPath)),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            return;
        }
    }

    if (!m_editor->saveQuestionsAs(targetPath)) {
        return;
    }

    if (m_downloadImagesCheckBox && m_downloadImagesCheckBox->isChecked()) {
        if (!downloadImagesAndRewriteJson(targetPath)) {
            logLine("图片下载未完全成功，已尽可能保留原链接。");
        }
    }

    m_configManager->refreshSubjectBanks(m_subjectName);
    if (!m_configManager->saveConfig()) {
        logLine(QString("配置保存失败：%1").arg(m_configManager->getLastError()));
    }

    QMessageBox::information(this, "保存成功", QString("题库已保存：\n%1").arg(QDir::toNativeSeparators(targetPath)));
}

QString PtaImportDialog::pickUniqueAssetFilePath(const QString &assetDir, const QString &fileName) const
{
    QFileInfo fi(fileName);
    const QString baseName = fi.completeBaseName().isEmpty() ? "image" : fi.completeBaseName();
    const QString suffix = fi.suffix();

    QString candidate = QDir(assetDir).filePath(fileName);
    if (!QFileInfo::exists(candidate)) {
        return candidate;
    }

    for (int i = 2; i < 1000; ++i) {
        const QString name = suffix.isEmpty()
            ? QString("%1_%2").arg(baseName).arg(i)
            : QString("%1_%2.%3").arg(baseName).arg(i).arg(suffix);
        candidate = QDir(assetDir).filePath(name);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    const QString name = suffix.isEmpty()
        ? QString("%1_%2").arg(baseName).arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"))
        : QString("%1_%2.%3").arg(baseName).arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")).arg(suffix);
    return QDir(assetDir).filePath(name);
}

bool PtaImportDialog::downloadImagesAndRewriteJson(const QString &jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray content = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(content, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.value("data").isArray()) {
        return false;
    }

    const QString bankDir = QFileInfo(jsonPath).absolutePath();
    const QString assetDir = QDir(bankDir).filePath("asset");
    QDir().mkpath(assetDir);

    QNetworkAccessManager manager;
    bool allOk = true;

    QJsonArray data = root.value("data").toArray();
    for (int i = 0; i < data.size(); ++i) {
        if (!data[i].isObject()) {
            continue;
        }
        QJsonObject q = data[i].toObject();
        if (!q.value("image").isObject()) {
            continue;
        }

        QJsonObject imageObj = q.value("image").toObject();
        bool changed = false;

        for (auto it = imageObj.begin(); it != imageObj.end(); ++it) {
            const QString key = it.key();
            const QString urlStr = it.value().toString().trimmed();
            if (!(urlStr.startsWith("http://", Qt::CaseInsensitive) || urlStr.startsWith("https://", Qt::CaseInsensitive))) {
                continue;
            }

            const QUrl url(urlStr);
            QString fileName = url.fileName();
            if (fileName.isEmpty()) {
                fileName = key + ".bin";
            }

            const QString localPath = pickUniqueAssetFilePath(assetDir, fileName);

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::UserAgentHeader, "ProblemX/1.0");
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

            QEventLoop loop;
            QNetworkReply *reply = manager.get(request);
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            QByteArray bytes;
            if (reply->error() == QNetworkReply::NoError) {
                bytes = reply->readAll();
            }
            reply->deleteLater();

            if (bytes.isEmpty()) {
                allOk = false;
                continue;
            }

            QFile out(localPath);
            if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                allOk = false;
                continue;
            }
            out.write(bytes);
            out.close();

            const QString rel = QDir(bankDir).relativeFilePath(localPath);
            it.value() = QDir::fromNativeSeparators(rel);
            changed = true;
        }

        if (changed) {
            q["image"] = imageObj;
            data[i] = q;
        }
    }

    root["data"] = data;
    doc.setObject(root);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return allOk;
}

