#include "questionpreviewwidget.h"

#include "../utils/markdownrenderer.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>

QuestionPreviewWidget::QuestionPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_typeLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollContent(nullptr)
    , m_contentLayout(nullptr)
    , m_questionRenderer(nullptr)
    , m_choicesContainer(nullptr)
    , m_choicesLayout(nullptr)
    , m_answerContainer(nullptr)
    , m_answerLayout(nullptr)
    , m_answerTitleLabel(nullptr)
    , m_answerContentLabel(nullptr)
{
    setupUI();
}

QuestionPreviewWidget::~QuestionPreviewWidget()
{
}

void QuestionPreviewWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(8);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setStyleSheet("font-weight: bold; color: #555; padding: 4px;");

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_scrollContent = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setContentsMargins(8, 8, 8, 8);
    m_contentLayout->setSpacing(12);

    m_questionRenderer = new MarkdownRenderer(m_scrollContent);
    m_questionRenderer->setAutoResize(true, 1000); // 增加最大高度限制，避免过早截断

    m_choicesContainer = new QWidget(m_scrollContent);
    m_choicesLayout = new QVBoxLayout(m_choicesContainer);
    m_choicesLayout->setContentsMargins(0, 0, 0, 0);
    m_choicesLayout->setSpacing(8);

    m_contentLayout->addWidget(m_questionRenderer);
    m_contentLayout->addWidget(m_choicesContainer);
    m_contentLayout->addStretch();

    m_scrollContent->setLayout(m_contentLayout);
    m_scrollArea->setWidget(m_scrollContent);

    // Answer Container
    m_answerContainer = new QWidget(this);
    m_answerContainer->setObjectName("answerContainer");
    m_answerContainer->setStyleSheet(
        "#answerContainer { background-color: #f0f7ff; border: 1px solid #cce5ff; border-radius: 6px; }"
    );
    
    m_answerLayout = new QVBoxLayout(m_answerContainer);
    m_answerLayout->setContentsMargins(12, 12, 12, 12);
    m_answerLayout->setSpacing(4);

    m_answerTitleLabel = new QLabel("参考答案", m_answerContainer);
    m_answerTitleLabel->setStyleSheet("font-weight: bold; color: #004085; font-size: 13px;");
    
    m_answerContentLabel = new QLabel(m_answerContainer);
    m_answerContentLabel->setWordWrap(true);
    m_answerContentLabel->setStyleSheet("color: #004085; font-size: 14px; font-weight: bold;");
    m_answerContentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_answerLayout->addWidget(m_answerTitleLabel);
    m_answerLayout->addWidget(m_answerContentLabel);

    m_mainLayout->addWidget(m_typeLabel);
    m_mainLayout->addWidget(m_scrollArea, 1);
    m_mainLayout->addWidget(m_answerContainer);

    setLayout(m_mainLayout);
    clear();
}

void QuestionPreviewWidget::clear()
{
    m_typeLabel->setText("题目类型：-");
    m_answerContentLabel->setText("-");
    m_questionRenderer->setContent("");

    while (m_choicesLayout->count() > 0) {
        QLayoutItem *item = m_choicesLayout->takeAt(0);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

static QString questionTypeToText(QuestionType t)
{
    switch (t) {
    case QuestionType::Choice:
        return "选择题";
    case QuestionType::TrueOrFalse:
        return "判断题";
    case QuestionType::FillBlank:
        return "填空题";
    case QuestionType::MultipleChoice:
        return "多选题";
    }
    return "选择题";
}

QString QuestionPreviewWidget::buildAnswerText(const Question &question) const
{
    if (question.getType() == QuestionType::FillBlank) {
        const QStringList ans = question.getAnswers();
        if (ans.isEmpty()) {
            return "-";
        }
        return ans.join(" | ");
    }

    const QString a = question.getSingleAnswer().trimmed();
    return a.isEmpty() ? "-" : a;
}

void QuestionPreviewWidget::rebuildChoices(const Question &question)
{
    while (m_choicesLayout->count() > 0) {
        QLayoutItem *item = m_choicesLayout->takeAt(0);
        if (item && item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QStringList choices = question.getChoices();
    if (choices.isEmpty()) {
        m_choicesContainer->setVisible(false);
        return;
    }

    m_choicesContainer->setVisible(true);
    for (const QString &c : choices) {
        MarkdownRenderer *r = new MarkdownRenderer(m_choicesContainer);
        r->setAutoResize(true, 1000); // 增加选项渲染高度限制
        r->setContent(c, question.getImages());
        r->setStyleSheet("MarkdownRenderer { border: 1px solid #eee; border-radius: 4px; padding: 4px; background: #fff; }");
        m_choicesLayout->addWidget(r);
    }
}

void QuestionPreviewWidget::setQuestion(const Question &question)
{
    m_typeLabel->setText(QString("题目类型：%1").arg(questionTypeToText(question.getType())));
    m_answerContentLabel->setText(buildAnswerText(question));

    m_questionRenderer->setContent(question.getQuestion(), question.getImages());
    rebuildChoices(question);
}
