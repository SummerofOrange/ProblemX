#ifndef BANKEDITORWIDGET_H
#define BANKEDITORWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "../models/question.h"
#include "../utils/markdownrenderer.h"

QT_BEGIN_NAMESPACE
class QSplitter;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QSpinBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QGroupBox;
class QScrollArea;
class QStackedWidget;
class QFrame;
QT_END_NAMESPACE

class ConfigManager;
class MarkdownRenderer;

class BankEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BankEditorWidget(QWidget *parent = nullptr);
    ~BankEditorWidget();

    void setBankInfo(const QString &subject, const QString &bankType, int bankIndex);
    void loadQuestions();
    void saveQuestions();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onQuestionListItemClicked(QListWidgetItem *item);
    void onAddQuestionClicked();
    void onDeleteQuestionClicked();
    void onSaveClicked();
    void onBackClicked();
    void onQuestionContentChanged();
    void onChoiceCountChanged(int count);
    void onBlankCountChanged(int count);
    void onQuestionTypeChanged(const QString &type);

signals:
    void backRequested();
    void questionsSaved();

private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    
    // Setup editor widgets for different question types
    void setupChoiceEditor();
    void setupTrueOrFalseEditor();
    void setupMultiChoiceEditor();
    void setupFillBlankEditor();
    
    // Setup preview widgets for different question types
    void setupChoicePreview();
    void setupTrueOrFalsePreview();
    void setupMultiChoicePreview();
    void setupFillBlankPreview();
    
    void loadQuestionToEditor(int index);
    void saveCurrentQuestion();
    void updateQuestionList();
    void updatePreview();
    void clearEditor();
    void setupEditorForType(QuestionType type);
    
    bool validateCurrentQuestion();
    Question createQuestionFromEditor();
    void populateEditorFromQuestion(const Question &question);
    
    QString escapeJsonString(const QString &str);
    QString unescapeJsonString(const QString &str);
    
    // UI Components - Main Layout
    QSplitter *m_mainSplitter;
    
    // Left Panel - Question List
    QWidget *m_leftPanel;
    QVBoxLayout *m_leftLayout;
    QLabel *m_bankInfoLabel;
    QListWidget *m_questionListWidget;
    QHBoxLayout *m_listButtonLayout;
    QPushButton *m_addQuestionButton;
    QPushButton *m_deleteQuestionButton;
    
    // Right Panel - Editor and Preview
    QWidget *m_rightPanel;
    QVBoxLayout *m_rightLayout;
    QSplitter *m_editorSplitter;
    
    // Editor Panel (Left side of right panel)
    QWidget *m_editorPanel;
    QVBoxLayout *m_editorLayout;
    QGroupBox *m_editorGroup;
    QVBoxLayout *m_editorGroupLayout;
    
    // Question Type Selection
    QHBoxLayout *m_typeLayout;
    QLabel *m_typeLabel;
    QComboBox *m_typeComboBox;
    
    // Question Content Editor
    QLabel *m_questionLabel;
    QTextEdit *m_questionTextEdit;
    
    // Dynamic Editor Stack
    QStackedWidget *m_editorStack;
    
    // Choice Question Editor
    QWidget *m_choiceEditorWidget;
    QVBoxLayout *m_choiceEditorLayout;
    QHBoxLayout *m_choiceCountLayout;
    QLabel *m_choiceCountLabel;
    QSpinBox *m_choiceCountSpinBox;
    QVector<QLineEdit*> m_choiceEdits;
    QLabel *m_choiceAnswerLabel;
    QComboBox *m_choiceAnswerComboBox;
    
    // TrueOrFalse Question Editor
    QWidget *m_trueOrFalseEditorWidget;
    QVBoxLayout *m_trueOrFalseEditorLayout;
    QLabel *m_trueOrFalseAnswerLabel;
    QComboBox *m_trueOrFalseAnswerComboBox;
    
    // MultiChoice Question Editor
    QWidget *m_multiChoiceEditorWidget;
    QVBoxLayout *m_multiChoiceEditorLayout;
    QHBoxLayout *m_multiChoiceCountLayout;
    QLabel *m_multiChoiceCountLabel;
    QSpinBox *m_multiChoiceCountSpinBox;
    QVector<QLineEdit*> m_multiChoiceEdits;
    QLabel *m_multiChoiceAnswerLabel;
    QTextEdit *m_multiChoiceAnswerEdit;
    
    // FillBlank Question Editor
    QWidget *m_fillBlankEditorWidget;
    QVBoxLayout *m_fillBlankEditorLayout;
    QHBoxLayout *m_blankCountLayout;
    QLabel *m_blankCountLabel;
    QSpinBox *m_blankCountSpinBox;
    QVector<QLineEdit*> m_blankAnswerEdits;
    
    // Preview Panel (Right side of right panel)
    QWidget *m_previewPanel;
    QVBoxLayout *m_previewLayout;
    QGroupBox *m_previewGroup;
    QVBoxLayout *m_previewGroupLayout;
    QScrollArea *m_previewScrollArea;
    QWidget *m_previewContent;
    QVBoxLayout *m_previewContentLayout;
    MarkdownRenderer *m_previewRenderer;
    QStackedWidget *m_previewStack;
    
    // Preview widgets for different question types
    QWidget *m_choicePreviewWidget;
    QVBoxLayout *m_choicePreviewLayout;
    QVector<MarkdownRenderer*> m_choicePreviewRenderers;
    
    QWidget *m_trueOrFalsePreviewWidget;
    QVBoxLayout *m_trueOrFalsePreviewLayout;
    QLabel *m_trueOrFalsePreviewLabel;
    
    QWidget *m_multiChoicePreviewWidget;
    QVBoxLayout *m_multiChoicePreviewLayout;
    QVector<MarkdownRenderer*> m_multiChoicePreviewRenderers;
    
    QWidget *m_fillBlankPreviewWidget;
    QVBoxLayout *m_fillBlankPreviewLayout;
    QLabel *m_fillBlankPreviewLabel;
    
    // Bottom Buttons
    QHBoxLayout *m_bottomButtonLayout;
    QPushButton *m_saveButton;
    QPushButton *m_backButton;
    
    // Data
    QString m_currentSubject;
    QString m_currentBankType;
    int m_currentBankIndex;
    QString m_bankFilePath;
    QVector<Question> m_questions;
    int m_currentQuestionIndex;
    bool m_isLoading;
    bool m_hasUnsavedChanges;
};

#endif // BANKEDITORWIDGET_H