#include "questionassistantwidget.h"
#include "questionpreviewwidget.h"
#include "ptaassistcontroller.h"
#include "../core/ocsserver.h"
#include "../core/configmanager.h"
#include "../utils/questionsearchindex.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QMessageBox>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QLineEdit>
#include <QToolButton>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QColor>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QTextEdit>
#include <QDateTime>
#include <QHeaderView>
#include <QClipboard>
#include <QGuiApplication>
#include <QGroupBox>
#include <QGridLayout>
#include <QTextCursor>

QuestionAssistantWidget::QuestionAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , m_configManager(nullptr)
    , m_searchIndex(new QuestionSearchIndex(this))
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_backButton(nullptr)
    , m_titleLabel(nullptr)
    , m_tabs(nullptr)
    , m_searchTab(nullptr)
    , m_ptaTab(nullptr)
    , m_ocsTab(nullptr)
    , m_indexStatusLabel(nullptr)
    , m_queryEdit(nullptr)
    , m_topKSpinBox(nullptr)
    , m_searchButton(nullptr)
    // , m_rebuildIndexButton(nullptr) // Removed
    , m_resultsTree(nullptr)
    , m_previewWidget(nullptr)
    , m_ptaController(new PtaAssistController(this))
    , m_ptaWebView(nullptr)
    , m_ptaBackButton(nullptr)
    , m_ptaForwardButton(nullptr)
    , m_ptaReloadButton(nullptr)
    , m_ptaAddressBar(nullptr)
    , m_ptaParseButton(nullptr)
    , m_ptaThresholdSpinBox(nullptr)
    , m_ptaAutoAnswerButton(nullptr)
    , m_ptaStopAutoButton(nullptr)
    , m_ptaExportNewButton(nullptr)
    , m_ptaQuestionList(nullptr)
    , m_logEdit(nullptr)
    , m_ptaPageTipLabel(nullptr)
    , m_ptaCurrentQuestionPreview(nullptr)
    , m_ptaTopKSpinBox(nullptr)
    , m_ptaSearchButton(nullptr)
    , m_ptaResultsTree(nullptr)
    , m_ptaFillButton(nullptr)
    , m_ptaSelectedBankPreview(nullptr)
    , m_ocsServer(new OcsServer(m_searchIndex, this))
    , m_ocsStatusLabel(nullptr)
    , m_ocsUrlLabel(nullptr)
    , m_ocsHostEdit(nullptr)
    , m_ocsPortSpinBox(nullptr)
    , m_ocsThresholdSpinBox(nullptr)
    , m_ocsTopKSpinBox(nullptr)
    , m_ocsStartButton(nullptr)
    , m_ocsStopButton(nullptr)
    , m_ocsRefreshIndexButton(nullptr)
    , m_ocsCopyConfigButton(nullptr)
    , m_ocsExportUnmatchedButton(nullptr)
    , m_ocsClearUnmatchedButton(nullptr)
    , m_ocsUnmatchedLabel(nullptr)
    , m_ocsLogEdit(nullptr)
    , m_ocsConfigEdit(nullptr)
{
    setupUI();
    setupConnections();
}

QuestionAssistantWidget::~QuestionAssistantWidget()
{
}

void QuestionAssistantWidget::setConfigManager(ConfigManager *configManager)
{
    m_configManager = configManager;

    if (!m_configManager) {
        return;
    }

    const int k = m_configManager->getAssistantSearchTopK();
    const double th = m_configManager->getAssistantAutoThreshold();

    if (m_topKSpinBox) {
        m_topKSpinBox->setValue(k);
        connect(m_topKSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
            if (!m_configManager) return;
            m_configManager->setAssistantSearchTopK(v);
            m_configManager->saveConfig();
        }, Qt::UniqueConnection);
    }
    if (m_ptaTopKSpinBox) {
        m_ptaTopKSpinBox->setValue(k);
        connect(m_ptaTopKSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
            if (!m_configManager) return;
            m_configManager->setAssistantSearchTopK(v);
            m_configManager->saveConfig();
        }, Qt::UniqueConnection);
    }
    if (m_ptaThresholdSpinBox) {
        m_ptaThresholdSpinBox->setValue(th);
        connect(m_ptaThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
            if (!m_configManager) return;
            m_configManager->setAssistantAutoThreshold(v);
            m_configManager->saveConfig();
        }, Qt::UniqueConnection);
    }
    if (m_ocsHostEdit) {
        m_ocsHostEdit->setText(m_configManager->getOcsServiceHost());
    }
    if (m_ocsPortSpinBox) {
        m_ocsPortSpinBox->setValue(m_configManager->getOcsServicePort());
    }
    if (m_ocsThresholdSpinBox) {
        m_ocsThresholdSpinBox->setValue(m_configManager->getOcsServiceThreshold());
    }
    if (m_ocsTopKSpinBox) {
        m_ocsTopKSpinBox->setValue(m_configManager->getOcsServiceTopK());
    }
    updateOcsConfigText();
    updateOcsServiceState();
}

bool QuestionAssistantWidget::prepareForShow()
{
    return ensureIndexReady(true);
}

void QuestionAssistantWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    m_mainLayout->setSpacing(12);

    m_headerLayout = new QHBoxLayout();
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(8);

    m_backButton = new QPushButton("返回主页", this);
    m_backButton->setAutoDefault(false);
    m_backButton->setDefault(false);

    m_titleLabel = new QLabel("题目助手", this);
    QFont f = m_titleLabel->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    m_titleLabel->setFont(f);

    m_headerLayout->addWidget(m_backButton);
    m_headerLayout->addSpacing(8);
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();

    m_tabs = new QTabWidget(this);
    m_searchTab = new QWidget(m_tabs);
    m_ptaTab = new QWidget(m_tabs);
    m_ocsTab = new QWidget(m_tabs);
    m_tabs->addTab(m_searchTab, "搜题");
    m_tabs->addTab(m_ptaTab, "辅助答题(PTA)");
    m_tabs->addTab(m_ocsTab, "OCS服务");

    m_mainLayout->addLayout(m_headerLayout);
    m_mainLayout->addWidget(m_tabs, 1);
    setLayout(m_mainLayout);

    setupSearchTab();
    setupPtaTab();
    setupOcsTab();
}

void QuestionAssistantWidget::setupConnections()
{
    connect(m_backButton, &QPushButton::clicked, this, &QuestionAssistantWidget::backRequested);
}

void QuestionAssistantWidget::setupSearchTab()
{
    QVBoxLayout *root = new QVBoxLayout(m_searchTab);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    m_indexStatusLabel = new QLabel(m_searchTab);
    m_indexStatusLabel->setText("题库索引：未加载");

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_searchTab);

    QWidget *left = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_queryEdit = new QPlainTextEdit(left);
    m_queryEdit->setPlaceholderText("粘贴题目内容（可为部分题目）...");
    m_queryEdit->setMinimumHeight(140);

    QHBoxLayout *ctrl = new QHBoxLayout();
    ctrl->setContentsMargins(0, 0, 0, 0);
    ctrl->setSpacing(8);

    QLabel *kLabel = new QLabel("Top-K：", left);
    m_topKSpinBox = new QSpinBox(left);
    m_topKSpinBox->setRange(1, 50);
    m_topKSpinBox->setValue(5);
    m_topKSpinBox->setMinimumWidth(80);

    m_searchButton = new QPushButton("搜题", left);
    m_searchButton->setAutoDefault(false);
    m_searchButton->setDefault(false);

    // Removed Rebuild Index Button

    ctrl->addWidget(kLabel);
    ctrl->addWidget(m_topKSpinBox);
    ctrl->addStretch();
    // ctrl->addWidget(m_rebuildIndexButton);
    ctrl->addWidget(m_searchButton);

    m_resultsTree = new QTreeWidget(left);
    m_resultsTree->setHeaderLabels(QStringList() << "题型" << "科目" << "题库" << "来源" << "相似度");
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setRootIsDecorated(false);
    m_resultsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultsTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTree->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_resultsTree->header()->resizeSection(0, 60);  // 题型
    m_resultsTree->header()->resizeSection(1, 80);  // 科目
    m_resultsTree->header()->resizeSection(2, 120); // 题库
    m_resultsTree->header()->resizeSection(3, 120); // 来源
    // 相似度 auto

    // QSS Styling for Search Tab
    left->setStyleSheet(
        "QTreeWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-size: 13px; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:selected { background-color: #e6f3ff; color: #000; }"
        "QPlainTextEdit { border: 1px solid #dcdcdc; border-radius: 4px; padding: 4px; }"
        "QPushButton { padding: 5px 10px; border-radius: 4px; background-color: #007bff; color: white; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:pressed { background-color: #004085; }"
    );

    leftLayout->addWidget(m_queryEdit);
    leftLayout->addLayout(ctrl);
    leftLayout->addWidget(m_resultsTree, 1);
    left->setLayout(leftLayout);

    QWidget *right = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_previewWidget = new QuestionPreviewWidget(right);
    rightLayout->addWidget(m_previewWidget, 1);
    right->setLayout(rightLayout);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 600 << 800);

    root->addWidget(m_indexStatusLabel);
    root->addWidget(splitter, 1);
    m_searchTab->setLayout(root);

    // Removed m_rebuildIndexButton connect

    connect(m_searchButton, &QPushButton::clicked, this, [this]() {
        if (!ensureIndexReady(false)) {
            return;
        }
        const QString query = m_queryEdit->toPlainText().trimmed();
        const int k = m_topKSpinBox->value();
        m_resultsTree->clear();
        m_previewWidget->clear();

        const QVector<SearchHit> hits = m_searchIndex->searchTopK(query, k);
        if (hits.isEmpty()) {
            // m_resultsList->addItem("未找到相似题目"); // ListWidget legacy
            return;
        }

        for (const SearchHit &h : hits) {
            const QuestionSourceInfo &src = m_searchIndex->documentSource(h.docIndex);
            const Question &q = m_searchIndex->documentQuestion(h.docIndex);
            
            QTreeWidgetItem *item = new QTreeWidgetItem(m_resultsTree);
            item->setText(0, Question::typeToString(q.getType()));
            item->setText(1, src.subject);
            item->setText(2, src.bankName.isEmpty() ? "未命名题库" : src.bankName);
            item->setText(3, QFileInfo(src.bankSrc).fileName()); // Show filename only for cleaner view
            item->setText(4, QString::number(h.score, 'f', 3));
            item->setToolTip(3, src.bankSrc); // Full path in tooltip

            item->setData(0, Qt::UserRole, h.docIndex);
            item->setData(0, Qt::UserRole + 1, h.score);
        }
    });

    connect(m_resultsTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column) {
        if (!item) {
            return;
        }
        const QVariant v = item->data(0, Qt::UserRole);
        if (!v.isValid()) {
            return;
        }
        const int docIndex = v.toInt();
        if (docIndex < 0 || docIndex >= m_searchIndex->documentCount()) {
            return;
        }
        m_previewWidget->setQuestion(m_searchIndex->documentQuestion(docIndex));
    });
}

void QuestionAssistantWidget::setupPtaTab()
{
    QVBoxLayout *root = new QVBoxLayout(m_ptaTab);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_ptaTab);

    QWidget *left = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_ptaParseButton = new QPushButton("解析当前页面题目", left);
    m_ptaParseButton->setAutoDefault(false);
    m_ptaParseButton->setDefault(false);

    QHBoxLayout *autoRow = new QHBoxLayout();
    autoRow->setContentsMargins(0, 0, 0, 0);
    autoRow->setSpacing(8);

    QLabel *thLabel = new QLabel("阈值：", left);
    m_ptaThresholdSpinBox = new QDoubleSpinBox(left);
    m_ptaThresholdSpinBox->setRange(0.0, 1.0);
    m_ptaThresholdSpinBox->setSingleStep(0.05);
    m_ptaThresholdSpinBox->setDecimals(2);
    m_ptaThresholdSpinBox->setValue(0.85);
    m_ptaThresholdSpinBox->setFixedWidth(100);

    m_ptaAutoAnswerButton = new QPushButton("自动答题", left);
    m_ptaAutoAnswerButton->setAutoDefault(false);
    m_ptaAutoAnswerButton->setDefault(false);

    m_ptaStopAutoButton = new QPushButton("停止", left);
    m_ptaStopAutoButton->setAutoDefault(false);
    m_ptaStopAutoButton->setDefault(false);
    m_ptaStopAutoButton->setEnabled(false);

    autoRow->addWidget(thLabel);
    autoRow->addWidget(m_ptaThresholdSpinBox);
    autoRow->addStretch();
    autoRow->addWidget(m_ptaAutoAnswerButton);
    autoRow->addWidget(m_ptaStopAutoButton);

    m_ptaExportNewButton = new QPushButton("导出新题", left);
    m_ptaExportNewButton->setAutoDefault(false);
    m_ptaExportNewButton->setDefault(false);

    m_ptaQuestionList = new QListWidget(left);
    
    // QSS for PTA Left Panel
    left->setStyleSheet(
        "QListWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-size: 13px; background-color: #f9f9f9; }"
        // Removed item styling to avoid conflict with setBackground()
        "QTextEdit { border: 1px solid #dcdcdc; border-radius: 4px; background-color: #f5f5f5; color: #555; }"
        "QPushButton { padding: 5px; border-radius: 4px; background-color: #f0f0f0; border: 1px solid #ccc; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        "QPushButton#actionBtn { background-color: #007bff; color: white; border: none; }"
        "QPushButton#actionBtn:hover { background-color: #0056b3; }"
        "QPushButton#stopBtn { background-color: #dc3545; color: white; border: none; }"
        "QPushButton#stopBtn:hover { background-color: #c82333; }"
    );
    
    // Assign Object Names for styling
    m_ptaParseButton->setObjectName("actionBtn");
    m_ptaAutoAnswerButton->setObjectName("actionBtn");
    m_ptaStopAutoButton->setObjectName("stopBtn");

    m_logEdit = new QTextEdit(left);
    m_logEdit->setReadOnly(true);
    m_logEdit->setPlaceholderText("操作日志...");
    m_logEdit->setMaximumHeight(120);

    leftLayout->addWidget(m_ptaParseButton);
    leftLayout->addLayout(autoRow);
    leftLayout->addWidget(m_ptaExportNewButton);
    leftLayout->addWidget(m_ptaQuestionList, 1);
    leftLayout->addWidget(m_logEdit);
    left->setLayout(leftLayout);

    QWidget *middle = new QWidget(splitter);
    QVBoxLayout *midLayout = new QVBoxLayout(middle);
    midLayout->setContentsMargins(0, 0, 0, 0);
    midLayout->setSpacing(8);

    QHBoxLayout *nav = new QHBoxLayout();
    nav->setContentsMargins(0, 0, 0, 0);
    nav->setSpacing(6);

    m_ptaBackButton = new QToolButton(middle);
    m_ptaBackButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_ptaBackButton->setEnabled(false);

    m_ptaForwardButton = new QToolButton(middle);
    m_ptaForwardButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    m_ptaForwardButton->setEnabled(false);

    m_ptaReloadButton = new QToolButton(middle);
    m_ptaReloadButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    m_ptaAddressBar = new QLineEdit(middle);
    m_ptaAddressBar->setPlaceholderText("输入网址并回车");

    nav->addWidget(m_ptaBackButton);
    nav->addWidget(m_ptaForwardButton);
    nav->addWidget(m_ptaReloadButton);
    nav->addWidget(m_ptaAddressBar, 1);

    m_ptaWebView = new QWebEngineView(middle);
    const QUrl initialUrl("https://pintia.cn/auth/login");
    m_ptaAddressBar->setText(initialUrl.toString());
    m_ptaWebView->setUrl(initialUrl);
    m_ptaController->setWebView(m_ptaWebView);

    midLayout->addLayout(nav);
    midLayout->addWidget(m_ptaWebView, 1);
    middle->setLayout(midLayout);

    QWidget *right = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_ptaPageTipLabel = new QLabel("提示：请登录PTA，并打开包含题目的作业/题目列表页面，然后点击“解析当前页面题目”。", right);
    m_ptaPageTipLabel->setWordWrap(true);

    m_ptaCurrentQuestionPreview = new QuestionPreviewWidget(right);
    m_ptaCurrentQuestionPreview->hide(); // Hidden as per user request (removed from layout)
    m_ptaSelectedBankPreview = new QuestionPreviewWidget(right);

    QHBoxLayout *ctrl = new QHBoxLayout();
    ctrl->setContentsMargins(0, 0, 0, 0);
    ctrl->setSpacing(8);

    QLabel *kLabel = new QLabel("Top-K：", right);
    m_ptaTopKSpinBox = new QSpinBox(right);
    m_ptaTopKSpinBox->setRange(1, 50);
    m_ptaTopKSpinBox->setValue(5);
    m_ptaTopKSpinBox->setMinimumWidth(80);

    m_ptaSearchButton = new QPushButton("搜题", right);
    m_ptaSearchButton->setAutoDefault(false);
    m_ptaSearchButton->setDefault(false);

    m_ptaFillButton = new QPushButton("填入答案", right);
    m_ptaFillButton->setAutoDefault(false);
    m_ptaFillButton->setDefault(false);

    ctrl->addWidget(kLabel);
    ctrl->addWidget(m_ptaTopKSpinBox);
    ctrl->addStretch();
    ctrl->addWidget(m_ptaSearchButton);
    ctrl->addWidget(m_ptaFillButton);

    m_ptaResultsTree = new QTreeWidget(right);
    m_ptaResultsTree->setHeaderLabels(QStringList() << "题型" << "科目" << "题库" << "来源" << "相似度");
    m_ptaResultsTree->setAlternatingRowColors(true);
    m_ptaResultsTree->setRootIsDecorated(false);
    m_ptaResultsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ptaResultsTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ptaResultsTree->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_ptaResultsTree->header()->resizeSection(0, 60);  // 题型
    m_ptaResultsTree->header()->resizeSection(1, 80);  // 科目
    m_ptaResultsTree->header()->resizeSection(2, 120); // 题库
    m_ptaResultsTree->header()->resizeSection(3, 120); // 来源

    // QSS for PTA Right Panel
    right->setStyleSheet(
        "QTreeWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-size: 12px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background-color: #e6f3ff; color: #000; }"
        "QLabel { font-weight: bold; color: #333; margin-top: 5px; }"
        "QPushButton { padding: 4px 8px; border-radius: 4px; background-color: #007bff; color: white; border: none; }"
        "QPushButton:hover { background-color: #0056b3; }"
    );

    rightLayout->addWidget(m_ptaPageTipLabel);
    // Removed Current Question Preview as per user request
    // rightLayout->addWidget(new QLabel("当前题目", right));
    // rightLayout->addWidget(m_ptaCurrentQuestionPreview, 1);
    rightLayout->addLayout(ctrl);
    rightLayout->addWidget(new QLabel("相似题结果（点击查看详情）", right));
    rightLayout->addWidget(m_ptaResultsTree, 1);
    rightLayout->addWidget(new QLabel("选择的题库题目（用于填入答案）", right));
    rightLayout->addWidget(m_ptaSelectedBankPreview, 1);
    right->setLayout(rightLayout);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes(QList<int>() << 320 << 900 << 600);

    root->addWidget(splitter, 1);
    m_ptaTab->setLayout(root);

    connect(m_ptaBackButton, &QToolButton::clicked, m_ptaWebView, &QWebEngineView::back);
    connect(m_ptaForwardButton, &QToolButton::clicked, m_ptaWebView, &QWebEngineView::forward);
    connect(m_ptaReloadButton, &QToolButton::clicked, m_ptaWebView, &QWebEngineView::reload);

    connect(m_ptaAddressBar, &QLineEdit::returnPressed, this, [this]() {
        QString urlText = m_ptaAddressBar->text().trimmed();
        if (urlText.isEmpty()) {
            return;
        }
        if (!urlText.contains("://")) {
            urlText = "https://" + urlText;
        }
        const QUrl url = QUrl::fromUserInput(urlText);
        if (url.isValid()) {
            m_ptaWebView->setUrl(url);
        }
    });

    connect(m_ptaWebView, &QWebEngineView::urlChanged, this, [this](const QUrl &url) {
        if (!m_ptaAddressBar->hasFocus()) {
            m_ptaAddressBar->setText(url.toString());
        }
        if (m_ptaWebView->history()) {
            m_ptaBackButton->setEnabled(m_ptaWebView->history()->canGoBack());
            m_ptaForwardButton->setEnabled(m_ptaWebView->history()->canGoForward());
        }
    });

    connect(m_ptaWebView, &QWebEngineView::loadFinished, this, [this](bool) {
        if (m_ptaWebView->history()) {
            m_ptaBackButton->setEnabled(m_ptaWebView->history()->canGoBack());
            m_ptaForwardButton->setEnabled(m_ptaWebView->history()->canGoForward());
        }
    });

    auto applyItemColor = [this](const QString &ptaId) {
        updatePtaQuestionItemVisual(ptaId);
    };

    auto renderPtaCache = [this](const QString &ptaId) {
        m_ptaResultsTree->clear();
        m_ptaSelectedBankPreview->clear();

        if (!m_ptaCache.contains(ptaId)) {
            return;
        }
        const PtaCacheEntry entry = m_ptaCache.value(ptaId);
        for (const SearchHit &h : entry.hits) {
            const QuestionSourceInfo &src = m_searchIndex->documentSource(h.docIndex);
            const Question &q = m_searchIndex->documentQuestion(h.docIndex);
            
            QTreeWidgetItem *item = new QTreeWidgetItem(m_ptaResultsTree);
            item->setText(0, Question::typeToString(q.getType()));
            item->setText(1, src.subject);
            item->setText(2, src.bankName.isEmpty() ? "未命名题库" : src.bankName);
            item->setText(3, QFileInfo(src.bankSrc).fileName());
            item->setText(4, QString::number(h.score, 'f', 3));
            item->setToolTip(3, src.bankSrc);

            item->setData(0, Qt::UserRole, h.docIndex);
            item->setData(0, Qt::UserRole + 1, h.score);
        }

        if (entry.selectedDocIndex >= 0 && entry.selectedDocIndex < m_searchIndex->documentCount()) {
            m_ptaSelectedBankPreview->setQuestion(m_searchIndex->documentQuestion(entry.selectedDocIndex));
        }
    };

    connect(m_ptaParseButton, &QPushButton::clicked, this, [this]() {
        m_ptaController->parseVisibleQuestions();
    });

    connect(m_ptaAutoAnswerButton, &QPushButton::clicked, this, [this]() {
        startPtaAutoAnswer();
    });

    connect(m_ptaStopAutoButton, &QPushButton::clicked, this, [this]() {
        stopPtaAutoAnswer();
    });

    connect(m_ptaExportNewButton, &QPushButton::clicked, this, [this]() {
        exportPtaNewQuestions();
    });

    connect(m_ptaController, &PtaAssistController::parsedJsonReady, this, [this, renderPtaCache, applyItemColor](const QString &jsonText) {
        m_ptaQuestions.clear();
        m_ptaQuestionList->clear();
        m_currentPtaId.clear();
        m_ptaCurrentQuestionPreview->clear();
        m_ptaResultsTree->clear();
        m_ptaSelectedBankPreview->clear();

        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QMessageBox::warning(this, "解析失败", "无法解析页面题目数据");
            return;
        }

        const QJsonObject rootObj = doc.object();
        const QJsonArray data = rootObj.value("data").toArray();
        for (int i = 0; i < data.size(); ++i) {
            if (!data[i].isObject()) continue;
            const QJsonObject o = data[i].toObject();
            ParsedPtaQuestion pq;
            pq.id = o.value("id").toString().trimmed();
            if (pq.id.isEmpty()) {
                pq.id = QString("idx_%1").arg(i + 1);
            }
            pq.label = o.value("label").toString().trimmed();
            pq.type = o.value("type").toString().trimmed();
            pq.question = o.value("question").toString();
            pq.blankNum = o.value("BlankNum").toInt(0);

            if (o.value("choices").isArray()) {
                const QJsonArray arr = o.value("choices").toArray();
                for (const auto &v : arr) {
                    if (!v.isObject()) continue;
                    const QJsonObject opt = v.toObject();
                    const QString tag = opt.value("tag").toString().trimmed();
                    const QString txt = opt.value("text").toString().trimmed();
                    if (!tag.isEmpty() && !txt.isEmpty()) {
                        pq.choices.append(tag + "." + txt);
                    }
                }
            }

            if (o.value("image").isObject()) {
                const QJsonObject img = o.value("image").toObject();
                for (auto it = img.begin(); it != img.end(); ++it) {
                    const QString k = it.key().trimmed();
                    const QString v = it.value().toString().trimmed();
                    if (!k.isEmpty() && !v.isEmpty()) {
                        pq.images.insert(k, v);
                    }
                }
            }

            m_ptaQuestions.insert(pq.id, pq);

            const QString displayLabel = pq.label.isEmpty()
                ? QString("%1 [%2]").arg(pq.id, pq.type)
                : QString("%1 [%2]").arg(pq.label, pq.type);

            QListWidgetItem *item = new QListWidgetItem(displayLabel, m_ptaQuestionList);
            item->setData(Qt::UserRole, pq.id);
            m_ptaQuestionList->addItem(item);

            if (!m_ptaCache.contains(pq.id)) {
                PtaCacheEntry e;
                e.filled = false;
                m_ptaCache.insert(pq.id, e);
            }
            applyItemColor(pq.id);
        }

        if (m_ptaQuestionList->count() > 0) {
            m_ptaQuestionList->setCurrentRow(0);
            QListWidgetItem *it = m_ptaQuestionList->item(0);
            if (it) {
                const QString id = it->data(Qt::UserRole).toString();
                m_currentPtaId = id;
                const ParsedPtaQuestion pq = m_ptaQuestions.value(id);
                Question q;
                q.setType(Question::stringToType(pq.type));
                q.setQuestion(pq.question);
                q.setChoices(pq.choices);
                q.setBlankNum(pq.blankNum);
                q.setImages(pq.images);
                m_ptaCurrentQuestionPreview->setQuestion(q);
                renderPtaCache(id);
            }
        }
    });

    connect(m_ptaQuestionList, &QListWidget::itemClicked, this, [this, renderPtaCache](QListWidgetItem *item) {
        if (!item) return;
        const QString id = item->data(Qt::UserRole).toString();
        if (id.isEmpty()) return;
        m_currentPtaId = id;

        const ParsedPtaQuestion pq = m_ptaQuestions.value(id);
        Question q;
        q.setType(Question::stringToType(pq.type));
        q.setQuestion(pq.question);
        q.setChoices(pq.choices);
        q.setBlankNum(pq.blankNum);
        q.setImages(pq.images);
        m_ptaCurrentQuestionPreview->setQuestion(q);

        if (!id.startsWith("idx_")) {
            m_ptaController->scrollToQuestionId(id);
        }

        renderPtaCache(id);
    });

    connect(m_ptaSearchButton, &QPushButton::clicked, this, [this, renderPtaCache, applyItemColor]() {
        if (!ensureIndexReady(false)) {
            return;
        }
        if (m_currentPtaId.isEmpty() || !m_ptaQuestions.contains(m_currentPtaId)) {
            QMessageBox::information(this, "提示", "请先解析并选择题目");
            return;
        }
        const ParsedPtaQuestion pq = m_ptaQuestions.value(m_currentPtaId);
        QString query = buildPtaQueryText(pq); // Use helper
        
        const int k = m_ptaTopKSpinBox->value();
        PtaCacheEntry entry = m_ptaCache.value(m_currentPtaId);
        entry.hits = m_searchIndex->searchTopK(query, k);
        if (!entry.hits.isEmpty()) {
            entry.selectedDocIndex = entry.hits.first().docIndex;
            entry.bestScore = entry.hits.first().score; // Store best score
        } else {
            entry.selectedDocIndex = -1;
            entry.bestScore = 0.0;
        }
        entry.noMatch = entry.hits.isEmpty(); // Basic check
        
        m_ptaCache.insert(m_currentPtaId, entry);
        renderPtaCache(m_currentPtaId);
        applyItemColor(m_currentPtaId);
    });

    connect(m_ptaResultsTree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column) {
        if (!item) return;
        const int docIndex = item->data(0, Qt::UserRole).toInt();
        if (docIndex < 0 || docIndex >= m_searchIndex->documentCount()) return;
        PtaCacheEntry entry = m_ptaCache.value(m_currentPtaId);
        entry.selectedDocIndex = docIndex;
        m_ptaCache.insert(m_currentPtaId, entry);
        m_ptaSelectedBankPreview->setQuestion(m_searchIndex->documentQuestion(docIndex));
    });

    connect(m_ptaFillButton, &QPushButton::clicked, this, [this]() {
        if (m_currentPtaId.isEmpty()) {
            return;
        }
        const PtaCacheEntry entry = m_ptaCache.value(m_currentPtaId);
        if (entry.selectedDocIndex < 0 || entry.selectedDocIndex >= m_searchIndex->documentCount()) {
            QMessageBox::information(this, "提示", "请先在右侧选择一个题库题目");
            return;
        }
        const Question &q = m_searchIndex->documentQuestion(entry.selectedDocIndex);
        if (!m_currentPtaId.startsWith("idx_")) {
            m_ptaController->fillFromBankQuestion(m_currentPtaId, q);
        } else {
            QMessageBox::information(this, "提示", "该题目缺少可定位ID，无法自动填入");
        }
    });

    connect(m_ptaController, &PtaAssistController::fillFinished, this, [this, applyItemColor](const QString &ptaId, bool ok, const QString &message) {
        if (!ok) {
            log(QString("题目 %1 填入失败: %2").arg(ptaId, message));
            if (m_ptaAutoRunning && m_ptaAutoPos < m_ptaAutoQueue.size() && m_ptaAutoQueue[m_ptaAutoPos] == ptaId) {
                PtaCacheEntry entry = m_ptaCache.value(ptaId);
                entry.filled = false;
                entry.noMatch = true;
                m_ptaCache.insert(ptaId, entry);
                applyItemColor(ptaId);
                m_ptaAutoPos++;
                QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
                return;
            }
            QMessageBox::warning(this, "填入失败", message);
            return;
        }

        log(QString("题目 %1 填入成功").arg(ptaId));
        PtaCacheEntry entry = m_ptaCache.value(ptaId);
        entry.filled = true;
        entry.noMatch = false;
        m_ptaCache.insert(ptaId, entry);
        applyItemColor(ptaId);

        if (m_ptaAutoRunning && m_ptaAutoPos < m_ptaAutoQueue.size() && m_ptaAutoQueue[m_ptaAutoPos] == ptaId) {
            m_ptaAutoPos++;
            QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
        }
    });

    // Auto Load Logic
    connect(this, &QuestionAssistantWidget::backRequested, this, [this](){
        // Stop auto answer when leaving
        if (m_ptaAutoRunning) {
            stopPtaAutoAnswer();
        }
    });
}

void QuestionAssistantWidget::setupOcsTab()
{
    QVBoxLayout *root = new QVBoxLayout(m_ocsTab);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_ocsTab);

    QWidget *left = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    QGroupBox *statusBox = new QGroupBox("服务状态", left);
    QVBoxLayout *statusLayout = new QVBoxLayout(statusBox);
    statusLayout->setSpacing(8);

    m_ocsStatusLabel = new QLabel("状态：未启动", statusBox);
    m_ocsUrlLabel = new QLabel("地址：-", statusBox);
    m_ocsUrlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QHBoxLayout *buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(8);
    m_ocsStartButton = new QPushButton("启动服务", statusBox);
    m_ocsStopButton = new QPushButton("停止服务", statusBox);
    m_ocsRefreshIndexButton = new QPushButton("刷新题库索引", statusBox);
    m_ocsStopButton->setEnabled(false);
    buttonRow->addWidget(m_ocsStartButton);
    buttonRow->addWidget(m_ocsStopButton);
    buttonRow->addWidget(m_ocsRefreshIndexButton);

    statusLayout->addWidget(m_ocsStatusLabel);
    statusLayout->addWidget(m_ocsUrlLabel);
    statusLayout->addLayout(buttonRow);

    QGroupBox *configBox = new QGroupBox("服务配置", left);
    QGridLayout *configGrid = new QGridLayout(configBox);
    configGrid->setContentsMargins(14, 14, 14, 14);
    configGrid->setHorizontalSpacing(14);
    configGrid->setVerticalSpacing(12);
    configGrid->setColumnStretch(0, 0);
    configGrid->setColumnStretch(1, 1);

    m_ocsHostEdit = new QLineEdit(configBox);
    m_ocsHostEdit->setText("127.0.0.1");
    m_ocsHostEdit->setPlaceholderText("127.0.0.1");
    m_ocsHostEdit->setMinimumWidth(220);

    m_ocsPortSpinBox = new QSpinBox(configBox);
    m_ocsPortSpinBox->setRange(1, 65535);
    m_ocsPortSpinBox->setValue(27419);
    m_ocsPortSpinBox->setMinimumWidth(140);
    m_ocsPortSpinBox->setAlignment(Qt::AlignRight);

    m_ocsThresholdSpinBox = new QDoubleSpinBox(configBox);
    m_ocsThresholdSpinBox->setRange(0.0, 1.0);
    m_ocsThresholdSpinBox->setSingleStep(0.05);
    m_ocsThresholdSpinBox->setDecimals(2);
    m_ocsThresholdSpinBox->setValue(0.85);
    m_ocsThresholdSpinBox->setMinimumWidth(140);
    m_ocsThresholdSpinBox->setAlignment(Qt::AlignRight);

    m_ocsTopKSpinBox = new QSpinBox(configBox);
    m_ocsTopKSpinBox->setRange(1, 50);
    m_ocsTopKSpinBox->setValue(5);
    m_ocsTopKSpinBox->setMinimumWidth(140);
    m_ocsTopKSpinBox->setAlignment(Qt::AlignRight);

    auto addConfigRow = [configBox, configGrid](int row, const QString &text, QWidget *editor) {
        QLabel *label = new QLabel(text, configBox);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        configGrid->addWidget(label, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
        configGrid->addWidget(editor, row, 1, Qt::AlignRight | Qt::AlignVCenter);
    };

    addConfigRow(0, "监听地址：", m_ocsHostEdit);
    addConfigRow(1, "端口：", m_ocsPortSpinBox);
    addConfigRow(2, "相似度阈值：", m_ocsThresholdSpinBox);
    addConfigRow(3, "返回数量：", m_ocsTopKSpinBox);

    m_ocsLogEdit = new QTextEdit(left);
    m_ocsLogEdit->setReadOnly(true);
    m_ocsLogEdit->setPlaceholderText("服务日志...");

    m_ocsUnmatchedLabel = new QLabel("未命中题目：0", left);
    QHBoxLayout *unmatchedRow = new QHBoxLayout();
    unmatchedRow->setContentsMargins(0, 0, 0, 0);
    unmatchedRow->setSpacing(8);
    m_ocsExportUnmatchedButton = new QPushButton("导出未命中题目", left);
    m_ocsClearUnmatchedButton = new QPushButton("清空记录", left);
    unmatchedRow->addWidget(m_ocsExportUnmatchedButton);
    unmatchedRow->addWidget(m_ocsClearUnmatchedButton);
    unmatchedRow->addStretch();

    leftLayout->addWidget(statusBox);
    leftLayout->addWidget(configBox);
    leftLayout->addWidget(m_ocsUnmatchedLabel);
    leftLayout->addLayout(unmatchedRow);
    leftLayout->addWidget(new QLabel("日志", left));
    leftLayout->addWidget(m_ocsLogEdit, 1);
    left->setLayout(leftLayout);

    QWidget *right = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    QLabel *configTitle = new QLabel("OCS题库配置文本", right);
    m_ocsConfigEdit = new QPlainTextEdit(right);
    m_ocsConfigEdit->setReadOnly(true);
    m_ocsConfigEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_ocsConfigEdit->setPlaceholderText("启动或修改配置后自动生成，可直接复制到 OCS 网课助手的题库配置中。");

    QHBoxLayout *configButtonRow = new QHBoxLayout();
    configButtonRow->setContentsMargins(0, 0, 0, 0);
    configButtonRow->setSpacing(8);
    m_ocsCopyConfigButton = new QPushButton("复制配置文本", right);
    configButtonRow->addStretch();
    configButtonRow->addWidget(m_ocsCopyConfigButton);

    QLabel *tip = new QLabel("接口：POST /api/search；健康检查：GET /health。OCS 会把当前题目标题发送到本地服务。", right);
    tip->setWordWrap(true);

    rightLayout->addWidget(configTitle);
    rightLayout->addWidget(m_ocsConfigEdit, 1);
    rightLayout->addLayout(configButtonRow);
    rightLayout->addWidget(tip);
    right->setLayout(rightLayout);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << 420 << 760);

    root->addWidget(splitter, 1);
    m_ocsTab->setLayout(root);

    left->setStyleSheet(
        "QGroupBox { border: 1px solid #dcdcdc; border-radius: 4px; margin-top: 8px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QTextEdit, QLineEdit, QSpinBox, QDoubleSpinBox { border: 1px solid #dcdcdc; border-radius: 4px; padding: 4px; }"
        "QPushButton { padding: 5px 10px; border-radius: 4px; background-color: #007bff; color: white; border: none; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:disabled { background-color: #c8c8c8; color: #666; }"
    );
    right->setStyleSheet(
        "QPlainTextEdit { border: 1px solid #dcdcdc; border-radius: 4px; padding: 6px; font-family: Consolas, monospace; }"
        "QPushButton { padding: 5px 10px; border-radius: 4px; background-color: #007bff; color: white; border: none; }"
        "QPushButton:hover { background-color: #0056b3; }"
    );

    connect(m_ocsStartButton, &QPushButton::clicked, this, &QuestionAssistantWidget::startOcsService);
    connect(m_ocsStopButton, &QPushButton::clicked, this, &QuestionAssistantWidget::stopOcsService);
    connect(m_ocsRefreshIndexButton, &QPushButton::clicked, this, [this]() {
        if (ensureIndexReady(true)) {
            logOcs(QString("题库索引已刷新：%1 题").arg(m_searchIndex->documentCount()));
        }
    });
    connect(m_ocsCopyConfigButton, &QPushButton::clicked, this, [this]() {
        const QString text = buildOcsConfigText();
        QGuiApplication::clipboard()->setText(text);
        logOcs("OCS配置文本已复制到剪贴板");
    });
    connect(m_ocsExportUnmatchedButton, &QPushButton::clicked, this, &QuestionAssistantWidget::exportOcsUnmatchedQuestions);
    connect(m_ocsClearUnmatchedButton, &QPushButton::clicked, this, &QuestionAssistantWidget::clearOcsUnmatchedQuestions);

    auto persistConfig = [this]() {
        if (!m_configManager) {
            updateOcsConfigText();
            return;
        }
        m_configManager->setOcsServiceHost(m_ocsHostEdit->text());
        m_configManager->setOcsServicePort(m_ocsPortSpinBox->value());
        m_configManager->setOcsServiceThreshold(m_ocsThresholdSpinBox->value());
        m_configManager->setOcsServiceTopK(m_ocsTopKSpinBox->value());
        m_configManager->saveConfig();
        updateOcsConfigText();
    };

    connect(m_ocsHostEdit, &QLineEdit::editingFinished, this, persistConfig);
    connect(m_ocsPortSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, persistConfig);
    connect(m_ocsThresholdSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, persistConfig);
    connect(m_ocsTopKSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, persistConfig);

    connect(m_ocsServer, &OcsServer::logMessage, this, &QuestionAssistantWidget::logOcs);
    connect(m_ocsServer, &OcsServer::unmatchedQuestion, this, &QuestionAssistantWidget::recordOcsUnmatchedQuestion);
    connect(m_ocsServer, &OcsServer::runningChanged, this, [this](bool) {
        updateOcsServiceState();
        updateOcsConfigText();
    });

    loadOcsUnmatchedQuestions();
    updateOcsConfigText();
    updateOcsServiceState();
    updateOcsUnmatchedStatus();
}

void QuestionAssistantWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_configManager) {
        ensureIndexReady(false);
    }
}

void QuestionAssistantWidget::log(const QString &msg)
{
    if (m_logEdit) {
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
        m_logEdit->append(QString("[%1] %2").arg(timeStr, msg));
    }
}

void QuestionAssistantWidget::logOcs(const QString &msg)
{
    if (!m_ocsLogEdit) {
        return;
    }
    const QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_ocsLogEdit->append(QString("[%1] %2").arg(timeStr, msg));
    m_ocsLogEdit->moveCursor(QTextCursor::End);
}

bool QuestionAssistantWidget::ensureIndexReady(bool forceRebuild)
{
    if (!forceRebuild && m_searchIndex->isReady()) {
        return true;
    }
    if (!m_configManager) {
        QMessageBox::warning(this, "错误", "ConfigManager 未初始化");
        return false;
    }

    if (!m_searchIndex->buildFromConfig(m_configManager)) {
        QMessageBox::warning(this, "加载题库失败", m_searchIndex->lastError());
        m_indexStatusLabel->setText("题库索引：加载失败");
        return false;
    }

    m_indexStatusLabel->setText(QString("题库索引：已加载 %1 题").arg(m_searchIndex->documentCount()));
    return true;
}

void QuestionAssistantWidget::startOcsService()
{
    if (!m_ocsServer) {
        return;
    }
    if (!ensureIndexReady(false)) {
        logOcs("题库索引加载失败，服务未启动");
        return;
    }

    OcsServerConfig config;
    config.host = m_ocsHostEdit ? m_ocsHostEdit->text().trimmed() : "127.0.0.1";
    if (config.host.isEmpty()) {
        config.host = "127.0.0.1";
    }
    config.port = static_cast<quint16>(m_ocsPortSpinBox ? m_ocsPortSpinBox->value() : 27419);
    config.threshold = m_ocsThresholdSpinBox ? m_ocsThresholdSpinBox->value() : 0.85;
    config.topK = m_ocsTopKSpinBox ? m_ocsTopKSpinBox->value() : 5;

    if (m_configManager) {
        m_configManager->setOcsServiceHost(config.host);
        m_configManager->setOcsServicePort(config.port);
        m_configManager->setOcsServiceThreshold(config.threshold);
        m_configManager->setOcsServiceTopK(config.topK);
        m_configManager->saveConfig();
    }

    if (!m_ocsServer->start(config, m_configManager)) {
        logOcs(QString("服务启动失败：%1").arg(m_ocsServer->lastError()));
        QMessageBox::warning(this, "OCS服务启动失败", m_ocsServer->lastError());
        updateOcsServiceState();
        return;
    }

    updateOcsServiceState();
    updateOcsConfigText();
}

void QuestionAssistantWidget::stopOcsService()
{
    if (m_ocsServer) {
        m_ocsServer->stop();
    }
    updateOcsServiceState();
}

void QuestionAssistantWidget::updateOcsServiceState()
{
    const bool running = m_ocsServer && m_ocsServer->isRunning();
    if (m_ocsStatusLabel) {
        m_ocsStatusLabel->setText(running ? "状态：运行中" : "状态：未启动");
    }
    if (m_ocsUrlLabel) {
        m_ocsUrlLabel->setText(running && m_ocsServer ? QString("地址：%1").arg(m_ocsServer->baseUrl()) : "地址：-");
    }
    if (m_ocsStartButton) {
        m_ocsStartButton->setEnabled(!running);
    }
    if (m_ocsStopButton) {
        m_ocsStopButton->setEnabled(running);
    }
    if (m_ocsHostEdit) {
        m_ocsHostEdit->setEnabled(!running);
    }
    if (m_ocsPortSpinBox) {
        m_ocsPortSpinBox->setEnabled(!running);
    }
}

void QuestionAssistantWidget::updateOcsConfigText()
{
    if (m_ocsConfigEdit) {
        m_ocsConfigEdit->setPlainText(buildOcsConfigText());
    }
}

QString QuestionAssistantWidget::buildOcsConfigText() const
{
    const QString host = m_ocsHostEdit && !m_ocsHostEdit->text().trimmed().isEmpty()
        ? m_ocsHostEdit->text().trimmed()
        : QStringLiteral("127.0.0.1");
    const int port = m_ocsPortSpinBox ? m_ocsPortSpinBox->value() : 27419;
    const double threshold = m_ocsThresholdSpinBox ? m_ocsThresholdSpinBox->value() : 0.85;
    const int topK = m_ocsTopKSpinBox ? m_ocsTopKSpinBox->value() : 5;
    const QString url = QString("http://%1:%2/api/search").arg(host).arg(port);

    QJsonObject titleHandler;
    titleHandler["handler"] = "return (env) => env.title || env.question || ''";

    QJsonObject optionsHandler;
    optionsHandler["handler"] = "return (env) => Array.isArray(env.options) ? env.options : []";

    QJsonObject typeHandler;
    typeHandler["handler"] = "return (env) => env.type || 'auto'";

    QJsonObject data;
    data["title"] = titleHandler;
    data["options"] = optionsHandler;
    data["type"] = typeHandler;

    QJsonObject headers;
    headers["Content-Type"] = "application/json";

    const QString handler =
        "return (res) => {\n"
        "  const list = Array.isArray(res?.data) ? res.data : [];\n"
        "  return list.map((item) => [\n"
        "    item.question || '',\n"
        "    item.answer || '',\n"
        "    {\n"
        "      score: item.score,\n"
        "      subject: item.subject,\n"
        "      bank: item.bank,\n"
        "      source: item.source,\n"
        "      answers: item.answers\n"
        "    }\n"
        "  ]);\n"
        "}";

    QJsonObject wrapper;
    wrapper["name"] = "ProblemX 本地题库";
    wrapper["homepage"] = "http://127.0.0.1";
    wrapper["url"] = url;
    wrapper["method"] = "post";
    wrapper["type"] = "GM_xmlhttpRequest";
    wrapper["contentType"] = "json";
    wrapper["headers"] = headers;
    wrapper["data"] = data;
    wrapper["handler"] = handler;

    QJsonObject extra;
    extra["threshold"] = threshold;
    extra["topK"] = topK;
    wrapper["extra"] = extra;

    QJsonArray root;
    root.append(wrapper);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString QuestionAssistantWidget::ocsUnmatchedCachePath() const
{
    return "ocs_unmatched_questions.json";
}

QString QuestionAssistantWidget::makeOcsUnmatchedKey(const QString &title, const QStringList &options, const QString &type) const
{
    QString key = title.trimmed();
    if (!options.isEmpty()) {
        key.append("\n");
        key.append(options.join("\n"));
    }
    key.append("\n");
    key.append(type.trimmed());
    return key;
}

static QString normalizeOcsUnmatchedExportType(const QString &type)
{
    const QString trimmed = type.trimmed();
    if (trimmed == "Choice" || trimmed == "MultipleChoice" || trimmed == "FillBlank" || trimmed == "TrueorFalse" || trimmed == "TrueOrFalse") {
        return trimmed == "TrueOrFalse" ? "TrueorFalse" : trimmed;
    }

    const QString lower = trimmed.toLower();
    if (lower.contains("fill") || lower.contains("blank") || lower.contains("completion")) {
        return "FillBlank";
    }
    if (lower.contains("multi") || lower.contains("multiple")) {
        return "MultipleChoice";
    }
    if (lower.contains("judge") || lower.contains("true") || lower.contains("false") || lower.contains("boolean")) {
        return "TrueorFalse";
    }
    return "Choice";
}

void QuestionAssistantWidget::loadOcsUnmatchedQuestions()
{
    m_ocsUnmatchedQuestions.clear();

    QFile file(ocsUnmatchedCachePath());
    if (!file.exists()) {
        updateOcsUnmatchedStatus();
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        logOcs("未命中题目记录读取失败");
        updateOcsUnmatchedStatus();
        return;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        logOcs("未命中题目记录格式无效，已忽略");
        updateOcsUnmatchedStatus();
        return;
    }

    const QJsonArray data = doc.object().value("data").toArray();
    for (const QJsonValue &value : data) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        OcsUnmatchedQuestion item;
        item.title = object.value("question").toString().trimmed();
        item.type = object.value("_ocs_type").toString().trimmed();
        item.bestScore = object.value("_ocs_best_score").toDouble(0.0);
        item.threshold = object.value("_ocs_threshold").toDouble(0.0);
        item.count = qMax(1, object.value("_ocs_count").toInt(1));
        item.firstSeen = object.value("_ocs_first_seen").toString();
        item.lastSeen = object.value("_ocs_last_seen").toString();

        const QJsonArray choices = object.value("choices").toArray();
        for (const QJsonValue &choice : choices) {
            const QString text = choice.toString().trimmed();
            if (!text.isEmpty()) {
                item.options.append(text);
            }
        }

        if (item.title.isEmpty()) {
            continue;
        }
        m_ocsUnmatchedQuestions.insert(makeOcsUnmatchedKey(item.title, item.options, item.type), item);
    }

    updateOcsUnmatchedStatus();
}

void QuestionAssistantWidget::saveOcsUnmatchedQuestions() const
{
    QJsonArray data;
    for (auto it = m_ocsUnmatchedQuestions.constBegin(); it != m_ocsUnmatchedQuestions.constEnd(); ++it) {
        const OcsUnmatchedQuestion item = it.value();
        QJsonObject object;
        object["type"] = normalizeOcsUnmatchedExportType(item.type);
        object["question"] = item.title;

        if (!item.options.isEmpty()) {
            QJsonArray choices;
            for (const QString &option : item.options) {
                choices.append(option);
            }
            object["choices"] = choices;
        }

        object["answer"] = QJsonValue::Null;
        object["_ocs_type"] = item.type;
        object["_ocs_best_score"] = item.bestScore;
        object["_ocs_threshold"] = item.threshold;
        object["_ocs_count"] = item.count;
        object["_ocs_first_seen"] = item.firstSeen;
        object["_ocs_last_seen"] = item.lastSeen;
        data.append(object);
    }

    QJsonObject root;
    root["data"] = data;
    root["_source"] = "ProblemX OCS service unmatched questions";
    root["_saved_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(ocsUnmatchedCachePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
}

void QuestionAssistantWidget::recordOcsUnmatchedQuestion(const QString &title, const QStringList &options, const QString &type, double bestScore, double threshold)
{
    const QString cleanTitle = title.trimmed();
    if (cleanTitle.isEmpty()) {
        return;
    }

    const QString key = makeOcsUnmatchedKey(cleanTitle, options, type);
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    OcsUnmatchedQuestion item = m_ocsUnmatchedQuestions.value(key);
    if (item.title.isEmpty()) {
        item.title = cleanTitle;
        item.options = options;
        item.type = type.trimmed();
        item.firstSeen = now;
        item.count = 0;
    }
    item.bestScore = bestScore;
    item.threshold = threshold;
    item.lastSeen = now;
    item.count += 1;
    m_ocsUnmatchedQuestions.insert(key, item);

    saveOcsUnmatchedQuestions();
    updateOcsUnmatchedStatus();
    logOcs(QString("已记录未命中题目：%1（累计 %2 次）")
           .arg(cleanTitle.left(36))
           .arg(item.count));
}

void QuestionAssistantWidget::exportOcsUnmatchedQuestions()
{
    if (m_ocsUnmatchedQuestions.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可导出的未命中题目");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "导出OCS未命中题目", "ocs_unmatched_questions.json", "JSON (*.json)");
    if (filePath.isEmpty()) {
        return;
    }

    QJsonArray data;
    for (auto it = m_ocsUnmatchedQuestions.constBegin(); it != m_ocsUnmatchedQuestions.constEnd(); ++it) {
        const OcsUnmatchedQuestion item = it.value();
        QJsonObject object;
        object["type"] = normalizeOcsUnmatchedExportType(item.type);
        object["question"] = item.title;
        if (!item.options.isEmpty()) {
            QJsonArray choices;
            for (const QString &option : item.options) {
                choices.append(option);
            }
            object["choices"] = choices;
        }
        object["answer"] = QJsonValue::Null;
        object["_ocs_type"] = item.type;
        object["_ocs_best_score"] = item.bestScore;
        object["_ocs_threshold"] = item.threshold;
        object["_ocs_count"] = item.count;
        object["_ocs_first_seen"] = item.firstSeen;
        object["_ocs_last_seen"] = item.lastSeen;
        data.append(object);
    }

    QJsonObject root;
    root["data"] = data;
    root["_source"] = "ProblemX OCS service unmatched questions";
    root["_exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件");
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "导出成功", QString("已导出 %1 题：\n%2").arg(data.size()).arg(filePath));
}

void QuestionAssistantWidget::clearOcsUnmatchedQuestions()
{
    if (m_ocsUnmatchedQuestions.isEmpty()) {
        updateOcsUnmatchedStatus();
        return;
    }

    const QMessageBox::StandardButton ret = QMessageBox::question(
        this,
        "清空未命中题目",
        "确定要清空当前记录的 OCS 未命中题目吗？",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    m_ocsUnmatchedQuestions.clear();
    saveOcsUnmatchedQuestions();
    updateOcsUnmatchedStatus();
    logOcs("未命中题目记录已清空");
}

void QuestionAssistantWidget::updateOcsUnmatchedStatus()
{
    if (m_ocsUnmatchedLabel) {
        int totalCount = 0;
        for (auto it = m_ocsUnmatchedQuestions.constBegin(); it != m_ocsUnmatchedQuestions.constEnd(); ++it) {
            totalCount += qMax(1, it.value().count);
        }
        m_ocsUnmatchedLabel->setText(QString("未命中题目：%1 题 / %2 次").arg(m_ocsUnmatchedQuestions.size()).arg(totalCount));
    }
    const bool hasData = !m_ocsUnmatchedQuestions.isEmpty();
    if (m_ocsExportUnmatchedButton) {
        m_ocsExportUnmatchedButton->setEnabled(hasData);
    }
    if (m_ocsClearUnmatchedButton) {
        m_ocsClearUnmatchedButton->setEnabled(hasData);
    }
}

void QuestionAssistantWidget::updatePtaQuestionItemVisual(const QString &ptaId)
{
    for (int i = 0; i < m_ptaQuestionList->count(); ++i) {
        QListWidgetItem *item = m_ptaQuestionList->item(i);
        if (!item) continue;
        if (item->data(Qt::UserRole).toString() != ptaId) continue;

        const PtaCacheEntry entry = m_ptaCache.value(ptaId);
        if (entry.filled) {
            item->setBackground(QColor("#D4EDDA"));
            item->setToolTip("已作答");
        } else if (entry.noMatch) {
            item->setBackground(QColor("#F8D7DA"));
            item->setToolTip(QString("新题/未匹配（best=%1）").arg(QString::number(entry.bestScore, 'f', 3)));
        } else {
            item->setBackground(QColor("#FFF3CD"));
            item->setToolTip("未作答");
        }
        break;
    }
}

QString QuestionAssistantWidget::buildPtaQueryText(const ParsedPtaQuestion &ptaQuestion) const
{
    QString query = ptaQuestion.question;
    if (!ptaQuestion.choices.isEmpty()) {
        query.append("\n");
        query.append(ptaQuestion.choices.join("\n"));
    }
    return query;
}

void QuestionAssistantWidget::startPtaAutoAnswer()
{
    if (m_ptaAutoRunning) {
        return;
    }
    if (!ensureIndexReady(false)) {
        return;
    }
    if (!m_ptaQuestionList || m_ptaQuestionList->count() == 0) {
        QMessageBox::information(this, "提示", "请先解析题目列表");
        return;
    }

    m_ptaAutoQueue.clear();
    m_ptaAutoQueue.reserve(m_ptaQuestionList->count());
    for (int i = 0; i < m_ptaQuestionList->count(); ++i) {
        QListWidgetItem *item = m_ptaQuestionList->item(i);
        if (!item) continue;
        const QString id = item->data(Qt::UserRole).toString();
        if (!id.isEmpty()) {
            m_ptaAutoQueue.append(id);
        }
    }
    if (m_ptaAutoQueue.isEmpty()) {
        QMessageBox::information(this, "提示", "题目列表为空");
        return;
    }

    m_ptaAutoRunning = true;
    m_ptaAutoPos = 0;
    if (m_ptaAutoAnswerButton) m_ptaAutoAnswerButton->setEnabled(false);
    if (m_ptaStopAutoButton) m_ptaStopAutoButton->setEnabled(true);
    
    log("开始自动答题...");
    processNextPtaAuto();
}

void QuestionAssistantWidget::stopPtaAutoAnswer()
{
    m_ptaAutoRunning = false;
    m_ptaAutoQueue.clear();
    m_ptaAutoPos = 0;
    if (m_ptaAutoAnswerButton) m_ptaAutoAnswerButton->setEnabled(true);
    if (m_ptaStopAutoButton) m_ptaStopAutoButton->setEnabled(false);
    log("自动答题已停止");
}

void QuestionAssistantWidget::processNextPtaAuto()
{
    if (!m_ptaAutoRunning) {
        stopPtaAutoAnswer();
        return;
    }
    if (m_ptaAutoPos < 0 || m_ptaAutoPos >= m_ptaAutoQueue.size()) {
        stopPtaAutoAnswer();
        return;
    }

    const QString id = m_ptaAutoQueue[m_ptaAutoPos];
    if (!m_ptaQuestions.contains(id)) {
        m_ptaAutoPos++;
        QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
        return;
    }

    PtaCacheEntry entry = m_ptaCache.value(id);
    if (entry.filled) {
        m_ptaAutoPos++;
        QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
        return;
    }

    const ParsedPtaQuestion pq = m_ptaQuestions.value(id);
    const QString query = buildPtaQueryText(pq);
    const int k = m_ptaTopKSpinBox ? m_ptaTopKSpinBox->value() : 5;
    const double threshold = m_ptaThresholdSpinBox ? m_ptaThresholdSpinBox->value() : 0.85;

    const QVector<SearchHit> hits = m_searchIndex->searchTopK(query, k);
    entry.hits = hits;
    entry.bestScore = hits.isEmpty() ? 0.0 : hits.first().score;

    if (hits.isEmpty() || hits.first().score < threshold) {
        log(QString("题目 %1 未匹配 (best=%2)").arg(id, QString::number(entry.bestScore, 'f', 2)));
        entry.selectedDocIndex = hits.isEmpty() ? -1 : hits.first().docIndex;
        entry.noMatch = true;
        entry.filled = false;
        m_ptaCache.insert(id, entry);
        updatePtaQuestionItemVisual(id);
        m_ptaAutoPos++;
        QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
        return;
    }

    log(QString("题目 %1 匹配成功 (score=%2)，正在填入...").arg(id, QString::number(hits.first().score, 'f', 2)));
    entry.selectedDocIndex = hits.first().docIndex;
    entry.noMatch = false;
    m_ptaCache.insert(id, entry);
    updatePtaQuestionItemVisual(id);

    if (!id.startsWith("idx_")) {
        const Question &bankQuestion = m_searchIndex->documentQuestion(entry.selectedDocIndex);
        m_ptaController->fillFromBankQuestion(id, bankQuestion);
    } else {
        log(QString("题目 %1 缺少ID，跳过填入").arg(id));
        entry.noMatch = true;
        m_ptaCache.insert(id, entry);
        updatePtaQuestionItemVisual(id);
        m_ptaAutoPos++;
        QTimer::singleShot(0, this, &QuestionAssistantWidget::processNextPtaAuto);
    }
}

void QuestionAssistantWidget::exportPtaNewQuestions()
{
    if (m_ptaQuestions.isEmpty()) {
        QMessageBox::information(this, "提示", "请先解析题目列表");
        return;
    }

    QJsonArray data;
    for (auto it = m_ptaQuestions.constBegin(); it != m_ptaQuestions.constEnd(); ++it) {
        const QString id = it.key();
        const ParsedPtaQuestion pq = it.value();
        const PtaCacheEntry entry = m_ptaCache.value(id);
        if (!entry.noMatch) {
            continue;
        }

        QJsonObject o;
        o["type"] = pq.type;
        o["question"] = pq.question;

        if (!pq.choices.isEmpty()) {
            QJsonArray arr;
            for (const QString &c : pq.choices) {
                arr.append(c);
            }
            o["choices"] = arr;
        }

        if (pq.type == "FillBlank") {
            o["BlankNum"] = pq.blankNum;
        }

        o["answer"] = QJsonValue::Null;

        if (!pq.images.isEmpty()) {
            QJsonObject img;
            for (auto it2 = pq.images.constBegin(); it2 != pq.images.constEnd(); ++it2) {
                img.insert(it2.key(), it2.value());
            }
            o["image"] = img;
        }

        o["_pta_id"] = id;
        o["_pta_label"] = pq.label;
        o["_pta_best_score"] = entry.bestScore;
        data.append(o);
    }

    if (data.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可导出的新题（未匹配题目）");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this, "导出新题题库", "pta_new_questions.json", "JSON (*.json)");
    if (filePath.isEmpty()) {
        return;
    }

    QJsonObject root;
    root["data"] = data;
    const QJsonDocument doc(root);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件");
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    QMessageBox::information(this, "导出成功", QString("已导出 %1 题：\n%2").arg(data.size()).arg(filePath));
}
