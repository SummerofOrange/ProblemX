#include "ptaassistcontroller.h"

#include "../models/question.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PtaAssistController::PtaAssistController(QObject *parent)
    : QObject(parent)
    , m_view(nullptr)
{
}

void PtaAssistController::setWebView(QWebEngineView *view)
{
    m_view = view;
}

QWebEngineView *PtaAssistController::webView() const
{
    return m_view;
}

QString PtaAssistController::buildParseScript() const
{
    return R"JS(
(() => {
  function normalizeText(s) {
    if (!s) return '';
    const out = [];
    for (const ch of Array.from(String(s))) {
      const code = ch.codePointAt(0);
      const isCjk =
        (code >= 0x4E00 && code <= 0x9FFF) ||
        (code >= 0x3400 && code <= 0x4DBF) ||
        (code >= 0x20000 && code <= 0x2A6DF) ||
        (code >= 0x2A700 && code <= 0x2B73F) ||
        (code >= 0x2B740 && code <= 0x2B81F) ||
        (code >= 0x2B820 && code <= 0x2CEAF);
      const isAlnum = /[0-9A-Za-z]/.test(ch);
      if (isCjk || isAlnum) out.push(ch.toLowerCase());
    }
    return out.join('');
  }

  function extractLabel(container) {
    const btn = container.querySelector('button .pc-text-raw.text-xs.ellipsis') || container.querySelector('button .pc-text-raw');
    return btn ? (btn.innerText || '').trim() : '';
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

  function parseChoice(container) {
    const optionLabels = container.querySelectorAll('label.w-full');
    const options = [];
    for (const label of optionLabels) {
      const input = label.querySelector('input[type=radio]');
      const optionTagElem = label.querySelector('span');
      const optionTextBlock = label.querySelector('div.rendered-markdown') || label.querySelector('div');
      if (!input || !optionTagElem || !optionTextBlock) continue;
      const optionTag = (optionTagElem.innerText || '').trim().replace('.', '');
      const optionText = (optionTextBlock.innerText || '').trim();
      if (!optionTag || !optionText) continue;
      options.push({ tag: optionTag, text: optionText });
    }
    return options.length ? options : null;
  }

  function parseTrueOrFalse(container) {
    const radios = container.querySelectorAll('input[type=radio]');
    const found = [];
    for (const radio of radios) {
      const label = radio.closest('label');
      if (!label) continue;
      const txt = (label.innerText || '').trim();
      if (txt) found.push(txt);
    }
    if (found.includes('T') && found.includes('F')) return true;
    return false;
  }

  function parseFillBlank(container, cloneForText) {
    const blanks = Array.from(cloneForText.querySelectorAll('span[data-blank="true"]'));
    if (!blanks.length) return { ok: false, blanks: 0 };
    blanks.forEach((span, idx) => {
      const i = idx + 1;
      const placeholder = document.createTextNode(`【第${i}空】`);
      span.replaceWith(placeholder);
    });
    return { ok: true, blanks: blanks.length };
  }

  const result = [];
  const containers = document.querySelectorAll('div.pc-x[id]');
  for (const container of containers) {
    const id = (container.getAttribute('id') || '').trim();
    const questionElem = container.querySelector('div.markdownBlock_tErSz > div.rendered-markdown') || container.querySelector('div.rendered-markdown');
    if (!questionElem) continue;
    const extracted = extractQuestionAndImages(questionElem);
    if (!extracted.text) continue;

    const label = extractLabel(container);

    const choiceOptions = parseChoice(container);
    if (choiceOptions) {
      result.push({
        id,
        label,
        type: 'Choice',
        question: extracted.text,
        choices: choiceOptions,
        BlankNum: 0,
        image: extracted.images
      });
      continue;
    }

    const clone = questionElem.cloneNode(true);
    const extracted2 = extractQuestionAndImages(clone);
    const fill = parseFillBlank(container, clone);
    if (fill.ok) {
      const text = (clone.innerText || '').trim();
      result.push({
        id,
        label,
        type: 'FillBlank',
        question: text || extracted.text,
        choices: [],
        BlankNum: fill.blanks,
        image: Object.assign({}, extracted.images, extracted2.images)
      });
      continue;
    }

    if (parseTrueOrFalse(container)) {
      result.push({
        id,
        label,
        type: 'TrueorFalse',
        question: extracted.text,
        choices: [],
        BlankNum: 0,
        image: extracted.images
      });
      continue;
    }
  }

  return JSON.stringify({ data: result });
})()
)JS";
}

QString PtaAssistController::buildScrollScript(const QString &id) const
{
    QJsonObject obj;
    obj["id"] = id;
    const QString payload = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    return QString(R"JS(
(() => {
  const payload = %1;
  const id = payload.id || '';
  if (!id) return 'no-id';
  const el = document.getElementById(id);
  if (!el) return 'not-found';
  el.scrollIntoView({ behavior: 'smooth', block: 'start' });
  return 'ok';
})()
)JS").arg(payload);
}

QString PtaAssistController::buildFillScript(const QString &ptaId, const QJsonObject &payload) const
{
    QJsonObject root = payload;
    root["ptaId"] = ptaId;
    const QString payloadJson = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));

    return QString(R"JS(
(() => {
  function normalizeText(s) {
    if (!s) return '';
    const out = [];
    for (const ch of Array.from(String(s))) {
      const code = ch.codePointAt(0);
      const isCjk =
        (code >= 0x4E00 && code <= 0x9FFF) ||
        (code >= 0x3400 && code <= 0x4DBF) ||
        (code >= 0x20000 && code <= 0x2A6DF) ||
        (code >= 0x2A700 && code <= 0x2B73F) ||
        (code >= 0x2B740 && code <= 0x2B81F) ||
        (code >= 0x2B820 && code <= 0x2CEAF);
      const isAlnum = /[0-9A-Za-z]/.test(ch);
      if (isCjk || isAlnum) out.push(ch.toLowerCase());
    }
    return out.join('');
  }

  function makeGrams(s) {
    const t = normalizeText(s);
    if (!t) return [];
    const n = (t.length < 4) ? 1 : 2;
    if (t.length <= n) return [t];
    const out = [];
    for (let i = 0; i + n <= t.length; i++) out.push(t.slice(i, i + n));
    return Array.from(new Set(out));
  }

  function dice(a, b) {
    const ga = makeGrams(a);
    const gb = makeGrams(b);
    if (!ga.length || !gb.length) return 0;
    const setB = new Set(gb);
    let inter = 0;
    for (const g of ga) if (setB.has(g)) inter++;
    return (2 * inter) / (ga.length + gb.length);
  }

  const payload = %1;
  const ptaId = payload.ptaId || '';
  if (!ptaId) return JSON.stringify({ ok: false, message: 'ptaId为空' });
  const container = document.getElementById(ptaId);
  if (!container) return JSON.stringify({ ok: false, message: '找不到题目容器' });

  const type = payload.type || '';
  if (type === 'Choice') {
    const correctText = payload.correctChoiceText || '';
    if (!correctText) return JSON.stringify({ ok: false, message: '缺少正确选项文本' });
    const labels = Array.from(container.querySelectorAll('label.w-full'));
    let best = { score: 0, input: null };
    for (const label of labels) {
      const input = label.querySelector('input[type=radio]');
      const optionTextBlock = label.querySelector('div.rendered-markdown') || label.querySelector('div');
      if (!input || !optionTextBlock) continue;
      const optionText = (optionTextBlock.innerText || '').trim();
      if (!optionText) continue;
      const s = dice(optionText, correctText);
      if (s > best.score) best = { score: s, input };
    }
    if (!best.input || best.score < 0.6) {
      return JSON.stringify({ ok: false, message: '未找到足够相似的选项', score: best.score });
    }
    best.input.click();
    return JSON.stringify({ ok: true, message: 'ok', score: best.score });
  }

  if (type === 'TrueorFalse') {
    const ans = (payload.answer || '').trim();
    if (!ans) return JSON.stringify({ ok: false, message: '缺少答案' });
    const radios = Array.from(container.querySelectorAll('input[type=radio]'));
    for (const radio of radios) {
      const label = radio.closest('label');
      if (!label) continue;
      const txt = (label.innerText || '').trim();
      if (txt === ans) {
        radio.click();
        return JSON.stringify({ ok: true, message: 'ok' });
      }
    }
    return JSON.stringify({ ok: false, message: '未找到可点击的T/F选项' });
  }

  if (type === 'FillBlank') {
    const arr = Array.isArray(payload.answers) ? payload.answers : [];
    if (!arr.length) return JSON.stringify({ ok: false, message: '缺少填空答案数组' });
    const blanks = Array.from(container.querySelectorAll('span[data-blank=\"true\"]'));
    if (!blanks.length) return JSON.stringify({ ok: false, message: '未找到填空输入框' });
    const count = Math.min(arr.length, blanks.length);
    for (let i = 0; i < count; i++) {
      const input = blanks[i].querySelector('input[data-blank=\"true\"]') || blanks[i].querySelector('input');
      if (!input) continue;
      input.value = String(arr[i] ?? '');
      input.dispatchEvent(new Event('input', { bubbles: true }));
      input.dispatchEvent(new Event('change', { bubbles: true }));
    }
    return JSON.stringify({ ok: true, message: 'ok', filled: count });
  }

  return JSON.stringify({ ok: false, message: '不支持的题型' });
})()
)JS").arg(payloadJson);
}

void PtaAssistController::parseVisibleQuestions()
{
    if (!m_view || !m_view->page()) {
        emit parsedJsonReady("JSON.stringify({data: []})");
        return;
    }
    const QString script = buildParseScript();
    m_view->page()->runJavaScript(script, [this](const QVariant &result) {
        emit parsedJsonReady(result.toString());
    });
}

void PtaAssistController::scrollToQuestionId(const QString &id)
{
    if (!m_view || !m_view->page()) {
        return;
    }
    const QString script = buildScrollScript(id);
    m_view->page()->runJavaScript(script);
}

void PtaAssistController::fillFromBankQuestion(const QString &ptaId, const Question &bankQuestion)
{
    if (!m_view || !m_view->page()) {
        emit fillFinished(ptaId, false, "WebView未初始化");
        return;
    }

    QJsonObject payload;
    const QString typeStr = Question::typeToString(bankQuestion.getType());
    payload["type"] = typeStr;

    if (bankQuestion.getType() == QuestionType::Choice) {
        const QString ans = bankQuestion.getSingleAnswer().trimmed();
        const QStringList choices = bankQuestion.getChoices();
        QString correctText;
        for (const QString &c : choices) {
            if (c.startsWith(ans + ".", Qt::CaseInsensitive)) {
                correctText = c.mid(ans.size() + 1).trimmed();
                break;
            }
        }
        payload["answer"] = ans;
        payload["correctChoiceText"] = correctText;
    } else if (bankQuestion.getType() == QuestionType::TrueOrFalse) {
        payload["answer"] = bankQuestion.getSingleAnswer().trimmed();
    } else if (bankQuestion.getType() == QuestionType::FillBlank) {
        QJsonArray arr;
        for (const QString &a : bankQuestion.getAnswers()) {
            arr.append(a);
        }
        payload["answers"] = arr;
    } else {
        emit fillFinished(ptaId, false, "暂不支持该题型");
        return;
    }

    const QString script = buildFillScript(ptaId, payload);
    m_view->page()->runJavaScript(script, [this, ptaId](const QVariant &result) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(result.toString().toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            emit fillFinished(ptaId, false, "填入脚本返回解析失败");
            return;
        }
        const QJsonObject obj = doc.object();
        emit fillFinished(ptaId, obj.value("ok").toBool(), obj.value("message").toString());
    });
}

