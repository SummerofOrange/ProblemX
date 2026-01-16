#include "bankeditorwidget.h"
#include "../core/configmanager.h"
#include "../models/questionbank.h"
#include <QShowEvent>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QStyle>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QFile>
#include <QResizeEvent>
#include <QTextOption>
#include <QtMath>
#include <QColor>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QAbstractItemView>

class EvalListItemDelegate : public QStyledItemDelegate
{
public:
    explicit EvalListItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(44);
        return s;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();

        QRect r = option.rect.adjusted(2, 2, -2, -2);

        QBrush bg = index.data(Qt::BackgroundRole).value<QBrush>();
        if (bg.style() == Qt::NoBrush) {
            bg = QBrush(Qt::white);
        }

        QColor borderColor("#e9ecef");
        int borderWidth = 1;
        if (option.state & QStyle::State_Selected) {
            borderColor = QColor("#357abd");
            borderWidth = 2;
        } else if (option.state & QStyle::State_MouseOver) {
            borderColor = QColor("#4A90E2");
            borderWidth = 1;
        }

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(borderColor, borderWidth));
        painter->setBrush(bg);
        painter->drawRoundedRect(r, 8, 8);

        QString text = index.data(Qt::DisplayRole).toString();
        painter->setPen(QColor("#2c3e50"));
        QRect textRect = r.adjusted(12, 0, -12, 0);
        QFontMetrics fm(option.font);
        text = fm.elidedText(text, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        painter->restore();
    }
};

BankEditorWidget::BankEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentQuestionIndex(-1)
    , m_isLoading(false)
    , m_hasUnsavedChanges(false)
{
    setupUI();
    setupConnections();
    applyStyles();
}

BankEditorWidget::~BankEditorWidget()
{
}

void BankEditorWidget::openBankFile(const QString &filePath, const QString &displayTitle)
{
    m_bankFilePath = filePath;
    const QString title = displayTitle.isEmpty() ? QFileInfo(filePath).completeBaseName() : displayTitle;
    m_bankInfoLabel->setText(title.isEmpty() ? "题库编辑器" : title);
    loadQuestions();
}

void BankEditorWidget::setQuestionEvalStates(const QVector<int> &states, const QStringList &labels)
{
    m_questionEvalStates = states;
    m_questionEvalLabels = labels;
    updateQuestionList();
}

void BankEditorWidget::setBankInfo(const QString &subject, const QString &bankType, int bankIndex)
{
    m_currentSubject = subject;
    m_currentBankType = bankType;
    m_currentBankIndex = bankIndex;
    
    // 构建题库文件路径
    QString bankDir;
    if (bankType.startsWith("choice")) {
        bankDir = "Choice";
    } else if (bankType.startsWith("trueorfalse")) {
        bankDir = "TrueorFalse";
    } else if (bankType.startsWith("fillblank")) {
        bankDir = "FillBlank";
    } else if (bankType.startsWith("multichoice")) {
        bankDir = "MultiChoice";
    }
    
    // 使用ConfigManager获取科目路径和题库信息
    ConfigManager configManager;
    QString subjectPath = configManager.getSubjectPath(subject);
    QuestionBank bank = configManager.getQuestionBank(subject);
    
    // 根据题库类型和索引获取正确的题库文件名
    QString bankFileName;
    QuestionBankInfo bankInfo;
    bool found = false;
    
    if (bankType == "choice") {
        QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
        if (bankIndex >= 0 && bankIndex < choiceBanks.size()) {
            bankInfo = choiceBanks[bankIndex];
            found = true;
        }
    } else if (bankType == "trueorfalse") {
        QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
        if (bankIndex >= 0 && bankIndex < trueOrFalseBanks.size()) {
            bankInfo = trueOrFalseBanks[bankIndex];
            found = true;
        }
    } else if (bankType == "fillblank") {
        QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
        if (bankIndex >= 0 && bankIndex < fillBlankBanks.size()) {
            bankInfo = fillBlankBanks[bankIndex];
            found = true;
        }
    }
    
    if (found) {
        bankFileName = bankInfo.name;
    } else {
        qDebug() << "Warning: Could not find bank info for" << bankType << "index" << bankIndex;
        bankFileName = subject; // 回退到使用科目名称
    }
    
    if (found && !bankInfo.src.isEmpty()) {
        m_bankFilePath = QDir(subjectPath).filePath(bankInfo.src);
        if (!QFileInfo::exists(m_bankFilePath) && !bankDir.isEmpty()) {
            m_bankFilePath = QDir(subjectPath).filePath(bankDir + "/" + bankInfo.src);
        }
    } else {
        m_bankFilePath = QString("%1/%2/%3.json").arg(subjectPath).arg(bankDir).arg(bankFileName);
    }
    
    qDebug() << "BankEditorWidget::setBankInfo - Subject:" << subject << "BankType:" << bankType << "Index:" << bankIndex;
    qDebug() << "BankEditorWidget::setBankInfo - Subject path:" << subjectPath;
    qDebug() << "BankEditorWidget::setBankInfo - Bank file name:" << bankFileName;
    qDebug() << "BankEditorWidget::setBankInfo - Bank file path:" << m_bankFilePath;
    
    // 更新界面标题
    QString bankName = QString("%1 - %2 - %3").arg(subject).arg(bankDir).arg(bankFileName);
    m_bankInfoLabel->setText(bankName);
    
    loadQuestions();
}

void BankEditorWidget::loadQuestions()
{
    qDebug() << "BankEditorWidget::loadQuestions() - Starting to load questions from:" << m_bankFilePath;
    
    m_questions.clear();
    m_questionListWidget->clear();
    
    QFile file(m_bankFilePath);
    if (!file.exists()) {
        qDebug() << "Bank file does not exist:" << m_bankFilePath;
        QMessageBox::warning(this, "文件不存在", QString("题库文件不存在:\n%1").arg(m_bankFilePath));
        return;
    }
    
    qDebug() << "Bank file exists, attempting to open...";
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open bank file:" << m_bankFilePath;
        QMessageBox::warning(this, "文件打开失败", QString("无法打开题库文件:\n%1").arg(m_bankFilePath));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    qDebug() << "File read successfully, data size:" << data.size() << "bytes";
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << error.errorString();
        QMessageBox::warning(this, "JSON解析错误", QString("题库文件格式错误:\n%1").arg(error.errorString()));
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    qDebug() << "JSON parsed successfully, found" << dataArray.size() << "questions";
    
    m_questionEvalStates.clear();
    m_questionEvalLabels.clear();
    for (int i = 0; i < dataArray.size(); ++i) {
        QJsonObject questionObj = dataArray[i].toObject();
        const QString eval = questionObj.value("_pta_eval").toString();
        const QString label = questionObj.value("_pta_label").toString();
        int state = 0;
        if (eval == "Correct") {
            state = 1;
        } else if (eval == "Wrong") {
            state = 2;
        } else if (eval == "Unjudged") {
            state = 3;
        }
        m_questionEvalStates.append(state);
        m_questionEvalLabels.append(label);
        Question question(questionObj);
        m_questions.append(question);
        // qDebug() << "Loaded question" << (i+1) << "type:" << question.getType() << "question preview:" << question.getQuestion().left(50);
    }
    
    updateQuestionList();
    
    qDebug() << "Question list updated, total questions loaded:" << m_questions.size();
    
    if (!m_questions.isEmpty()) {
        m_questionListWidget->setCurrentRow(0);
        loadQuestionToEditor(0);
        qDebug() << "First question loaded to editor";
    } else {
        qDebug() << "No questions found in the bank file";
    }
}

void BankEditorWidget::saveQuestions()
{
    if (!writeQuestionsToFile(m_bankFilePath)) {
        return;
    }
    m_hasUnsavedChanges = false;
    QMessageBox::information(this, "保存成功", "题库已成功保存!");
    emit questionsSaved();
}

bool BankEditorWidget::saveQuestionsAs(const QString &filePath)
{
    if (!writeQuestionsToFile(filePath)) {
        return false;
    }

    m_bankFilePath = filePath;
    m_hasUnsavedChanges = false;
    emit questionsSaved();
    return true;
}

void BankEditorWidget::setEmbeddedMode(bool enabled)
{
    m_saveButton->setVisible(!enabled);
    m_backButton->setVisible(!enabled);
}

bool BankEditorWidget::writeQuestionsToFile(const QString &filePath)
{
    if (filePath.trimmed().isEmpty()) {
        QMessageBox::warning(this, "保存失败", "题库文件路径为空。");
        return false;
    }

    if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questions.size()) {
        saveCurrentQuestion();
    }

    QJsonObject root;
    QJsonArray dataArray;
    for (const Question &question : m_questions) {
        dataArray.append(question.toJson());
    }
    root["data"] = dataArray;

    const QString dirPath = QFileInfo(filePath).absolutePath();
    if (!dirPath.isEmpty()) {
        QDir().mkpath(dirPath);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "保存失败", "无法写入文件: " + filePath);
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void BankEditorWidget::setupUI()
{
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create left panel (question list)
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(10, 10, 10, 10);
    m_leftLayout->setSpacing(10);
    
    // Bank info label
    m_bankInfoLabel = new QLabel("题库编辑器");
    m_bankInfoLabel->setObjectName("bankInfoLabel");
    
    // Question list
    m_questionListWidget = new QListWidget();
    m_questionListWidget->setAlternatingRowColors(false);
    m_questionListWidget->setSpacing(6);
    m_questionListWidget->setUniformItemSizes(true);
    m_questionListWidget->setMouseTracking(true);
    m_questionListWidget->setItemDelegate(new EvalListItemDelegate(m_questionListWidget));
    m_questionListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    
    // List buttons
    m_listButtonLayout = new QHBoxLayout();
    m_addQuestionButton = new QPushButton("添加题目");
    m_deleteQuestionButton = new QPushButton("删除题目");
    
    m_listButtonLayout->addWidget(m_addQuestionButton);
    m_listButtonLayout->addWidget(m_deleteQuestionButton);
    
    m_leftLayout->addWidget(m_bankInfoLabel);
    m_leftLayout->addWidget(m_questionListWidget);
    m_leftLayout->addLayout(m_listButtonLayout);
    
    // Create right panel (editor and preview)
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(10, 10, 10, 10);
    m_rightLayout->setSpacing(10);
    
    // Create editor splitter
    m_editorSplitter = new QSplitter(Qt::Horizontal);
    
    // Create editor panel
    m_editorPanel = new QWidget();
    m_editorLayout = new QVBoxLayout(m_editorPanel);
    m_editorLayout->setContentsMargins(5, 5, 5, 5);
    m_editorLayout->setSpacing(10);
    
    // Editor group
    m_editorGroup = new QGroupBox("题目编辑");
    m_editorGroupLayout = new QVBoxLayout(m_editorGroup);
    
    // Question type selection
    m_typeLayout = new QHBoxLayout();
    m_typeLabel = new QLabel("题目类型:");
    m_typeComboBox = new QComboBox();
    m_typeComboBox->addItem("选择题", "Choice");
    m_typeComboBox->addItem("判断题", "TrueOrFalse");
    m_typeComboBox->addItem("多选题", "MultipleChoice");
    m_typeComboBox->addItem("填空题", "FillBlank");
    
    m_typeLayout->addWidget(m_typeLabel);
    m_typeLayout->addWidget(m_typeComboBox);
    m_typeLayout->addStretch();
    
    // Question content editor
    m_questionLabel = new QLabel("题目内容:");
    m_questionTextEdit = new QTextEdit();
    m_questionTextEdit->setMinimumHeight(150);
    m_questionTextEdit->setPlaceholderText("请输入题目内容，支持Markdown语法和LaTeX数学公式...");

    m_imageGroup = new QGroupBox("图片");
    m_imageLayout = new QVBoxLayout(m_imageGroup);

    m_imageTable = new QTableWidget();
    m_imageTable->setColumnCount(2);
    m_imageTable->setHorizontalHeaderLabels(QStringList() << "键" << "路径");
    m_imageTable->horizontalHeader()->setStretchLastSection(true);
    m_imageTable->verticalHeader()->setVisible(false);
    m_imageTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_imageTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_imageTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_imageTable->setMinimumHeight(130);

    m_imageButtonLayout = new QHBoxLayout();
    m_addImageButton = new QPushButton("添加");
    m_removeImageButton = new QPushButton("删除");
    m_chooseImageButton = new QPushButton("选择图片");
    m_imageButtonLayout->addWidget(m_addImageButton);
    m_imageButtonLayout->addWidget(m_removeImageButton);
    m_imageButtonLayout->addStretch();
    m_imageButtonLayout->addWidget(m_chooseImageButton);

    m_imageLayout->addWidget(m_imageTable);
    m_imageLayout->addLayout(m_imageButtonLayout);
    
    // Dynamic editor stack
    m_editorStack = new QStackedWidget();
    
    // Setup different question type editors
    setupChoiceEditor();
    setupTrueOrFalseEditor();
    setupMultiChoiceEditor();
    setupFillBlankEditor();
    
    m_editorGroupLayout->addLayout(m_typeLayout);
    m_editorGroupLayout->addWidget(m_questionLabel);
    m_editorGroupLayout->addWidget(m_questionTextEdit);
    m_editorGroupLayout->addWidget(m_imageGroup);
    m_editorGroupLayout->addWidget(m_editorStack);
    
    m_editorLayout->addWidget(m_editorGroup);
    
    // Create preview panel
    m_previewPanel = new QWidget();
    m_previewLayout = new QVBoxLayout(m_previewPanel);
    m_previewLayout->setContentsMargins(5, 5, 5, 5);
    m_previewLayout->setSpacing(10);
    
    // Preview group
    m_previewGroup = new QGroupBox("实时预览");
    m_previewGroupLayout = new QVBoxLayout(m_previewGroup);
    
    // Preview scroll area
    m_previewScrollArea = new QScrollArea();
    m_previewScrollArea->setWidgetResizable(true);
    m_previewScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_previewScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_previewContent = new QWidget();
    m_previewContentLayout = new QVBoxLayout(m_previewContent);
    m_previewContentLayout->setContentsMargins(10, 10, 10, 10);
    m_previewContentLayout->setSpacing(15);
    
    // Question text renderer
    m_previewRenderer = new MarkdownRenderer(this);
    m_previewRenderer->setAutoResize(true, 400);
    
    // Preview stack for different question types
    m_previewStack = new QStackedWidget();
    
    setupChoicePreview();
    setupTrueOrFalsePreview();
    setupMultiChoicePreview();
    setupFillBlankPreview();
    
    m_previewContentLayout->addWidget(m_previewRenderer);
    m_previewContentLayout->addWidget(m_previewStack);
    m_previewContentLayout->addStretch();
    
    m_previewScrollArea->setWidget(m_previewContent);
    m_previewGroupLayout->addWidget(m_previewScrollArea);
    
    m_previewLayout->addWidget(m_previewGroup);
    
    // Add panels to editor splitter
    m_editorSplitter->addWidget(m_editorPanel);
    m_editorSplitter->addWidget(m_previewPanel);
    m_editorSplitter->setStretchFactor(0, 1);
    m_editorSplitter->setStretchFactor(1, 1);
    
    // Bottom buttons
    m_bottomButtonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("保存题库");
    m_saveButton->setObjectName("saveButton");
    m_backButton = new QPushButton("返回");
    m_backButton->setObjectName("backButton");
    
    m_bottomButtonLayout->addStretch();
    m_bottomButtonLayout->addWidget(m_saveButton);
    m_bottomButtonLayout->addWidget(m_backButton);
    
    // Add to right layout
    m_rightLayout->addWidget(m_editorSplitter);
    m_rightLayout->addLayout(m_bottomButtonLayout);
    
    // Add panels to main splitter
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_mainSplitter);
    
    setLayout(mainLayout);
}

void BankEditorWidget::setupChoiceEditor()
{
    m_choiceEditorWidget = new QWidget();
    m_choiceEditorLayout = new QVBoxLayout(m_choiceEditorWidget);
    
    // Choice count
    m_choiceCountLayout = new QHBoxLayout();
    m_choiceCountLabel = new QLabel("选项数量:");
    m_choiceCountSpinBox = new QSpinBox();
    m_choiceCountSpinBox->setMinimum(2);
    m_choiceCountSpinBox->setMaximum(8);
    m_choiceCountSpinBox->setValue(4);
    m_choiceCountSpinBox->setMinimumWidth(80);  // 设置最小宽度
    
    m_choiceCountLayout->addWidget(m_choiceCountLabel);
    m_choiceCountLayout->addWidget(m_choiceCountSpinBox);
    m_choiceCountLayout->addStretch();
    
    m_choiceEditorLayout->addLayout(m_choiceCountLayout);
    
    // Choice options (will be created dynamically)
    for (int i = 0; i < 8; ++i) {
        QTextEdit *edit = new QTextEdit();
        edit->setPlaceholderText(QString("选项 %1").arg(QChar('A' + i)));
        edit->setAcceptRichText(false);
        edit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        edit->setMinimumHeight(36);
        edit->setVisible(i < 4);
        m_choiceEdits.append(edit);
        m_choiceEditorLayout->addWidget(edit);
    }
    
    // Answer selection
    m_choiceAnswerLabel = new QLabel("正确答案:");
    m_choiceAnswerComboBox = new QComboBox();
    m_choiceAnswerComboBox->addItem("A");
    m_choiceAnswerComboBox->addItem("B");
    m_choiceAnswerComboBox->addItem("C");
    m_choiceAnswerComboBox->addItem("D");
    
    m_choiceEditorLayout->addWidget(m_choiceAnswerLabel);
    m_choiceEditorLayout->addWidget(m_choiceAnswerComboBox);
    m_choiceEditorLayout->addStretch();
    
    m_editorStack->addWidget(m_choiceEditorWidget);
}

void BankEditorWidget::setupTrueOrFalseEditor()
{
    m_trueOrFalseEditorWidget = new QWidget();
    m_trueOrFalseEditorLayout = new QVBoxLayout(m_trueOrFalseEditorWidget);
    
    // Answer selection
    m_trueOrFalseAnswerLabel = new QLabel("正确答案:");
    m_trueOrFalseAnswerComboBox = new QComboBox();
    m_trueOrFalseAnswerComboBox->addItem("正确 (T)", "T");
    m_trueOrFalseAnswerComboBox->addItem("错误 (F)", "F");
    
    m_trueOrFalseEditorLayout->addWidget(m_trueOrFalseAnswerLabel);
    m_trueOrFalseEditorLayout->addWidget(m_trueOrFalseAnswerComboBox);
    m_trueOrFalseEditorLayout->addStretch();
    
    m_editorStack->addWidget(m_trueOrFalseEditorWidget);
}

void BankEditorWidget::setupMultiChoiceEditor()
{
    m_multiChoiceEditorWidget = new QWidget();
    m_multiChoiceEditorLayout = new QVBoxLayout(m_multiChoiceEditorWidget);
    
    // Choice count
    m_multiChoiceCountLayout = new QHBoxLayout();
    m_multiChoiceCountLabel = new QLabel("选项数量:");
    m_multiChoiceCountSpinBox = new QSpinBox();
    m_multiChoiceCountSpinBox->setMinimum(2);
    m_multiChoiceCountSpinBox->setMaximum(8);
    m_multiChoiceCountSpinBox->setValue(4);
    m_multiChoiceCountSpinBox->setMinimumWidth(80);  // 设置最小宽度
    
    m_multiChoiceCountLayout->addWidget(m_multiChoiceCountLabel);
    m_multiChoiceCountLayout->addWidget(m_multiChoiceCountSpinBox);
    m_multiChoiceCountLayout->addStretch();
    
    m_multiChoiceEditorLayout->addLayout(m_multiChoiceCountLayout);
    
    // Choice options (will be created dynamically)
    for (int i = 0; i < 8; ++i) {
        QTextEdit *edit = new QTextEdit();
        edit->setPlaceholderText(QString("选项 %1").arg(QChar('A' + i)));
        edit->setAcceptRichText(false);
        edit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        edit->setMinimumHeight(36);
        edit->setVisible(i < 4);
        m_multiChoiceEdits.append(edit);
        m_multiChoiceEditorLayout->addWidget(edit);
    }
    
    // Answer input
    m_multiChoiceAnswerLabel = new QLabel("正确答案 (如: A,B,C):");
    m_multiChoiceAnswerEdit = new QTextEdit();
    m_multiChoiceAnswerEdit->setMaximumHeight(60);
    m_multiChoiceAnswerEdit->setPlaceholderText("输入正确答案的字母组合，如: A,B,C");
    
    m_multiChoiceEditorLayout->addWidget(m_multiChoiceAnswerLabel);
    m_multiChoiceEditorLayout->addWidget(m_multiChoiceAnswerEdit);
    m_multiChoiceEditorLayout->addStretch();
    
    m_editorStack->addWidget(m_multiChoiceEditorWidget);
}

void BankEditorWidget::setupFillBlankEditor()
{
    m_fillBlankEditorWidget = new QWidget();
    m_fillBlankEditorLayout = new QVBoxLayout(m_fillBlankEditorWidget);
    
    // Blank count
    m_blankCountLayout = new QHBoxLayout();
    m_blankCountLabel = new QLabel("空格数量:");
    m_blankCountSpinBox = new QSpinBox();
    m_blankCountSpinBox->setMinimum(1);
    m_blankCountSpinBox->setMaximum(10);
    m_blankCountSpinBox->setValue(1);
    m_blankCountSpinBox->setMinimumWidth(80);  // 设置最小宽度
    
    m_blankCountLayout->addWidget(m_blankCountLabel);
    m_blankCountLayout->addWidget(m_blankCountSpinBox);
    m_blankCountLayout->addStretch();
    
    m_fillBlankEditorLayout->addLayout(m_blankCountLayout);
    
    // Blank answers (will be created dynamically)
    for (int i = 0; i < 10; ++i) {
        QLineEdit *edit = new QLineEdit();
        edit->setPlaceholderText(QString("第 %1 个空的答案").arg(i + 1));
        edit->setVisible(i < 1);
        m_blankAnswerEdits.append(edit);
        m_fillBlankEditorLayout->addWidget(edit);
    }
    
    m_fillBlankEditorLayout->addStretch();
    
    m_editorStack->addWidget(m_fillBlankEditorWidget);
}

void BankEditorWidget::setupChoicePreview()
{
    m_choicePreviewWidget = new QWidget();
    m_choicePreviewLayout = new QVBoxLayout(m_choicePreviewWidget);
    
    for (int i = 0; i < 8; ++i) {
        MarkdownRenderer *renderer = new MarkdownRenderer(this);
        renderer->setObjectName("choicePreviewRenderer");
        renderer->setVisible(false);
        renderer->setAutoResize(true, 200);  // 启用自动适配，最大高度200
        renderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_choicePreviewRenderers.append(renderer);
        m_choicePreviewLayout->addWidget(renderer);
    }
    
    m_choicePreviewLayout->addStretch();
    m_previewStack->addWidget(m_choicePreviewWidget);
}

void BankEditorWidget::setupTrueOrFalsePreview()
{
    m_trueOrFalsePreviewWidget = new QWidget();
    m_trueOrFalsePreviewLayout = new QVBoxLayout(m_trueOrFalsePreviewWidget);
    
    m_trueOrFalsePreviewLabel = new QLabel("○ 正确\n○ 错误");
    m_trueOrFalsePreviewLabel->setObjectName("trueOrFalsePreviewLabel");
    
    m_trueOrFalsePreviewLayout->addWidget(m_trueOrFalsePreviewLabel);
    m_trueOrFalsePreviewLayout->addStretch();
    
    m_previewStack->addWidget(m_trueOrFalsePreviewWidget);
}

void BankEditorWidget::setupMultiChoicePreview()
{
    m_multiChoicePreviewWidget = new QWidget();
    m_multiChoicePreviewLayout = new QVBoxLayout(m_multiChoicePreviewWidget);
    
    for (int i = 0; i < 8; ++i) {
        MarkdownRenderer *renderer = new MarkdownRenderer(this);
        renderer->setObjectName("multiChoicePreviewRenderer");
        renderer->setVisible(false);
        renderer->setAutoResize(true, 200);  // 启用自动适配，最大高度200
        renderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_multiChoicePreviewRenderers.append(renderer);
        m_multiChoicePreviewLayout->addWidget(renderer);
    }
    
    m_multiChoicePreviewLayout->addStretch();
    m_previewStack->addWidget(m_multiChoicePreviewWidget);
}

void BankEditorWidget::setupFillBlankPreview()
{
    m_fillBlankPreviewWidget = new QWidget();
    m_fillBlankPreviewLayout = new QVBoxLayout(m_fillBlankPreviewWidget);
    
    m_fillBlankPreviewLabel = new QLabel("填空题预览区域");
    m_fillBlankPreviewLabel->setObjectName("fillBlankPreviewLabel");
    
    m_fillBlankPreviewLayout->addWidget(m_fillBlankPreviewLabel);
    m_fillBlankPreviewLayout->addStretch();
    
    m_previewStack->addWidget(m_fillBlankPreviewWidget);
}

void BankEditorWidget::setupConnections()
{
    connect(m_questionListWidget, &QListWidget::itemClicked,
            this, &BankEditorWidget::onQuestionListItemClicked);
    
    connect(m_addQuestionButton, &QPushButton::clicked,
            this, &BankEditorWidget::onAddQuestionClicked);
    
    connect(m_deleteQuestionButton, &QPushButton::clicked,
            this, &BankEditorWidget::onDeleteQuestionClicked);
    
    connect(m_saveButton, &QPushButton::clicked,
            this, &BankEditorWidget::onSaveClicked);
    
    connect(m_backButton, &QPushButton::clicked,
            this, &BankEditorWidget::onBackClicked);
    
    connect(m_typeComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &BankEditorWidget::onQuestionTypeChanged);
    
    connect(m_questionTextEdit, &QTextEdit::textChanged,
            this, &BankEditorWidget::onQuestionContentChanged);
    
    connect(m_choiceCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BankEditorWidget::onChoiceCountChanged);
    
    connect(m_multiChoiceCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BankEditorWidget::onChoiceCountChanged);
    
    connect(m_blankCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BankEditorWidget::onBlankCountChanged);
    
    // Connect all choice edits
    for (QTextEdit *edit : m_choiceEdits) {
        connect(edit, &QTextEdit::textChanged, this, &BankEditorWidget::onQuestionContentChanged);
    }
    
    for (QTextEdit *edit : m_multiChoiceEdits) {
        connect(edit, &QTextEdit::textChanged, this, &BankEditorWidget::onQuestionContentChanged);
    }
    
    for (QLineEdit *edit : m_blankAnswerEdits) {
        connect(edit, &QLineEdit::textChanged, this, &BankEditorWidget::onQuestionContentChanged);
    }
    
    connect(m_choiceAnswerComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &BankEditorWidget::onQuestionContentChanged);
    
    connect(m_trueOrFalseAnswerComboBox, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &BankEditorWidget::onQuestionContentChanged);
    
    connect(m_multiChoiceAnswerEdit, &QTextEdit::textChanged,
            this, &BankEditorWidget::onQuestionContentChanged);

    connect(m_addImageButton, &QPushButton::clicked,
            this, &BankEditorWidget::onAddImageClicked);
    connect(m_removeImageButton, &QPushButton::clicked,
            this, &BankEditorWidget::onRemoveImageClicked);
    connect(m_chooseImageButton, &QPushButton::clicked,
            this, &BankEditorWidget::onChooseImageClicked);
    connect(m_imageTable, &QTableWidget::cellChanged,
            this, &BankEditorWidget::onImageTableChanged);
}

void BankEditorWidget::applyStyles()
{
    setStyleSheet(
        "BankEditorWidget {"
        "    background-color: #f8f9fa;"
        "}"
        
        "#bankInfoLabel {"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    padding: 5px 0;"
        "}"
        
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "    background-color: white;"
        "}"
        
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    color: #495057;"
        "}"
        
        "QListWidget {"
        "    border: 1px solid #e5e7eb;"
        "    border-radius: 10px;"
        "    background-color: #ffffff;"
        "    font-size: 13px;"
        "    padding: 6px;"
        "    outline: none;"
        "}"
        
        "QPushButton {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    background-color: white;"
        "    font-size: 13px;"
        "}"
        
        "QPushButton:hover {"
        "    background-color: #f8f9fa;"
        "    border-color: #adb5bd;"
        "}"
        
        "QPushButton:pressed {"
        "    background-color: #e9ecef;"
        "}"
        
        "#saveButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
        
        "#saveButton:hover {"
        "    background-color: #218838;"
        "}"
        
        "#backButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "}"
        
        "#backButton:hover {"
        "    background-color: #5a6268;"
        "}"
        
        "QLineEdit, QTextEdit, QSpinBox, QComboBox {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    background-color: white;"
        "}"
        
        "QLineEdit:focus, QTextEdit:focus, QSpinBox:focus, QComboBox:focus {"
        "    border-color: #4A90E2;"
        "    outline: none;"
        "}"
        
        "#choicePreviewRenderer, #multiChoicePreviewRenderer {"
        "    background-color: white;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "    margin: 2px 0;"
        "}"
        
        "#trueOrFalsePreviewLabel, #fillBlankPreviewLabel {"
        "    background-color: white;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "    margin: 2px 0;"
        "}"
    );
}

void BankEditorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void BankEditorWidget::onQuestionListItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    int index = m_questionListWidget->row(item);
    if (index >= 0 && index < m_questions.size()) {
        // Save current question before switching
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questions.size()) {
            saveCurrentQuestion();
        }
        
        loadQuestionToEditor(index);
    }
}

void BankEditorWidget::onAddQuestionClicked()
{
    // Ask user for question type
    QStringList types;
    types << "选择题" << "判断题" << "多选题" << "填空题";
    
    bool ok;
    QString selectedType = QInputDialog::getItem(this, "选择题目类型", "请选择要添加的题目类型:", types, 0, false, &ok);
    
    if (!ok) return;
    
    QuestionType type;
    if (selectedType == "选择题") {
        type = QuestionType::Choice;
    } else if (selectedType == "判断题") {
        type = QuestionType::TrueOrFalse;
    } else if (selectedType == "多选题") {
        type = QuestionType::MultipleChoice;
    } else if (selectedType == "填空题") {
        type = QuestionType::FillBlank;
    } else {
        return;
    }
    
    // Create new question
    Question newQuestion;
    newQuestion.setType(type);
    newQuestion.setQuestion("新题目");
    
    // Set default values based on type
    if (type == QuestionType::Choice) {
        newQuestion.setChoices(QStringList() << "A. 选项A" << "B. 选项B" << "C. 选项C" << "D. 选项D");
        newQuestion.setSingleAnswer("A");
    } else if (type == QuestionType::TrueOrFalse) {
        newQuestion.setSingleAnswer("T");
    } else if (type == QuestionType::MultipleChoice) {
        newQuestion.setChoices(QStringList() << "A. 选项A" << "B. 选项B" << "C. 选项C" << "D. 选项D");
        newQuestion.setSingleAnswer("AB");
    } else if (type == QuestionType::FillBlank) {
        newQuestion.setBlankNum(1);
        newQuestion.setAnswers(QStringList() << "答案1");
    }
    
    m_questions.append(newQuestion);
    m_questionEvalStates.append(0);
    m_questionEvalLabels.append(QString());
    updateQuestionList();
    
    // Select the new question
    m_questionListWidget->setCurrentRow(m_questions.size() - 1);
    loadQuestionToEditor(m_questions.size() - 1);
    
    m_hasUnsavedChanges = true;
}

void BankEditorWidget::onDeleteQuestionClicked()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questions.size()) {
        QMessageBox::warning(this, "删除失败", "请先选择要删除的题目。");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除", "确定要删除当前题目吗？此操作不可撤销。",
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_questions.removeAt(m_currentQuestionIndex);
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionEvalStates.size()) {
            m_questionEvalStates.removeAt(m_currentQuestionIndex);
        }
        if (m_currentQuestionIndex >= 0 && m_currentQuestionIndex < m_questionEvalLabels.size()) {
            m_questionEvalLabels.removeAt(m_currentQuestionIndex);
        }
        updateQuestionList();
        
        // Select next question or clear editor
        if (m_questions.isEmpty()) {
            clearEditor();
            m_currentQuestionIndex = -1;
        } else {
            int newIndex = qMin(m_currentQuestionIndex, m_questions.size() - 1);
            m_questionListWidget->setCurrentRow(newIndex);
            loadQuestionToEditor(newIndex);
        }
        
        m_hasUnsavedChanges = true;
    }
}

void BankEditorWidget::onSaveClicked()
{
    // Validate current question
    if (m_currentQuestionIndex >= 0 && !validateCurrentQuestion()) {
        return;
    }
    
    saveQuestions();
}

void BankEditorWidget::onBackClicked()
{
    if (m_hasUnsavedChanges) {
        int ret = QMessageBox::question(this, "未保存的更改", "有未保存的更改，是否保存后退出？",
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                       QMessageBox::Save);
        
        if (ret == QMessageBox::Save) {
            if (validateCurrentQuestion()) {
                saveQuestions();
                emit backRequested();
            }
        } else if (ret == QMessageBox::Discard) {
            emit backRequested();
        }
        // Cancel: do nothing
    } else {
        emit backRequested();
    }
}

void BankEditorWidget::onQuestionContentChanged()
{
    if (!m_isLoading) {
        if (QTextEdit *edit = qobject_cast<QTextEdit*>(sender())) {
            if (m_choiceEdits.contains(edit) || m_multiChoiceEdits.contains(edit)) {
                adjustOptionEditHeight(edit);
            }
        }
        m_hasUnsavedChanges = true;
        updatePreview();
    }
}

void BankEditorWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (m_isLoading) {
        return;
    }

    for (QTextEdit *edit : m_choiceEdits) {
        if (edit && edit->isVisible()) {
            adjustOptionEditHeight(edit);
        }
    }
    for (QTextEdit *edit : m_multiChoiceEdits) {
        if (edit && edit->isVisible()) {
            adjustOptionEditHeight(edit);
        }
    }
}

void BankEditorWidget::onChoiceCountChanged(int count)
{
    QSpinBox *sender = qobject_cast<QSpinBox*>(this->sender());
    if (!sender) return;
    
    if (sender == m_choiceCountSpinBox) {
        // Update choice editor
        for (int i = 0; i < m_choiceEdits.size(); ++i) {
            m_choiceEdits[i]->setVisible(i < count);
            if (i < count) {
                adjustOptionEditHeight(m_choiceEdits[i]);
            }
        }
        
        // Update answer combo box
        m_choiceAnswerComboBox->clear();
        for (int i = 0; i < count; ++i) {
            m_choiceAnswerComboBox->addItem(QString(QChar('A' + i)));
        }
    } else if (sender == m_multiChoiceCountSpinBox) {
        // Update multi-choice editor
        for (int i = 0; i < m_multiChoiceEdits.size(); ++i) {
            m_multiChoiceEdits[i]->setVisible(i < count);
            if (i < count) {
                adjustOptionEditHeight(m_multiChoiceEdits[i]);
            }
        }
    }
    
    onQuestionContentChanged();
}

void BankEditorWidget::onBlankCountChanged(int count)
{
    for (int i = 0; i < m_blankAnswerEdits.size(); ++i) {
        m_blankAnswerEdits[i]->setVisible(i < count);
    }
    
    onQuestionContentChanged();
}

void BankEditorWidget::onQuestionTypeChanged(const QString &type)
{
    Q_UNUSED(type);
    
    QString typeData = m_typeComboBox->currentData().toString();
    QuestionType questionType;
    
    if (typeData == "Choice") {
        questionType = QuestionType::Choice;
        m_editorStack->setCurrentWidget(m_choiceEditorWidget);
        m_previewStack->setCurrentWidget(m_choicePreviewWidget);
    } else if (typeData == "TrueOrFalse") {
        questionType = QuestionType::TrueOrFalse;
        m_editorStack->setCurrentWidget(m_trueOrFalseEditorWidget);
        m_previewStack->setCurrentWidget(m_trueOrFalsePreviewWidget);
    } else if (typeData == "MultipleChoice") {
        questionType = QuestionType::MultipleChoice;
        m_editorStack->setCurrentWidget(m_multiChoiceEditorWidget);
        m_previewStack->setCurrentWidget(m_multiChoicePreviewWidget);
    } else if (typeData == "FillBlank") {
        questionType = QuestionType::FillBlank;
        m_editorStack->setCurrentWidget(m_fillBlankEditorWidget);
        m_previewStack->setCurrentWidget(m_fillBlankPreviewWidget);
    }
    
    onQuestionContentChanged();
}

void BankEditorWidget::onAddImageClicked()
{
    if (m_isLoading) {
        return;
    }

    const QMap<QString, QString> current = getImagesFromTable();
    int nextIndex = 1;
    while (current.contains(QString("img%1").arg(nextIndex))) {
        ++nextIndex;
    }

    const int row = m_imageTable->rowCount();
    m_imageTable->insertRow(row);

    QTableWidgetItem *keyItem = new QTableWidgetItem(QString("img%1").arg(nextIndex));
    QTableWidgetItem *valueItem = new QTableWidgetItem("");
    m_imageTable->setItem(row, 0, keyItem);
    m_imageTable->setItem(row, 1, valueItem);
    m_imageTable->setCurrentCell(row, 0);

    m_hasUnsavedChanges = true;
    updatePreview();
}

void BankEditorWidget::onRemoveImageClicked()
{
    if (m_isLoading) {
        return;
    }

    const int row = m_imageTable->currentRow();
    if (row < 0) {
        return;
    }

    m_imageTable->removeRow(row);
    m_hasUnsavedChanges = true;
    updatePreview();
}

void BankEditorWidget::onChooseImageClicked()
{
    if (m_isLoading) {
        return;
    }

    int row = m_imageTable->currentRow();
    if (row < 0) {
        onAddImageClicked();
        row = m_imageTable->currentRow();
        if (row < 0) {
            return;
        }
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择图片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*.*)"
    );
    if (filePath.isEmpty()) {
        return;
    }

    const QString baseDir = getCurrentImageBaseDir();
    const QString jsonDir = QFileInfo(m_bankFilePath).absolutePath();
    const QString targetBaseDir = !jsonDir.trimmed().isEmpty() ? jsonDir : baseDir;

    QTableWidgetItem *valueItem = m_imageTable->item(row, 1);
    if (!valueItem) {
        valueItem = new QTableWidgetItem();
        m_imageTable->setItem(row, 1, valueItem);
    }

    QString valueToStore;
    if (!targetBaseDir.trimmed().isEmpty()) {
        const QString assetDirPath = QDir(targetBaseDir).filePath("asset");
        QDir assetDir(assetDirPath);
        if (!assetDir.exists()) {
            if (!QDir(targetBaseDir).mkpath("asset")) {
                QMessageBox::warning(this, "复制失败", QString("无法创建asset目录:\n%1").arg(assetDirPath));
                return;
            } else {
                assetDir = QDir(assetDirPath);
            }
        }

        if (assetDir.exists()) {
            const QFileInfo srcInfo(filePath);
            const QString originalName = srcInfo.fileName();
            const QString baseName = srcInfo.completeBaseName();
            const QString suffix = srcInfo.suffix();

            const QString srcAbsPath = QFileInfo(filePath).absoluteFilePath();
            const QString srcDirAbsPath = QFileInfo(filePath).absoluteDir().absolutePath();
            const QString assetDirAbsPath = QFileInfo(assetDirPath).absoluteFilePath();
            if (QDir::cleanPath(srcDirAbsPath).compare(QDir::cleanPath(assetDirAbsPath), Qt::CaseInsensitive) == 0) {
                valueToStore = QString("asset/%1").arg(originalName);
            } else {
            auto buildFileName = [&](int index) -> QString {
                if (index <= 0) {
                    return originalName;
                }
                if (suffix.isEmpty()) {
                    return QString("%1_%2").arg(baseName).arg(index);
                }
                return QString("%1_%2.%3").arg(baseName).arg(index).arg(suffix);
            };

            QString destFileName;
            QString destAbsPath;
            bool foundSlot = false;
            for (int i = 0; i < 1000; ++i) {
                destFileName = buildFileName(i);
                destAbsPath = assetDir.filePath(destFileName);
                if (!QFile::exists(destAbsPath)) {
                    foundSlot = true;
                    break;
                }
            }
            if (!foundSlot) {
                QMessageBox::warning(this, "复制失败", "asset目录下同名文件过多，无法生成新文件名");
                return;
            }

            const QString destAbsPathNormalized = QFileInfo(destAbsPath).absoluteFilePath();
            if (!QFile::copy(srcAbsPath, destAbsPathNormalized)) {
                QMessageBox::warning(this, "复制失败", QString("无法复制图片到asset目录:\n%1\n\n目标:\n%2").arg(srcAbsPath, destAbsPathNormalized));
                return;
            }

            valueToStore = QString("asset/%1").arg(destFileName);
            }
        }
    } else {
        valueToStore = filePath;
    }

    valueItem->setText(QDir::fromNativeSeparators(valueToStore));

    m_hasUnsavedChanges = true;
    updatePreview();
}

void BankEditorWidget::onImageTableChanged(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (m_isLoading) {
        return;
    }
    m_hasUnsavedChanges = true;
    updatePreview();
}

void BankEditorWidget::loadQuestionToEditor(int index)
{
    if (index < 0 || index >= m_questions.size()) {
        clearEditor();
        return;
    }
    
    m_isLoading = true;
    m_currentQuestionIndex = index;
    
    const Question &question = m_questions[index];
    
    // Set question type
    QString typeData;
    switch (question.getType()) {
        case QuestionType::Choice:
            typeData = "Choice";
            break;
        case QuestionType::TrueOrFalse:
            typeData = "TrueOrFalse";
            break;
        case QuestionType::MultipleChoice:
            typeData = "MultipleChoice";
            break;
        case QuestionType::FillBlank:
            typeData = "FillBlank";
            break;
    }
    
    int typeIndex = m_typeComboBox->findData(typeData);
    if (typeIndex >= 0) {
        m_typeComboBox->setCurrentIndex(typeIndex);
    }
    
    // Set question content
    m_questionTextEdit->setPlainText(question.getQuestion());
    
    // Populate type-specific fields
    populateEditorFromQuestion(question);

    setImagesToTable(question.getImages());
    
    m_isLoading = false;
    
    updatePreview();
}

void BankEditorWidget::saveCurrentQuestion()
{
    if (m_currentQuestionIndex < 0 || m_currentQuestionIndex >= m_questions.size()) {
        return;
    }
    
    Question question = createQuestionFromEditor();
    m_questions[m_currentQuestionIndex] = question;
    
    // Update question list item text
    QListWidgetItem *item = m_questionListWidget->item(m_currentQuestionIndex);
    if (item) {
        QString questionText = question.getQuestion();
        if (questionText.length() > 30) {
            questionText = questionText.left(30) + "...";
        }
        item->setText(QString("%1. %2").arg(m_currentQuestionIndex + 1).arg(questionText));
    }
}

void BankEditorWidget::updateQuestionList()
{
    m_questionListWidget->clear();
    
    for (int i = 0; i < m_questions.size(); ++i) {
        const Question &question = m_questions[i];
        QString questionText = question.getQuestion();
        if (questionText.length() > 30) {
            questionText = questionText.left(30) + "...";
        }
        
        QString typeText;
        switch (question.getType()) {
            case QuestionType::Choice:
                typeText = "[选择]";
                break;
            case QuestionType::TrueOrFalse:
                typeText = "[判断]";
                break;
            case QuestionType::MultipleChoice:
                typeText = "[多选]";
                break;
            case QuestionType::FillBlank:
                typeText = "[填空]";
                break;
        }
        
        QListWidgetItem *item = new QListWidgetItem(QString("%1. %2 %3").arg(i + 1).arg(typeText).arg(questionText));
        const int state = (i >= 0 && i < m_questionEvalStates.size()) ? m_questionEvalStates[i] : 0;
        if (state == 1) {
            item->setBackground(QColor("#eefaf2"));
        } else if (state == 2) {
            item->setBackground(QColor("#fdf1f2"));
        } else if (state == 3) {
            item->setBackground(QColor("#fff8e6"));
        }
        if (i >= 0 && i < m_questionEvalLabels.size() && !m_questionEvalLabels[i].trimmed().isEmpty()) {
            item->setToolTip(m_questionEvalLabels[i].trimmed());
        }
        m_questionListWidget->addItem(item);
    }
}

void BankEditorWidget::updatePreview()
{
    // Update question text preview
    QString questionText = m_questionTextEdit->toPlainText();
    const QMap<QString, QString> images = getImagesFromTable();
    const QString imageBaseDir = getCurrentImageBaseDir();
    m_previewRenderer->setContent(questionText, images, imageBaseDir);
    
    // Update type-specific preview
    QString typeData = m_typeComboBox->currentData().toString();
    
    if (typeData == "Choice") {
        int choiceCount = m_choiceCountSpinBox->value();
        for (int i = 0; i < m_choicePreviewRenderers.size(); ++i) {
            if (i < choiceCount) {
                QString choiceText = m_choiceEdits[i]->toPlainText();
                if (choiceText.isEmpty()) {
                    choiceText = QString("%1. 选项%1").arg(QChar('A' + i));
                }
                m_choicePreviewRenderers[i]->setContent(QString("○ %1").arg(choiceText), images, imageBaseDir);
                m_choicePreviewRenderers[i]->setVisible(true);
            } else {
                m_choicePreviewRenderers[i]->setVisible(false);
            }
        }
    } else if (typeData == "MultipleChoice") {
        int choiceCount = m_multiChoiceCountSpinBox->value();
        for (int i = 0; i < m_multiChoicePreviewRenderers.size(); ++i) {
            if (i < choiceCount) {
                QString choiceText = m_multiChoiceEdits[i]->toPlainText();
                if (choiceText.isEmpty()) {
                    choiceText = QString("%1. 选项%1").arg(QChar('A' + i));
                }
                m_multiChoicePreviewRenderers[i]->setContent(QString("☐ %1").arg(choiceText), images, imageBaseDir);
                m_multiChoicePreviewRenderers[i]->setVisible(true);
            } else {
                m_multiChoicePreviewRenderers[i]->setVisible(false);
            }
        }
    } else if (typeData == "FillBlank") {
        int blankCount = m_blankCountSpinBox->value();
        QString previewText = QString("填空题包含 %1 个空格").arg(blankCount);
        for (int i = 0; i < blankCount; ++i) {
            previewText += QString("\n第 %1 空: ________").arg(i + 1);
        }
        m_fillBlankPreviewLabel->setText(previewText);
    }
}

void BankEditorWidget::clearEditor()
{
    m_isLoading = true;
    
    m_typeComboBox->setCurrentIndex(0);
    m_questionTextEdit->clear();
    
    // Clear all choice edits
    for (QTextEdit *edit : m_choiceEdits) {
        edit->clear();
    }
    for (QTextEdit *edit : m_multiChoiceEdits) {
        edit->clear();
    }
    for (QLineEdit *edit : m_blankAnswerEdits) {
        edit->clear();
    }
    
    m_choiceAnswerComboBox->setCurrentIndex(0);
    m_trueOrFalseAnswerComboBox->setCurrentIndex(0);
    m_multiChoiceAnswerEdit->clear();

    m_imageTable->setRowCount(0);
    
    m_isLoading = false;
    
    updatePreview();
}

bool BankEditorWidget::validateCurrentQuestion()
{
    QString questionText = m_questionTextEdit->toPlainText().trimmed();
    if (questionText.isEmpty()) {
        QMessageBox::warning(this, "验证失败", "题目内容不能为空。");
        return false;
    }
    
    QString typeData = m_typeComboBox->currentData().toString();
    
    if (typeData == "Choice" || typeData == "MultipleChoice") {
        QVector<QTextEdit*> *edits = (typeData == "Choice") ? &m_choiceEdits : &m_multiChoiceEdits;
        QSpinBox *countSpinBox = (typeData == "Choice") ? m_choiceCountSpinBox : m_multiChoiceCountSpinBox;
        
        int choiceCount = countSpinBox->value();
        for (int i = 0; i < choiceCount; ++i) {
            if ((*edits)[i]->toPlainText().trimmed().isEmpty()) {
                QMessageBox::warning(this, "验证失败", QString("选项 %1 不能为空。").arg(QChar('A' + i)));
                return false;
            }
        }
        
        if (typeData == "MultipleChoice") {
            QString answer = m_multiChoiceAnswerEdit->toPlainText().trimmed();
            if (answer.isEmpty()) {
                QMessageBox::warning(this, "验证失败", "多选题答案不能为空。");
                return false;
            }
            
            // 验证答案格式，支持逗号分隔或连续字符串格式
            QStringList validOptions;
            int choiceCount = m_multiChoiceCountSpinBox->value();
            for (int i = 0; i < choiceCount; ++i) {
                validOptions.append(QString(QChar('A' + i)));
            }
            
            QStringList answerOptions;
            if (answer.contains(',')) {
                // 逗号分隔格式
                answerOptions = answer.split(',', Qt::SkipEmptyParts);
                for (QString &opt : answerOptions) {
                    opt = opt.trimmed().toUpper();
                }
            } else {
                // 连续字符串格式
                for (int i = 0; i < answer.length(); ++i) {
                    answerOptions.append(QString(answer[i].toUpper()));
                }
            }
            
            // 检查答案选项是否有效
            for (const QString &opt : answerOptions) {
                if (!validOptions.contains(opt)) {
                    QMessageBox::warning(this, "验证失败", QString("无效的答案选项: %1").arg(opt));
                    return false;
                }
            }
        }
    } else if (typeData == "FillBlank") {
        int blankCount = m_blankCountSpinBox->value();
        for (int i = 0; i < blankCount; ++i) {
            if (m_blankAnswerEdits[i]->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "验证失败", QString("第 %1 个空的答案不能为空。").arg(i + 1));
                return false;
            }
        }
    }
    
    return true;
}

Question BankEditorWidget::createQuestionFromEditor()
{
    Question question;
    
    // Set type
    QString typeData = m_typeComboBox->currentData().toString();
    if (typeData == "Choice") {
        question.setType(QuestionType::Choice);
    } else if (typeData == "TrueOrFalse") {
        question.setType(QuestionType::TrueOrFalse);
    } else if (typeData == "MultipleChoice") {
        question.setType(QuestionType::MultipleChoice);
    } else if (typeData == "FillBlank") {
        question.setType(QuestionType::FillBlank);
    }
    
    // Set question text
    question.setQuestion(m_questionTextEdit->toPlainText());
    
    // Set type-specific data
    if (typeData == "Choice") {
        QStringList choices;
        int choiceCount = m_choiceCountSpinBox->value();
        for (int i = 0; i < choiceCount; ++i) {
            choices.append(m_choiceEdits[i]->toPlainText());
        }
        question.setChoices(choices);
        question.setSingleAnswer(m_choiceAnswerComboBox->currentText());
    } else if (typeData == "TrueOrFalse") {
        question.setSingleAnswer(m_trueOrFalseAnswerComboBox->currentData().toString());
    } else if (typeData == "MultipleChoice") {
        QStringList choices;
        int choiceCount = m_multiChoiceCountSpinBox->value();
        for (int i = 0; i < choiceCount; ++i) {
            choices.append(m_multiChoiceEdits[i]->toPlainText());
        }
        question.setChoices(choices);
        // 处理多选题答案格式，确保以逗号分隔
        QString rawAnswer = m_multiChoiceAnswerEdit->toPlainText().trimmed();
        QString formattedAnswer = rawAnswer;
        
        // 如果答案不包含逗号，则认为是连续字符串格式，需要转换为逗号分隔格式
        if (!rawAnswer.contains(',') && rawAnswer.length() > 1) {
            QStringList answerList;
            for (int i = 0; i < rawAnswer.length(); ++i) {
                answerList.append(QString(rawAnswer[i]));
            }
            formattedAnswer = answerList.join(",");
        }
        
        question.setSingleAnswer(formattedAnswer);
    } else if (typeData == "FillBlank") {
        int blankCount = m_blankCountSpinBox->value();
        question.setBlankNum(blankCount);
        QStringList answers;
        for (int i = 0; i < blankCount; ++i) {
            answers.append(m_blankAnswerEdits[i]->text());
        }
        question.setAnswers(answers);
    }

    question.setImages(getImagesFromTable());
    
    return question;
}

void BankEditorWidget::populateEditorFromQuestion(const Question &question)
{
    QuestionType type = question.getType();
    
    if (type == QuestionType::Choice) {
        QStringList choices = question.getChoices();
        m_choiceCountSpinBox->setValue(choices.size());
        
        for (int i = 0; i < choices.size() && i < m_choiceEdits.size(); ++i) {
            m_choiceEdits[i]->setPlainText(choices[i]);
            adjustOptionEditHeight(m_choiceEdits[i]);
        }
        
        QString answer = question.getSingleAnswer();
        int answerIndex = m_choiceAnswerComboBox->findText(answer);
        if (answerIndex >= 0) {
            m_choiceAnswerComboBox->setCurrentIndex(answerIndex);
        }
    } else if (type == QuestionType::TrueOrFalse) {
        QString answer = question.getSingleAnswer();
        int answerIndex = m_trueOrFalseAnswerComboBox->findData(answer);
        if (answerIndex >= 0) {
            m_trueOrFalseAnswerComboBox->setCurrentIndex(answerIndex);
        }
    } else if (type == QuestionType::MultipleChoice) {
        QStringList choices = question.getChoices();
        m_multiChoiceCountSpinBox->setValue(choices.size());
        
        for (int i = 0; i < choices.size() && i < m_multiChoiceEdits.size(); ++i) {
            m_multiChoiceEdits[i]->setPlainText(choices[i]);
            adjustOptionEditHeight(m_multiChoiceEdits[i]);
        }
        
        m_multiChoiceAnswerEdit->setPlainText(question.getSingleAnswer());
    } else if (type == QuestionType::FillBlank) {
        int blankCount = question.getBlankNum();
        m_blankCountSpinBox->setValue(blankCount);
        
        QStringList answers = question.getAnswers();
        for (int i = 0; i < answers.size() && i < m_blankAnswerEdits.size(); ++i) {
            m_blankAnswerEdits[i]->setText(answers[i]);
        }
    }
}

void BankEditorWidget::adjustOptionEditHeight(QTextEdit *edit)
{
    if (!edit) {
        return;
    }

    edit->document()->setTextWidth(edit->viewport()->width());

    const int minHeight = 36;
    int optionCount = 0;
    QVector<QTextEdit*> *optionEdits = nullptr;
    if (m_choiceEdits.contains(edit)) {
        optionEdits = &m_choiceEdits;
        optionCount = m_choiceCountSpinBox ? m_choiceCountSpinBox->value() : 0;
    } else if (m_multiChoiceEdits.contains(edit)) {
        optionEdits = &m_multiChoiceEdits;
        optionCount = m_multiChoiceCountSpinBox ? m_multiChoiceCountSpinBox->value() : 0;
    }

    optionCount = qMax(optionCount, 1);

    int maxHeight = 180;
    if (QWidget *container = edit->parentWidget()) {
        QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());

        int top = 0, left = 0, bottom = 0, right = 0;
        if (layout) {
            layout->getContentsMargins(&left, &top, &right, &bottom);
        }

        const int containerHeight = container->height();
        int fixedHeight = top + bottom;
        int visibleCount = 0;

        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                QLayoutItem *item = layout->itemAt(i);
                if (!item) {
                    continue;
                }

                if (QWidget *w = item->widget()) {
                    if (!w->isVisible()) {
                        continue;
                    }
                    ++visibleCount;

                    const QTextEdit *asOption = qobject_cast<QTextEdit*>(w);
                    if (asOption && optionEdits && optionEdits->contains(const_cast<QTextEdit*>(asOption))) {
                        continue;
                    }
                    fixedHeight += w->sizeHint().height();
                } else if (QSpacerItem *sp = item->spacerItem()) {
                    const QSize hint = sp->sizeHint();
                    if (hint.height() > 0) {
                        fixedHeight += hint.height();
                        ++visibleCount;
                    }
                }
            }
        }

        const int spacing = layout ? layout->spacing() : 0;
        const int spacingReserve = spacing * qMax(visibleCount - 1, 0);

        int availableForOptions = containerHeight - fixedHeight - spacingReserve;
        availableForOptions = qMax(availableForOptions, minHeight * optionCount);

        maxHeight = availableForOptions / optionCount;
        maxHeight = qMax(maxHeight, minHeight);
        maxHeight = qMin(maxHeight, qMax(180, containerHeight / 2));
    }
    const int docHeight = qCeil(edit->document()->size().height());

    const int extra = edit->frameWidth() * 2 + qCeil(edit->document()->documentMargin() * 2) + 8;
    const int target = qBound(minHeight, docHeight + extra, maxHeight);

    edit->setMinimumHeight(target);
    edit->setMaximumHeight(maxHeight);
    edit->updateGeometry();
}

QMap<QString, QString> BankEditorWidget::getImagesFromTable() const
{
    QMap<QString, QString> images;
    const int rows = m_imageTable->rowCount();
    for (int row = 0; row < rows; ++row) {
        QTableWidgetItem *keyItem = m_imageTable->item(row, 0);
        QTableWidgetItem *valueItem = m_imageTable->item(row, 1);
        const QString key = keyItem ? keyItem->text().trimmed() : QString();
        const QString value = valueItem ? valueItem->text().trimmed() : QString();
        if (!key.isEmpty() && !value.isEmpty()) {
            images.insert(key, value);
        }
    }
    return images;
}

void BankEditorWidget::setImagesToTable(const QMap<QString, QString> &images)
{
    QSignalBlocker blocker(m_imageTable);
    m_imageTable->setRowCount(0);

    int row = 0;
    for (auto it = images.begin(); it != images.end(); ++it) {
        m_imageTable->insertRow(row);
        m_imageTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_imageTable->setItem(row, 1, new QTableWidgetItem(it.value()));
        ++row;
    }
}

QString BankEditorWidget::getCurrentImageBaseDir() const
{
    if (m_currentSubject.trimmed().isEmpty()) {
        return QString();
    }

    QString typeData = m_typeComboBox->currentData().toString();
    QString typeDir;
    if (typeData == "Choice") {
        typeDir = "Choice";
    } else if (typeData == "TrueOrFalse") {
        typeDir = "TrueorFalse";
    } else if (typeData == "FillBlank") {
        typeDir = "FillBlank";
    } else if (typeData == "MultipleChoice") {
        typeDir = "MultiChoice";
    } else {
        typeDir = "Choice";
    }

    ConfigManager configManager;
    const QString subjectPath = configManager.getSubjectPath(m_currentSubject);
    if (!subjectPath.isEmpty()) {
        return QDir(subjectPath).filePath(typeDir);
    }
    return QDir(QApplication::applicationDirPath()).filePath("Subject/" + m_currentSubject + "/" + typeDir);
}

QString BankEditorWidget::escapeJsonString(const QString &str)
{
    QString escaped = str;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    escaped.replace("\t", "\\t");
    return escaped;
}

QString BankEditorWidget::unescapeJsonString(const QString &str)
{
    QString unescaped = str;
    unescaped.replace("\\n", "\n");
    unescaped.replace("\\r", "\r");
    unescaped.replace("\\t", "\t");
    unescaped.replace("\\\"", "\"");
    unescaped.replace("\\\\", "\\");
    return unescaped;
}
