#include "reviewwidget.h"
#include "../core/configmanager.h"
#include "../core/practicemanager.h"
#include "../models/question.h"
#include "../utils/jsonutils.h"
#include "../utils/markdownrenderer.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QFrame>
#include <QScrollArea>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QTimer>
#include <QShowEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QStringConverter>
#include <QPixmap>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>



ReviewWidget::ReviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_configManager(nullptr)
    , m_practiceManager(nullptr)
    , m_wrongAnswerSet(nullptr)
    , m_currentRecordIndex(-1)
{
    setupUI();
    setupConnections();
    applyStyles();
}

ReviewWidget::~ReviewWidget()
{
}

void ReviewWidget::setConfigManager(ConfigManager *configManager)
{
    m_configManager = configManager;
    updateFilterOptions();
}

void ReviewWidget::setPracticeManager(PracticeManager *practiceManager)
{
    m_practiceManager = practiceManager;
    
    // 连接错题导入请求信号
    if (m_practiceManager) {
        connect(m_practiceManager, &PracticeManager::wrongAnswersImportRequested,
                this, &ReviewWidget::onWrongAnswersImportRequested);
    }
}

void ReviewWidget::loadWrongAnswers()
{
    if (m_wrongAnswerSet) {
        // 使用新的错题集合系统
        m_wrongAnswerSet->loadFromFile();
        updateFromWrongAnswerSet();
    }
    
    updateWrongAnswersList();
    updateStatistics();
}



void ReviewWidget::refreshWrongAnswers()
{
    loadWrongAnswers();
}

void ReviewWidget::setupUI()
{
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Create left panel
    m_leftPanel = new QWidget();
    m_leftPanel->setMaximumWidth(450);  // 限制左侧面板最大宽度
    m_leftPanel->setMinimumWidth(350);  // 设置左侧面板最小宽度
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(10, 10, 10, 10);
    m_leftLayout->setSpacing(10);
    
    // Filter group
    m_filterGroup = new QGroupBox("筛选条件");
    m_filterGroup->setMaximumHeight(200);  // 限制筛选组高度
    m_filterLayout = new QGridLayout(m_filterGroup);
    m_filterLayout->setSpacing(6);
    m_filterLayout->setContentsMargins(8, 8, 8, 8);
    
    m_subjectLabel = new QLabel("科目:");
    m_subjectCombo = new QComboBox();
    m_subjectCombo->addItem("全部科目", "");
    m_subjectCombo->setMaximumHeight(28);
    
    m_typeLabel = new QLabel("题型:");
    m_typeCombo = new QComboBox();
    m_typeCombo->addItem("全部题型", "");
    m_typeCombo->addItem("选择题", "Choice");
    m_typeCombo->addItem("判断题", "TrueOrFalse");
    m_typeCombo->addItem("填空题", "FillBlank");
    m_typeCombo->addItem("多选题", "MultiChoice");
    m_typeCombo->setMaximumHeight(28);
    
    m_dateLabel = new QLabel("日期:");
    m_dateFromEdit = new QDateEdit();
    m_dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    m_dateFromEdit->setCalendarPopup(true);
    m_dateFromEdit->setMaximumHeight(28);
    m_dateFromEdit->setDisplayFormat("MM-dd");
    m_dateToEdit = new QDateEdit();
    m_dateToEdit->setDate(QDate::currentDate());
    m_dateToEdit->setCalendarPopup(true);
    m_dateToEdit->setMaximumHeight(28);
    m_dateToEdit->setDisplayFormat("MM-dd");
    
    m_showResolvedCheck = new QCheckBox("显示已解决");
    m_showResolvedCheck->setChecked(false);
    
    m_sortLabel = new QLabel("排序:");
    m_sortCombo = new QComboBox();
    m_sortCombo->addItem("时间降序", "time_desc");
    m_sortCombo->addItem("时间升序", "time_asc");
    m_sortCombo->addItem("复习次数降序", "review_desc");
    m_sortCombo->addItem("复习次数升序", "review_asc");
    m_sortCombo->setMaximumHeight(28);
    
    m_refreshButton = new QPushButton("刷新");
    m_refreshButton->setObjectName("refreshButton");
    m_refreshButton->setMaximumHeight(28);
    
    // 使用更紧凑的布局
    m_filterLayout->addWidget(m_subjectLabel, 0, 0);
    m_filterLayout->addWidget(m_subjectCombo, 0, 1);
    m_filterLayout->addWidget(m_typeLabel, 0, 2);
    m_filterLayout->addWidget(m_typeCombo, 0, 3);
    
    m_filterLayout->addWidget(m_dateLabel, 1, 0);
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(m_dateFromEdit);
    dateLayout->addWidget(new QLabel("至"));
    dateLayout->addWidget(m_dateToEdit);
    dateLayout->setSpacing(4);
    QWidget *dateWidget = new QWidget();
    dateWidget->setLayout(dateLayout);
    m_filterLayout->addWidget(dateWidget, 1, 1, 1, 3);
    
    m_filterLayout->addWidget(m_sortLabel, 2, 0);
    m_filterLayout->addWidget(m_sortCombo, 2, 1);
    m_filterLayout->addWidget(m_showResolvedCheck, 2, 2);
    m_filterLayout->addWidget(m_refreshButton, 2, 3);
    
    // Statistics group
    m_statisticsGroup = new QGroupBox("统计信息");
    m_statisticsGroup->setMaximumHeight(120);  // 限制统计组高度
    m_statisticsLayout = new QGridLayout(m_statisticsGroup);
    m_statisticsLayout->setSpacing(6);
    m_statisticsLayout->setContentsMargins(8, 8, 8, 8);
    
    m_totalLabel = new QLabel("总计: 0");
    m_totalLabel->setStyleSheet("font-weight: bold; color: #2c3e50;");
    m_unresolvedLabel = new QLabel("未解决: 0");
    m_unresolvedLabel->setStyleSheet("color: #e74c3c;");
    m_resolvedLabel = new QLabel("已解决: 0");
    m_resolvedLabel->setStyleSheet("color: #27ae60;");
    m_selectedLabel = new QLabel("已选择: 0");
    m_selectedLabel->setStyleSheet("color: #3498db;");
    m_resolvedProgressBar = new QProgressBar();
    m_resolvedProgressBar->setTextVisible(true);
    m_resolvedProgressBar->setFormat("解决率: %p%");
    m_resolvedProgressBar->setMaximumHeight(20);
    
    m_statisticsLayout->addWidget(m_totalLabel, 0, 0);
    m_statisticsLayout->addWidget(m_unresolvedLabel, 0, 1);
    m_statisticsLayout->addWidget(m_resolvedLabel, 1, 0);
    m_statisticsLayout->addWidget(m_selectedLabel, 1, 1);
    m_statisticsLayout->addWidget(m_resolvedProgressBar, 2, 0, 1, 2);
    
    // Wrong answers list group
    m_listGroup = new QGroupBox("错题列表");
    m_listLayout = new QVBoxLayout(m_listGroup);
    
    m_wrongAnswersList = new QListWidget();
    m_wrongAnswersList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    m_listLayout->addWidget(m_wrongAnswersList);
    
    // Selection buttons
    m_selectionLayout = new QHBoxLayout();
    m_selectAllButton = new QPushButton("全选");
    m_selectAllButton->setMaximumHeight(28);
    m_selectNoneButton = new QPushButton("全不选");
    m_selectNoneButton->setMaximumHeight(28);
    
    m_selectionLayout->addWidget(m_selectAllButton);
    m_selectionLayout->addWidget(m_selectNoneButton);
    m_selectionLayout->addStretch();
    
    // Action buttons - 分成两行显示
    m_actionLayout = new QGridLayout();
    m_actionLayout->setSpacing(4);
    
    m_startReviewButton = new QPushButton("开始复习");
    m_startReviewButton->setObjectName("startReviewButton");
    m_startReviewButton->setMaximumHeight(28);
    
    m_markResolvedButton = new QPushButton("标记已解决");
    m_markResolvedButton->setMaximumHeight(28);
    
    m_markUnresolvedButton = new QPushButton("标记未解决");
    m_markUnresolvedButton->setMaximumHeight(28);
    
    m_deleteSelectedButton = new QPushButton("删除选中");
    m_deleteSelectedButton->setObjectName("deleteButton");
    m_deleteSelectedButton->setMaximumHeight(28);
    
    m_clearResolvedButton = new QPushButton("清除已解决");
    m_clearResolvedButton->setObjectName("clearButton");
    m_clearResolvedButton->setMaximumHeight(28);
    
    // 第一行：主要操作
    m_actionLayout->addWidget(m_startReviewButton, 0, 0);
    m_actionLayout->addWidget(m_markResolvedButton, 0, 1);
    m_actionLayout->addWidget(m_markUnresolvedButton, 0, 2);
    
    // 第二行：删除操作
    m_actionLayout->addWidget(m_deleteSelectedButton, 1, 0);
    m_actionLayout->addWidget(m_clearResolvedButton, 1, 1);
    
    // Import/Export buttons
    m_importExportLayout = new QHBoxLayout();
    m_exportButton = new QPushButton("导出错题");
    m_exportButton->setMaximumHeight(28);
    m_importButton = new QPushButton("导入错题");
    m_importButton->setMaximumHeight(28);
    
    m_importExportLayout->addWidget(m_exportButton);
    m_importExportLayout->addWidget(m_importButton);
    m_importExportLayout->addStretch();
    
    // Control buttons
    m_controlLayout = new QHBoxLayout();
    m_backButton = new QPushButton("返回");
    m_backButton->setObjectName("backButton");
    m_backButton->setMaximumHeight(28);
    
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_backButton);
    
    // Add to left layout
    m_leftLayout->addWidget(m_filterGroup);
    m_leftLayout->addWidget(m_statisticsGroup);
    m_leftLayout->addWidget(m_listGroup, 1);
    m_leftLayout->addLayout(m_selectionLayout);
    m_leftLayout->addLayout(m_actionLayout);
    m_leftLayout->addLayout(m_importExportLayout);
    m_leftLayout->addLayout(m_controlLayout);
    
    // Create right panel
    m_rightPanel = new QWidget();
    m_rightPanel->setMinimumWidth(400);  // 设置右侧面板最小宽度
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(10, 10, 10, 10);
    m_rightLayout->setSpacing(10);
    
    // Details group
    m_detailsGroup = new QGroupBox("错题详情");
    m_detailsLayout = new QVBoxLayout(m_detailsGroup);
    
    m_detailsScrollArea = new QScrollArea();
    m_detailsScrollArea->setWidgetResizable(true);
    m_detailsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_detailsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_detailsContent = new QWidget();
    m_detailsContentLayout = new QVBoxLayout(m_detailsContent);
    m_detailsContentLayout->setContentsMargins(10, 10, 10, 10);
    m_detailsContentLayout->setSpacing(10);
    
    // Detail labels
    m_detailSubjectLabel = new QLabel();
    m_detailSubjectLabel->setObjectName("detailSubject");
    m_detailTypeLabel = new QLabel();
    m_detailTypeLabel->setObjectName("detailType");
    m_detailQuestionRenderer = new MarkdownRenderer();
    m_detailQuestionRenderer->setObjectName("detailQuestion");
    m_detailQuestionRenderer->setAutoResize(true, 400);  // 启用自动适配
    m_detailImageLabel = new QLabel();
    m_detailImageLabel->setAlignment(Qt::AlignCenter);
    m_detailImageLabel->setScaledContents(false);
    m_detailImageLabel->setVisible(false);
    m_detailChoicesRenderer = new MarkdownRenderer();
    m_detailChoicesRenderer->setObjectName("detailChoices");
    m_detailChoicesRenderer->setAutoResize(true, 300);  // 启用自动适配
    m_detailCorrectAnswerRenderer = new MarkdownRenderer();
    m_detailCorrectAnswerRenderer->setObjectName("detailCorrectAnswer");
    m_detailCorrectAnswerRenderer->setAutoResize(true, 200);  // 启用自动适配
    m_detailUserAnswerRenderer = new MarkdownRenderer();
    m_detailUserAnswerRenderer->setObjectName("detailUserAnswer");
    m_detailUserAnswerRenderer->setAutoResize(true, 200);  // 启用自动适配
    m_detailTimestampLabel = new QLabel();
    m_detailTimestampLabel->setObjectName("detailTimestamp");
    m_detailReviewCountLabel = new QLabel();
    m_detailReviewCountLabel->setObjectName("detailReviewCount");
    m_detailStatusLabel = new QLabel();
    m_detailStatusLabel->setObjectName("detailStatus");
    
    m_detailsContentLayout->addWidget(m_detailSubjectLabel);
    m_detailsContentLayout->addWidget(m_detailTypeLabel);
    m_detailsContentLayout->addWidget(m_detailQuestionRenderer);
    m_detailsContentLayout->addWidget(m_detailImageLabel);
    m_detailsContentLayout->addWidget(m_detailChoicesRenderer);
    m_detailsContentLayout->addWidget(m_detailCorrectAnswerRenderer);
    m_detailsContentLayout->addWidget(m_detailUserAnswerRenderer);
    m_detailsContentLayout->addWidget(m_detailTimestampLabel);
    m_detailsContentLayout->addWidget(m_detailReviewCountLabel);
    m_detailsContentLayout->addWidget(m_detailStatusLabel);
    m_detailsContentLayout->addStretch();
    
    m_detailsScrollArea->setWidget(m_detailsContent);
    m_detailsLayout->addWidget(m_detailsScrollArea);
    
    m_rightLayout->addWidget(m_detailsGroup);
    
    // Add panels to splitter
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    
    // 设置初始大小比例：左侧400px，右侧占剩余空间
    m_mainSplitter->setSizes({400, 600});
    m_mainSplitter->setStretchFactor(0, 0);  // 左侧面板不拉伸
    m_mainSplitter->setStretchFactor(1, 1);  // 右侧面板可拉伸
    
    // 设置分割器样式
    m_mainSplitter->setHandleWidth(3);
    m_mainSplitter->setStyleSheet(
        "QSplitter::handle {"
        "    background-color: #dee2e6;"
        "    border: 1px solid #adb5bd;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #4A90E2;"
        "}");
    
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_mainSplitter);
    
    setLayout(mainLayout);
    
    // Initialize details
    clearWrongAnswerDetails();
}

void ReviewWidget::setupConnections()
{
    connect(m_subjectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReviewWidget::onSubjectFilterChanged);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReviewWidget::onTypeFilterChanged);
    connect(m_dateFromEdit, &QDateEdit::dateChanged, this, &ReviewWidget::onDateFilterChanged);
    connect(m_dateToEdit, &QDateEdit::dateChanged, this, &ReviewWidget::onDateFilterChanged);
    connect(m_showResolvedCheck, &QCheckBox::toggled, this, &ReviewWidget::onResolvedFilterChanged);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReviewWidget::onSortOrderChanged);
    
    connect(m_wrongAnswersList, &QListWidget::itemClicked,
            this, &ReviewWidget::onWrongAnswerSelected);
    connect(m_wrongAnswersList, &QListWidget::itemSelectionChanged,
            this, &ReviewWidget::updateSelectionButtons);
    
    connect(m_refreshButton, &QPushButton::clicked, this, &ReviewWidget::onRefreshClicked);
    connect(m_startReviewButton, &QPushButton::clicked, this, &ReviewWidget::onStartReviewClicked);
    connect(m_markResolvedButton, &QPushButton::clicked, this, &ReviewWidget::onMarkResolvedClicked);
    connect(m_markUnresolvedButton, &QPushButton::clicked, this, &ReviewWidget::onMarkUnresolvedClicked);
    connect(m_deleteSelectedButton, &QPushButton::clicked, this, &ReviewWidget::onDeleteSelectedClicked);
    connect(m_clearResolvedButton, &QPushButton::clicked, this, &ReviewWidget::onClearResolvedClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &ReviewWidget::onExportClicked);
    connect(m_importButton, &QPushButton::clicked, this, &ReviewWidget::onImportClicked);
    connect(m_selectAllButton, &QPushButton::clicked, this, &ReviewWidget::onSelectAllClicked);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &ReviewWidget::onSelectNoneClicked);
    connect(m_backButton, &QPushButton::clicked, this, &ReviewWidget::onBackClicked);
}

void ReviewWidget::applyStyles()
{
    setStyleSheet(
        "ReviewWidget {"
        "    background-color: #f8f9fa;"
        "}"
        
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 6px;"
        "    margin-top: 8px;"
        "    padding-top: 8px;"
        "    background-color: white;"
        "    font-size: 13px;"
        "}"
        
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 8px 0 8px;"
        "    color: #495057;"
        "}"
        
        "QComboBox, QDateEdit {"
        "    padding: 4px 6px;"
        "    border: 1px solid #ced4da;"
        "    border-radius: 3px;"
        "    background-color: white;"
        "    font-size: 12px;"
        "    min-height: 20px;"
        "}"
        
        "QComboBox:focus, QDateEdit:focus {"
        "    border-color: #4A90E2;"
        "    outline: none;"
        "}"
        
        "QCheckBox {"
        "    font-size: 12px;"
        "    spacing: 6px;"
        "}"
        
        "QListWidget {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    alternate-background-color: #f8f9fa;"
        "    font-size: 12px;"
        "    selection-background-color: #4A90E2;"
        "}"
        
        "QListWidget::item {"
        "    padding: 6px 8px;"
        "    border-bottom: 1px solid #e9ecef;"
        "    min-height: 20px;"
        "}"
        
        "QListWidget::item:selected {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "}"
        
        "QListWidget::item:hover {"
        "    background-color: #e3f2fd;"
        "}"
        
        "QPushButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 10px;"
        "    font-size: 12px;"
        "    min-width: 60px;"
        "}"
        
        "QPushButton:hover {"
        "    background-color: #5a6268;"
        "    transform: translateY(-1px);"
        "}"
        
        "QPushButton:pressed {"
        "    background-color: #545b62;"
        "    transform: translateY(1px);"
        "}"
        
        "#startReviewButton {"
        "    background-color: #28a745;"
        "}"
        
        "#startReviewButton:hover {"
        "    background-color: #218838;"
        "}"
        
        "#deleteButton, #clearButton {"
        "    background-color: #dc3545;"
        "}"
        
        "#deleteButton:hover, #clearButton:hover {"
        "    background-color: #c82333;"
        "}"
        
        "#refreshButton {"
        "    background-color: #17a2b8;"
        "}"
        
        "#refreshButton:hover {"
        "    background-color: #138496;"
        "}"
        
        "#backButton {"
        "    background-color: #6c757d;"
        "}"
        
        "#backButton:hover {"
        "    background-color: #5a6268;"
        "}"
        
        "QProgressBar {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "    background-color: #f8f9fa;"
        "    font-size: 11px;"
        "    height: 18px;"
        "}"
        
        "QProgressBar::chunk {"
        "    background-color: #28a745;"
        "    border-radius: 2px;"
        "}"
        
        "QScrollArea {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
        
        "#detailSubject {"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    background-color: #e3f2fd;"
        "    padding: 6px 8px;"
        "    border-radius: 4px;"
        "}"
        
        "#detailType {"
        "    font-size: 13px;"
        "    color: #6c757d;"
        "    background-color: #e9ecef;"
        "    padding: 3px 6px;"
        "    border-radius: 3px;"
        "}"
        
        "#detailQuestion {"
        "    font-size: 14px;"
        "    line-height: 1.4;"
        "    color: #2c3e50;"
        "    background-color: #f8f9fa;"
        "    padding: 8px 10px;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "}"
        
        "#detailChoices {"
        "    font-size: 13px;"
        "    color: #495057;"
        "    background-color: white;"
        "    padding: 6px 8px;"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "}"
        
        "#detailCorrectAnswer {"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    color: #28a745;"
        "    background-color: #d4edda;"
        "    padding: 6px 8px;"
        "    border: 1px solid #c3e6cb;"
        "    border-radius: 4px;"
        "}"
        
        "#detailUserAnswer {"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    color: #dc3545;"
        "    background-color: #f8d7da;"
        "    padding: 6px 8px;"
        "    border: 1px solid #f5c6cb;"
        "    border-radius: 4px;"
        "}"
        
        "#detailTimestamp, #detailReviewCount, #detailStatus {"
        "    font-size: 11px;"
        "    color: #6c757d;"
        "    background-color: #f8f9fa;"
        "    padding: 4px 6px;"
        "    border-radius: 3px;"
        "    margin: 1px 0;"
        "}"
    );
}

void ReviewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadWrongAnswers();
    updateFilterOptions();
}

void ReviewWidget::onSubjectFilterChanged()
{
    updateWrongAnswersList();
    updateStatistics();
}

void ReviewWidget::onTypeFilterChanged()
{
    updateWrongAnswersList();
    updateStatistics();
}

void ReviewWidget::onDateFilterChanged()
{
    updateWrongAnswersList();
    updateStatistics();
}

void ReviewWidget::onResolvedFilterChanged()
{
    updateWrongAnswersList();
    updateStatistics();
}

void ReviewWidget::onSortOrderChanged()
{
    updateWrongAnswersList();
}

void ReviewWidget::onWrongAnswerSelected(QListWidgetItem *item)
{
    if (!item) {
        clearWrongAnswerDetails();
        return;
    }
    
    int recordIndex = item->data(Qt::UserRole).toInt();
    m_currentRecordIndex = recordIndex;
    
    if (m_wrongAnswerSet && recordIndex >= 0 && recordIndex < m_filteredItems.size()) {
        displayWrongAnswerDetails(m_filteredItems[recordIndex]);
    }
}

void ReviewWidget::onStartReviewClicked()
{
    if (!m_wrongAnswerSet) {
        QMessageBox::warning(this, "错误", "错题系统未初始化");
        return;
    }
    
    // 获取选中的错题ID
    QStringList selectedIds = getSelectedIds();
    if (selectedIds.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要复习的错题");
        return;
    }
    
    // 将选中的错题转换为Question列表
    QList<Question> reviewQuestions;
    for (const QString &id : selectedIds) {
        WrongAnswerItem item = m_wrongAnswerSet->getWrongAnswer(id);
        if (!item.id.isEmpty()) {
            // 将WrongAnswerItem转换为Question对象
            Question question = convertWrongAnswerItemToQuestion(item);
            reviewQuestions.append(question);
        }
    }
    
    if (reviewQuestions.isEmpty()) {
        QMessageBox::warning(this, "错误", "无法加载选中的错题");
        return;
    }
    
    // 发射开始复习信号
    emit startReviewRequested(reviewQuestions);
}

void ReviewWidget::onClearResolvedClicked()
{
    if (!m_wrongAnswerSet) {
        QMessageBox::warning(this, "错误", "错题系统未初始化");
        return;
    }
    
    int resolvedCount = m_wrongAnswerSet->getResolvedCount();
    
    if (resolvedCount == 0) {
        QMessageBox::information(this, "提示", "没有已解决的错题");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认清除",
                                   QString("确定要清除 %1 道已解决的错题吗？\n此操作不可撤销。")
                                   .arg(resolvedCount),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_wrongAnswerSet->removeResolvedItems();
        m_wrongAnswerSet->saveToFile();
        
        updateWrongAnswersList();
        updateStatistics();
        clearWrongAnswerDetails();
        
        QMessageBox::information(this, "完成", QString("已清除 %1 道已解决的错题").arg(resolvedCount));
    }
}

void ReviewWidget::onExportClicked()
{
    exportWrongAnswers();
}

void ReviewWidget::onImportClicked()
{
    importWrongAnswers();
}

void ReviewWidget::onDeleteSelectedClicked()
{
    QList<QListWidgetItem*> selectedItems = m_wrongAnswersList->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要删除的错题");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除",
                                   QString("确定要删除选中的 %1 道错题吗？\n此操作不可撤销。")
                                   .arg(selectedItems.size()),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        QVector<int> indicesToDelete;
        for (QListWidgetItem *item : selectedItems) {
            int index = item->data(Qt::UserRole).toInt();
            indicesToDelete.append(index);
        }
        
        deleteRecords(indicesToDelete);
        
        updateWrongAnswersList();
        updateStatistics();
        clearWrongAnswerDetails();
        
        QMessageBox::information(this, "完成", QString("已删除 %1 道错题").arg(selectedItems.size()));
    }
}

void ReviewWidget::onMarkResolvedClicked()
{
    QList<QListWidgetItem*> selectedItems = m_wrongAnswersList->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要标记的错题");
        return;
    }
    
    QVector<int> indices;
    for (QListWidgetItem *item : selectedItems) {
        int index = item->data(Qt::UserRole).toInt();
        indices.append(index);
    }
    
    markRecordsAsResolved(indices);
    
    updateWrongAnswersList();
    updateStatistics();
    
    QMessageBox::information(this, "完成", QString("已标记 %1 道错题为已解决").arg(indices.size()));
}

void ReviewWidget::onMarkUnresolvedClicked()
{
    QList<QListWidgetItem*> selectedItems = m_wrongAnswersList->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择要标记的错题");
        return;
    }
    
    QVector<int> indices;
    for (QListWidgetItem *item : selectedItems) {
        int index = item->data(Qt::UserRole).toInt();
        indices.append(index);
    }
    
    markRecordsAsUnresolved(indices);
    
    updateWrongAnswersList();
    updateStatistics();
    
    QMessageBox::information(this, "完成", QString("已标记 %1 道错题为未解决").arg(indices.size()));
}

void ReviewWidget::onSelectAllClicked()
{
    m_wrongAnswersList->selectAll();
    updateSelectionButtons();
}

void ReviewWidget::onSelectNoneClicked()
{
    m_wrongAnswersList->clearSelection();
    updateSelectionButtons();
}

void ReviewWidget::onBackClicked()
{
    emit backRequested();
}

void ReviewWidget::onRefreshClicked()
{
    refreshWrongAnswers();
}

void ReviewWidget::updateWrongAnswersList()
{
    m_wrongAnswersList->clear();
    
    if (m_wrongAnswerSet) {
        // 使用新的错题集合系统
        m_filteredItems = getFilteredWrongAnswerItems();
        
        for (int i = 0; i < m_filteredItems.size(); ++i) {
            const WrongAnswerItem &item = m_filteredItems[i];
            
            QListWidgetItem *listItem = new QListWidgetItem();
            listItem->setData(Qt::UserRole, i);
            
            QString statusIcon = item.isResolved ? "✓" : "✗";
            QString typeStr = getQuestionTypeString(item.questionType);
            QString timeStr = formatDateTime(item.timestamp);
            
            QString displayText = QString("%1 [%2] %3 - %4 (复习%5次)")
                                 .arg(statusIcon)
                                 .arg(item.subject)
                                 .arg(typeStr)
                                 .arg(timeStr)
                                 .arg(item.reviewCount);
            
            listItem->setText(displayText);
            
            // Set background color based on status
            if (item.isResolved) {
                listItem->setBackground(QColor("#d4edda"));
            } else {
                listItem->setBackground(QColor("#f8d7da"));
            }
            
            m_wrongAnswersList->addItem(listItem);
        }
    }
    
    updateSelectionButtons();
}

void ReviewWidget::updateStatistics()
{
    int total = 0;
    int resolved = 0;
    int unresolved = 0;
    
    if (m_wrongAnswerSet) {
        // 使用新的错题集合系统
        const QVector<WrongAnswerItem> &items = m_wrongAnswerSet->getAllWrongAnswers();
        total = items.size();
        
        for (const WrongAnswerItem &item : items) {
            if (item.isResolved) {
                resolved++;
            } else {
                unresolved++;
            }
        }
    }
    
    int selected = m_wrongAnswersList->selectedItems().size();
    
    m_totalLabel->setText(QString("总计: %1").arg(total));
    m_unresolvedLabel->setText(QString("未解决: %1").arg(unresolved));
    m_resolvedLabel->setText(QString("已解决: %1").arg(resolved));
    m_selectedLabel->setText(QString("已选择: %1").arg(selected));
    
    int resolvedPercentage = total > 0 ? (resolved * 100 / total) : 0;
    m_resolvedProgressBar->setValue(resolvedPercentage);
}

void ReviewWidget::updateFilterOptions()
{
    if (!m_configManager) {
        return;
    }
    
    // Update subject combo
    m_subjectCombo->clear();
    m_subjectCombo->addItem("全部科目", "");
    
    QStringList subjects = m_configManager->getSubjects();
    for (const QString &subject : subjects) {
        m_subjectCombo->addItem(subject, subject);
    }
}

void ReviewWidget::updateSelectionButtons()
{
    int selectedCount = m_wrongAnswersList->selectedItems().size();
    int totalCount = m_wrongAnswersList->count();
    
    m_startReviewButton->setEnabled(selectedCount > 0);
    m_markResolvedButton->setEnabled(selectedCount > 0);
    m_markUnresolvedButton->setEnabled(selectedCount > 0);
    m_deleteSelectedButton->setEnabled(selectedCount > 0);
    
    m_selectAllButton->setEnabled(totalCount > 0 && selectedCount < totalCount);
    m_selectNoneButton->setEnabled(selectedCount > 0);
    
    // Update statistics
    m_selectedLabel->setText(QString("已选择: %1").arg(selectedCount));
}

void ReviewWidget::displayWrongAnswerDetails(const WrongAnswerItem &item)
{
    m_detailSubjectLabel->setText(QString("科目: %1").arg(item.subject));
    m_detailTypeLabel->setText(QString("题型: %1").arg(getQuestionTypeString(item.questionType)));
    
    QString imageBaseDir;
    if (m_configManager) {
        const QString subjectPath = m_configManager->getSubjectPath(item.subject);
        if (!subjectPath.isEmpty()) {
            imageBaseDir = QDir(subjectPath).filePath(item.questionType);
        } else {
            imageBaseDir = QDir(QApplication::applicationDirPath()).filePath("Subject/" + item.subject + "/" + item.questionType);
        }
    } else {
        imageBaseDir = QDir(QApplication::applicationDirPath()).filePath("Subject/" + item.subject + "/" + item.questionType);
    }

    m_detailQuestionRenderer->setContent(QString("**题目:**\n%1").arg(item.questionText), item.images, imageBaseDir);
    
    // Display image if exists
    QString displayImagePath;
    if (!item.images.isEmpty()) {
        if (item.images.contains("img1")) {
            displayImagePath = item.images.value("img1");
        } else {
            displayImagePath = item.images.constBegin().value();
        }
    }

    if (!displayImagePath.isEmpty()) {
        QString fullImagePath;
        
        // 检查是否为绝对路径
        if (QDir::isAbsolutePath(displayImagePath)) {
            fullImagePath = displayImagePath;
        } else if (!imageBaseDir.isEmpty()) {
            fullImagePath = QDir(imageBaseDir).filePath(displayImagePath);
        } else {
            fullImagePath = displayImagePath;
        }
        
        if (QFile::exists(fullImagePath)) {
            QPixmap pixmap(fullImagePath);
            if (!pixmap.isNull()) {
                QPixmap scaledPixmap = pixmap.scaled(300, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                m_detailImageLabel->setPixmap(scaledPixmap);
                m_detailImageLabel->setVisible(true);
            } else {
                qDebug() << "Failed to load image:" << fullImagePath;
                m_detailImageLabel->setVisible(false);
            }
        } else {
            qDebug() << "Image file does not exist:" << fullImagePath;
            m_detailImageLabel->setVisible(false);
        }
    } else {
        m_detailImageLabel->setVisible(false);
    }
    
    // Display choices if available
    if (!item.choices.isEmpty()) {
        QString choicesText = "选项:\n";
        QStringList labels = {"A", "B", "C", "D", "E", "F"};
        for (int i = 0; i < item.choices.size() && i < labels.size(); ++i) {
            choicesText += QString("%1. %2\n").arg(labels[i]).arg(item.choices[i]);
        }
        m_detailChoicesRenderer->setContent(choicesText.trimmed(), item.images, imageBaseDir);
        m_detailChoicesRenderer->setVisible(true);
    } else {
        m_detailChoicesRenderer->setVisible(false);
    }
    
    // Display correct answer
    QString correctAnswerText;
    if (!item.correctAnswers.isEmpty()) {
        correctAnswerText = QString("正确答案: %1").arg(item.correctAnswers.join(", "));
    } else {
        correctAnswerText = QString("正确答案: %1").arg(item.correctAnswer);
    }
    m_detailCorrectAnswerRenderer->setContent(correctAnswerText);
    
    // Display user answer
    QString userAnswerText;
    if (!item.userAnswers.isEmpty()) {
        userAnswerText = QString("你的答案: %1").arg(item.userAnswers.join(", "));
    } else {
        userAnswerText = QString("你的答案: %1").arg(item.userAnswer);
    }
    m_detailUserAnswerRenderer->setContent(userAnswerText);
    
    m_detailTimestampLabel->setText(QString("错误时间: %1").arg(formatDateTime(item.timestamp)));
    m_detailReviewCountLabel->setText(QString("复习次数: %1").arg(item.reviewCount));
    m_detailStatusLabel->setText(QString("状态: %1").arg(item.isResolved ? "已解决" : "未解决"));
}

void ReviewWidget::clearWrongAnswerDetails()
{
    m_detailSubjectLabel->setText("科目: -");
    m_detailTypeLabel->setText("题型: -");
    m_detailQuestionRenderer->setContent("**题目:** 请选择一道错题查看详情");
    m_detailImageLabel->setVisible(false);
    m_detailChoicesRenderer->setVisible(false);
    m_detailCorrectAnswerRenderer->setContent("**正确答案:** -");
    m_detailUserAnswerRenderer->setContent("**你的答案:** -");
    m_detailTimestampLabel->setText("错误时间: -");
    m_detailReviewCountLabel->setText("复习次数: -");
    m_detailStatusLabel->setText("状态: -");
}



QVector<WrongAnswerItem> ReviewWidget::getFilteredWrongAnswerItems() const
{
    QVector<WrongAnswerItem> filtered;
    
    if (!m_wrongAnswerSet) {
        return filtered;
    }
    
    QString selectedSubject = m_subjectCombo->currentData().toString();
    QString selectedType = m_typeCombo->currentData().toString();
    QDate fromDate = m_dateFromEdit->date();
    QDate toDate = m_dateToEdit->date();
    bool showResolved = m_showResolvedCheck->isChecked();
    
    const QVector<WrongAnswerItem> &items = m_wrongAnswerSet->getAllWrongAnswers();
    
    for (const WrongAnswerItem &item : items) {
        // Filter by subject
        if (!selectedSubject.isEmpty() && item.subject != selectedSubject) {
            continue;
        }
        
        // Filter by type
        if (!selectedType.isEmpty() && item.questionType != selectedType) {
            continue;
        }
        
        // Filter by date range
        QDate itemDate = item.timestamp.date();
        if (itemDate < fromDate || itemDate > toDate) {
            continue;
        }
        
        // Filter by resolved status
        if (!showResolved && item.isResolved) {
            continue;
        }
        
        filtered.append(item);
    }
    
    // Sort filtered results
    QString sortOrder = m_sortCombo->currentData().toString();
    if (sortOrder == "time_desc") {
        std::sort(filtered.begin(), filtered.end(),
                 [](const WrongAnswerItem &a, const WrongAnswerItem &b) {
                     return a.timestamp > b.timestamp;
                 });
    } else if (sortOrder == "time_asc") {
        std::sort(filtered.begin(), filtered.end(),
                 [](const WrongAnswerItem &a, const WrongAnswerItem &b) {
                     return a.timestamp < b.timestamp;
                 });
    } else if (sortOrder == "review_desc") {
        std::sort(filtered.begin(), filtered.end(),
                 [](const WrongAnswerItem &a, const WrongAnswerItem &b) {
                     return a.reviewCount > b.reviewCount;
                 });
    } else if (sortOrder == "review_asc") {
        std::sort(filtered.begin(), filtered.end(),
                 [](const WrongAnswerItem &a, const WrongAnswerItem &b) {
                     return a.reviewCount < b.reviewCount;
                 });
    }
    
    return filtered;
}















void ReviewWidget::exportWrongAnswers()
{
    int totalCount = 0;
    if (m_wrongAnswerSet) {
        totalCount = m_wrongAnswerSet->getTotalCount();
    }
    
    if (totalCount == 0) {
        QMessageBox::information(this, "提示", "没有错题可以导出");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "导出错题",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/错题导出.md",
        "Markdown文件 (*.md)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QString markdownContent;
    
    if (m_wrongAnswerSet) {
        // 使用新的错题集合系统导出
        markdownContent = m_wrongAnswerSet->exportToMarkdown();
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << markdownContent;
        file.close();
        
        QMessageBox::information(this, "导出成功", 
                               QString("已成功导出 %1 道错题到文件：\n%2")
                               .arg(totalCount)
                               .arg(fileName));
    } else {
        QMessageBox::warning(this, "导出失败", "无法写入文件");
    }
}

void ReviewWidget::importWrongAnswers()
{
    QMessageBox::information(this, "提示", "新的错题系统使用WA_SET.json文件管理错题，\n请通过练习模块自动添加错题。");
}

void ReviewWidget::markRecordsAsResolved(const QVector<int> &indices)
{
    if (!m_wrongAnswerSet) {
        return;
    }
    
    for (int index : indices) {
        if (index >= 0 && index < m_filteredItems.size()) {
            const WrongAnswerItem &item = m_filteredItems[index];
            m_wrongAnswerSet->markAsResolved(item.id);
        }
    }
    
    m_wrongAnswerSet->saveToFile();
}

void ReviewWidget::markRecordsAsUnresolved(const QVector<int> &indices)
{
    if (!m_wrongAnswerSet) {
        return;
    }
    
    for (int index : indices) {
        if (index >= 0 && index < m_filteredItems.size()) {
            const WrongAnswerItem &item = m_filteredItems[index];
            m_wrongAnswerSet->markAsUnresolved(item.id);
        }
    }
    
    m_wrongAnswerSet->saveToFile();
}

void ReviewWidget::deleteRecords(const QVector<int> &indices)
{
    if (!m_wrongAnswerSet) {
        return;
    }
    
    // Sort indices in descending order to avoid index shifting issues
    QVector<int> sortedIndices = indices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());
    
    for (int index : sortedIndices) {
        if (index >= 0 && index < m_filteredItems.size()) {
            const WrongAnswerItem &item = m_filteredItems[index];
            m_wrongAnswerSet->removeWrongAnswer(item.id);
        }
    }
    
    m_wrongAnswerSet->saveToFile();
}

QString ReviewWidget::getQuestionTypeString(const QString &type) const
{
    if (type == "Choice") {
        return "选择题";
    } else if (type == "TrueOrFalse") {
        return "判断题";
    } else if (type == "FillBlank") {
        return "填空题";
    } else if (type == "MultipleChoice") {
        return "多选题";
    } else {
        return "未知题型";
    }
}

QString ReviewWidget::formatDateTime(const QDateTime &dateTime) const
{
    return dateTime.toString("yyyy-MM-dd hh:mm");
}

void ReviewWidget::setWrongAnswerSet(WrongAnswerSet *wrongAnswerSet)
{
    m_wrongAnswerSet = wrongAnswerSet;
    
    if (m_wrongAnswerSet) {
        // 连接错题集合信号
        connect(m_wrongAnswerSet, &WrongAnswerSet::dataChanged,
                this, &ReviewWidget::updateFromWrongAnswerSet);
        connect(m_wrongAnswerSet, &WrongAnswerSet::wrongAnswerAdded,
                this, [this](const WrongAnswerItem &) { updateFromWrongAnswerSet(); });
        connect(m_wrongAnswerSet, &WrongAnswerSet::wrongAnswerRemoved,
                this, [this](const QString &) { updateFromWrongAnswerSet(); });
        connect(m_wrongAnswerSet, &WrongAnswerSet::wrongAnswerUpdated,
                this, [this](const QString &, const WrongAnswerItem &) { updateFromWrongAnswerSet(); });
    }
}

WrongAnswerSet* ReviewWidget::getWrongAnswerSet() const
{
    return m_wrongAnswerSet;
}

void ReviewWidget::updateFromWrongAnswerSet()
{
    if (!m_wrongAnswerSet) {
        return;
    }
    
    // 获取所有错题项目
    m_currentItems = m_wrongAnswerSet->getAllWrongAnswers();
    
    updateWrongAnswersList();
    updateStatistics();
    updateFilterOptions();
}



QStringList ReviewWidget::getSelectedIds() const
{
    QStringList ids;
    QList<QListWidgetItem*> selectedItems = m_wrongAnswersList->selectedItems();
    
    for (QListWidgetItem *item : selectedItems) {
        int index = m_wrongAnswersList->row(item);
        if (index >= 0 && index < m_filteredItems.size()) {
            ids.append(m_filteredItems[index].id);
        }
    }
    
    return ids;
}

void ReviewWidget::onWrongAnswersImportRequested(const QList<Question> &wrongQuestions, const QString &subject)
{
    qDebug() << "[DEBUG] onWrongAnswersImportRequested: Received signal with" << wrongQuestions.size() << "wrong questions for subject:" << subject;
    
    if (wrongQuestions.isEmpty()) {
        qDebug() << "[DEBUG] onWrongAnswersImportRequested: No wrong questions, returning";
        return;
    }
    
    // 询问用户是否要导入错题
    qDebug() << "[DEBUG] onWrongAnswersImportRequested: Showing question dialog to user";
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "导入错题",
        QString("本次练习产生了 %1 道错题，是否要将这些错题导入到错题系统中？\n\n"
                "导入后可以在错题复习中查看和练习这些题目。")
            .arg(wrongQuestions.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );
    
    qDebug() << "[DEBUG] onWrongAnswersImportRequested: User replied:" << (reply == QMessageBox::Yes ? "Yes" : "No");
    
    if (reply == QMessageBox::Yes && m_practiceManager) {
        qDebug() << "[DEBUG] onWrongAnswersImportRequested: Calling importWrongAnswersToSet";
        // 调用PracticeManager的导入方法
        const int addedCount = m_practiceManager->importWrongAnswersToSet();
        if (addedCount < 0) {
            QMessageBox::warning(this, "导入失败", "错题导入或保存失败，请稍后重试。");
            return;
        }

        if (addedCount == wrongQuestions.size()) {
            QMessageBox::information(
                this,
                "导入成功",
                QString("已成功导入 %1 道错题到错题系统中。")
                    .arg(addedCount)
            );
        } else {
            QMessageBox::information(
                this,
                "导入完成",
                QString("本次新增导入 %1 道错题（%2 道已存在，已跳过）。")
                    .arg(addedCount)
                    .arg(wrongQuestions.size() - addedCount)
            );
        }
        
        qDebug() << "[DEBUG] onWrongAnswersImportRequested: Refreshing wrong answers display";
        // 刷新显示
        loadWrongAnswers();
    } else {
        qDebug() << "[DEBUG] onWrongAnswersImportRequested: Import cancelled or PracticeManager is null";
    }
}

Question ReviewWidget::convertWrongAnswerItemToQuestion(const WrongAnswerItem &item) const
{
    Question question;
    
    // 设置题目类型
    if (item.questionType == "Choice") {
        question.setType(QuestionType::Choice);
    } else if (item.questionType == "TrueOrFalse") {
        question.setType(QuestionType::TrueOrFalse);
    } else if (item.questionType == "FillBlank") {
        question.setType(QuestionType::FillBlank);
    } else if (item.questionType == "MultiChoice") {
        question.setType(QuestionType::MultipleChoice);
    } else {
        question.setType(QuestionType::Choice); // 默认为选择题
    }
    
    // 设置题目内容
    question.setQuestion(item.questionText);
    
    // 设置图片
    if (!item.images.isEmpty()) {
        question.setImages(item.images);
    }
    
    // 设置选项（如果有）
    if (!item.choices.isEmpty()) {
        question.setChoices(item.choices);
    }
    
    // 设置正确答案
    if (!item.correctAnswers.isEmpty()) {
        question.setAnswers(item.correctAnswers);
    } else if (!item.correctAnswer.isEmpty()) {
        question.setSingleAnswer(item.correctAnswer);
    }
    
    // 设置填空题的空格数量
    if (question.getType() == QuestionType::FillBlank) {
        question.setBlankNum(item.correctAnswers.size());
    }
    
    return question;
}
