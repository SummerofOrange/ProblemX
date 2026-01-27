#ifndef QUESTIONPREVIEWWIDGET_H
#define QUESTIONPREVIEWWIDGET_H

#include <QWidget>

#include "../models/question.h"

class QLabel;
class QVBoxLayout;
class QScrollArea;
class QWidget;
class MarkdownRenderer;

class QuestionPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuestionPreviewWidget(QWidget *parent = nullptr);
    ~QuestionPreviewWidget();

    void clear();
    void setQuestion(const Question &question);

private:
    void setupUI();
    void rebuildChoices(const Question &question);
    QString buildAnswerText(const Question &question) const;

    QVBoxLayout *m_mainLayout;
    QLabel *m_typeLabel;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_contentLayout;
    MarkdownRenderer *m_questionRenderer;
    QWidget *m_choicesContainer;
    QVBoxLayout *m_choicesLayout;
    
    // 答案区域
    QWidget *m_answerContainer;
    QVBoxLayout *m_answerLayout;
    QLabel *m_answerTitleLabel;
    QLabel *m_answerContentLabel;

};

#endif // QUESTIONPREVIEWWIDGET_H
