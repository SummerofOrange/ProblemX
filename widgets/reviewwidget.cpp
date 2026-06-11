#include "reviewwidget.h"
#include "ui_reviewwidget.h"
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
    , ui(new Ui::ReviewWidget)
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
    delete ui;
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
    ui->setupUi(this);

    m_mainSplitter = ui->mainSplitter;
    m_leftPanel = ui->leftPanel;
    m_leftLayout = ui->leftLayout;
    m_filterGroup = ui->filterGroup;
    m_filterLayout = ui->filterLayout;
    m_subjectLabel = ui->subjectLabel;
    m_subjectCombo = ui->subjectCombo;
    m_typeLabel = ui->typeLabel;
    m_typeCombo = ui->typeCombo;
    m_dateLabel = ui->dateLabel;
    m_dateFromEdit = ui->dateFromEdit;
    m_dateToEdit = ui->dateToEdit;
    m_showResolvedCheck = ui->showResolvedCheck;
    m_sortLabel = ui->sortLabel;
    m_sortCombo = ui->sortCombo;
    m_refreshButton = ui->refreshButton;
    m_statisticsGroup = ui->statisticsGroup;
    m_statisticsLayout = ui->statisticsLayout;
    m_totalLabel = ui->totalLabel;
    m_unresolvedLabel = ui->unresolvedLabel;
    m_resolvedLabel = ui->resolvedLabel;
    m_selectedLabel = ui->selectedLabel;
    m_resolvedProgressBar = ui->resolvedProgressBar;
    m_listGroup = ui->listGroup;
    m_listLayout = ui->listLayout;
    m_wrongAnswersList = ui->wrongAnswersList;
    m_selectionLayout = ui->selectionLayout;
    m_selectAllButton = ui->selectAllButton;
    m_selectNoneButton = ui->selectNoneButton;
    m_actionLayout = ui->actionLayout;
    m_startReviewButton = ui->startReviewButton;
    m_markResolvedButton = ui->markResolvedButton;
    m_markUnresolvedButton = ui->markUnresolvedButton;
    m_deleteSelectedButton = ui->deleteSelectedButton;
    m_clearResolvedButton = ui->clearResolvedButton;
    m_importExportLayout = ui->importExportLayout;
    m_exportButton = ui->exportButton;
    m_importButton = ui->importButton;
    m_controlLayout = ui->controlLayout;
    m_backButton = ui->backButton;
    m_rightPanel = ui->rightPanel;
    m_rightLayout = ui->rightLayout;
    m_detailsGroup = ui->detailsGroup;
    m_detailsLayout = ui->detailsLayout;
    m_detailsScrollArea = ui->detailsScrollArea;
    m_detailsContent = ui->detailsContent;
    m_detailsContentLayout = ui->detailsContentLayout;
    m_detailSubjectLabel = ui->detailSubjectLabel;
    m_detailTypeLabel = ui->detailTypeLabel;
    m_detailQuestionRenderer = ui->detailQuestionRenderer;
    m_detailImageLabel = ui->detailImageLabel;
    m_detailChoicesRenderer = ui->detailChoicesRenderer;
    m_detailCorrectAnswerRenderer = ui->detailCorrectAnswerRenderer;
    m_detailUserAnswerRenderer = ui->detailUserAnswerRenderer;
    m_detailTimestampLabel = ui->detailTimestampLabel;
    m_detailReviewCountLabel = ui->detailReviewCountLabel;
    m_detailStatusLabel = ui->detailStatusLabel;

    m_subjectCombo->addItem("全部科目", "");
    m_typeCombo->addItem("全部题型", "");
    m_typeCombo->addItem("选择题", "Choice");
    m_typeCombo->addItem("判断题", "TrueOrFalse");
    m_typeCombo->addItem("填空题", "FillBlank");
    m_typeCombo->addItem("多选题", "MultiChoice");
    m_sortCombo->addItem("时间降序", "time_desc");
    m_sortCombo->addItem("时间升序", "time_asc");
    m_sortCombo->addItem("复习次数降序", "review_desc");
    m_sortCombo->addItem("复习次数升序", "review_asc");

    m_dateFromEdit->setDate(QDate::currentDate().addDays(-30));
    m_dateToEdit->setDate(QDate::currentDate());

    m_refreshButton->setObjectName("refreshButton");
    m_startReviewButton->setObjectName("startReviewButton");
    m_deleteSelectedButton->setObjectName("deleteButton");
    m_clearResolvedButton->setObjectName("clearButton");
    m_backButton->setObjectName("backButton");
    m_detailSubjectLabel->setObjectName("detailSubject");
    m_detailTypeLabel->setObjectName("detailType");
    m_detailQuestionRenderer->setObjectName("detailQuestion");
    m_detailChoicesRenderer->setObjectName("detailChoices");
    m_detailCorrectAnswerRenderer->setObjectName("detailCorrectAnswer");
    m_detailUserAnswerRenderer->setObjectName("detailUserAnswer");
    m_detailTimestampLabel->setObjectName("detailTimestamp");
    m_detailReviewCountLabel->setObjectName("detailReviewCount");
    m_detailStatusLabel->setObjectName("detailStatus");

    m_detailQuestionRenderer->setAutoResize(true, 400);
    m_detailChoicesRenderer->setAutoResize(true, 300);
    m_detailCorrectAnswerRenderer->setAutoResize(true, 200);
    m_detailUserAnswerRenderer->setAutoResize(true, 200);

    m_mainSplitter->setSizes({400, 600});
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

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
