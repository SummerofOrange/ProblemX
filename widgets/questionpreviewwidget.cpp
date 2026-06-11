#include "questionpreviewwidget.h"
#include "ui_questionpreviewwidget.h"

#include "../utils/markdownrenderer.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>

QuestionPreviewWidget::QuestionPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QuestionPreviewWidget)
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
    delete ui;
}

void QuestionPreviewWidget::setupUI()
{
    ui->setupUi(this);

    m_mainLayout = ui->mainLayout;
    m_typeLabel = ui->typeLabel;
    m_scrollArea = ui->scrollArea;
    m_scrollContent = ui->scrollContent;
    m_contentLayout = ui->contentLayout;
    m_questionRenderer = ui->questionRenderer;
    m_choicesContainer = ui->choicesContainer;
    m_choicesLayout = ui->choicesLayout;
    m_answerContainer = ui->answerContainer;
    m_answerLayout = ui->answerLayout;
    m_answerTitleLabel = ui->answerTitleLabel;
    m_answerContentLabel = ui->answerContentLabel;

    m_questionRenderer->setAutoResize(true, 1000);
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
