#include "configwidget.h"
#include "bankeditorwidget.h"
#include "ptaimportdialog.h"
#include "../core/configmanager.h"
#include "../models/questionbank.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QShowEvent>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QRegularExpression>
#include <QSignalBlocker>

ConfigWidget::ConfigWidget(QWidget *parent)
    : QWidget(parent)
    , m_configManager(nullptr)
    , m_isLoading(false)
    , m_stackedWidget(nullptr)
    , m_bankEditorWidget(nullptr)
    , m_currentBankIndex(-1)
{
    setupUI();
    setupConnections();
    applyStyles();
}

ConfigWidget::~ConfigWidget()
{
}

void ConfigWidget::setConfigManager(ConfigManager *configManager)
{
    m_configManager = configManager;
    refreshData();
}

void ConfigWidget::refreshData()
{
    if (!m_configManager) {
        return;
    }
    
    m_isLoading = true;
    loadSubjects();
    updateStatistics();
    if (m_shuffleQuestionsCheckBox) {
        m_shuffleQuestionsCheckBox->setChecked(m_configManager->isShuffleQuestionsEnabled());
    }
    m_isLoading = false;
}

void ConfigWidget::setupUI()
{
    // Create stacked widget for main content and bank editor
    m_stackedWidget = new QStackedWidget(this);
    
    // Create main config widget
    QWidget *mainConfigWidget = new QWidget();
    
    // Create main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, mainConfigWidget);
    
    // Create left panel (subject tree)
    m_leftPanel = new QWidget();
    m_leftLayout = new QVBoxLayout(m_leftPanel);
    m_leftLayout->setContentsMargins(10, 10, 10, 10);
    m_leftLayout->setSpacing(10);
    
    // Subject tree
    m_subjectLabel = new QLabel("科目和题库");
    m_subjectLabel->setObjectName("sectionLabel");
    
    m_subjectTree = new QTreeWidget();
    m_subjectTree->setHeaderLabels(QStringList() << "名称" << "状态" << "题目数");
    m_subjectTree->setRootIsDecorated(true);
    m_subjectTree->setAlternatingRowColors(true);
    m_subjectTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_subjectTree->header()->setStretchLastSection(false);
    m_subjectTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_subjectTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_subjectTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    // Subject buttons
    m_subjectButtonLayout = new QHBoxLayout();
    m_addSubjectButton = new QPushButton("添加科目");
    m_createSubjectButton = new QPushButton("创建科目");
    m_removeSubjectButton = new QPushButton("删除科目");
    m_removeBankButton = new QPushButton("删除题库"); // 新增删除题库按钮
    
    m_subjectButtonLayout->addWidget(m_addSubjectButton);
    m_subjectButtonLayout->addWidget(m_createSubjectButton);
    m_subjectButtonLayout->addWidget(m_removeSubjectButton);
    m_subjectButtonLayout->addWidget(m_removeBankButton); // 添加到布局
    m_subjectButtonLayout->addStretch();
    
    m_leftLayout->addWidget(m_subjectLabel);
    m_leftLayout->addWidget(m_subjectTree);
    m_leftLayout->addLayout(m_subjectButtonLayout);
    
    // Create right panel (details)
    m_rightPanel = new QWidget();
    m_rightLayout = new QVBoxLayout(m_rightPanel);
    m_rightLayout->setContentsMargins(10, 10, 10, 10);
    m_rightLayout->setSpacing(15);
    
    // Subject Info Group
    m_subjectInfoGroup = new QGroupBox("科目信息");
    m_subjectInfoLayout = new QGridLayout(m_subjectInfoGroup);
    
    m_subjectNameLabel = new QLabel("科目名称:");
    m_subjectNameEdit = new QLineEdit();
    m_subjectNameEdit->setReadOnly(true);
    
    m_subjectPathLabel = new QLabel("题库路径:");
    m_subjectPathEdit = new QLineEdit();
    m_subjectPathEdit->setReadOnly(true);
    m_browsePathButton = new QPushButton("浏览");
    
    m_subjectInfoLayout->addWidget(m_subjectNameLabel, 0, 0);
    m_subjectInfoLayout->addWidget(m_subjectNameEdit, 0, 1, 1, 2);
    m_subjectInfoLayout->addWidget(m_subjectPathLabel, 1, 0);
    m_subjectInfoLayout->addWidget(m_subjectPathEdit, 1, 1);
    m_subjectInfoLayout->addWidget(m_browsePathButton, 1, 2);

    // Subject Actions Group
    m_subjectActionsGroup = new QGroupBox("题库操作");
    m_subjectActionsLayout = new QHBoxLayout(m_subjectActionsGroup);
    m_createBankButton = new QPushButton("创建题库");
    m_autoFetchBankButton = new QPushButton("自动获取题库");
    m_subjectActionsLayout->addWidget(m_createBankButton);
    m_subjectActionsLayout->addWidget(m_autoFetchBankButton);
    m_subjectActionsLayout->addStretch();
    
    // Bank Details Group
    m_bankDetailsGroup = new QGroupBox("题库详情");
    m_bankDetailsLayout = new QGridLayout(m_bankDetailsGroup);
    
    m_bankNameLabel = new QLabel("题库名称: --");
    m_bankTypeLabel = new QLabel("题库类型: --");
    m_totalQuestionsLabel = new QLabel("题目总数: --");
    m_bankStatusLabel = new QLabel("状态: --");
    
    m_extractCountLabel = new QLabel("抽取数量:");
    m_extractCountSpinBox = new QSpinBox();
    m_extractCountSpinBox->setMinimum(1);
    m_extractCountSpinBox->setMaximum(9999);
    m_extractCountSpinBox->setMinimumWidth(110);
    m_extractCountSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_extractPercentLabel = new QLabel("(0%)");
    m_extractCountSlider = new QSlider(Qt::Horizontal);
    m_extractCountSlider->setMinimum(1);
    m_extractCountSlider->setMaximum(9999);
    m_extractCountSlider->setValue(1);
    m_extractCountSlider->setEnabled(false);
    m_extractCountSlider->setMaximumWidth(550);
    m_extractCountSlider->setMinimumWidth(450);
    m_extractPercentLabel->setVisible(false);
    
    m_bankDetailsLayout->addWidget(m_bankNameLabel, 0, 0, 1, 3);
    m_bankDetailsLayout->addWidget(m_bankTypeLabel, 1, 0, 1, 3);
    m_bankDetailsLayout->addWidget(m_totalQuestionsLabel, 2, 0, 1, 3);
    m_bankDetailsLayout->addWidget(m_bankStatusLabel, 3, 0, 1, 3);

    QHBoxLayout *extractCountRowLayout = new QHBoxLayout();
    extractCountRowLayout->addWidget(m_extractCountLabel);
    extractCountRowLayout->addWidget(m_extractCountSlider);
    extractCountRowLayout->addWidget(m_extractCountSpinBox);
    m_bankDetailsLayout->addLayout(extractCountRowLayout, 4, 0, 1, 3);
    
    // Add edit bank button
    m_editBankButton = new QPushButton("编辑题库");
    m_editBankButton->setObjectName("editBankButton");
    m_bankDetailsLayout->addWidget(m_editBankButton, 5, 0, 1, 3);
    
    // Statistics Group
    m_statisticsGroup = new QGroupBox("统计信息");
    m_statisticsLayout = new QGridLayout(m_statisticsGroup);
    
    m_totalSubjectsLabel = new QLabel("总科目数: 0");
    m_totalBanksLabel = new QLabel("总题库数: 0");
    m_enabledBanksLabel = new QLabel("已启用: 0");
    m_statisticsQuestionsLabel = new QLabel("总题目数: 0");
    m_shuffleQuestionsCheckBox = new QCheckBox("乱序题目");
    
    m_configProgressBar = new QProgressBar();
    m_configProgressBar->setTextVisible(true);
    m_configProgressBar->setFormat("配置完成度: %p%");
    
    m_statisticsLayout->addWidget(m_totalSubjectsLabel, 0, 0);
    m_statisticsLayout->addWidget(m_totalBanksLabel, 0, 1);
    m_statisticsLayout->addWidget(m_enabledBanksLabel, 1, 0);
    m_statisticsLayout->addWidget(m_statisticsQuestionsLabel, 1, 1);
    m_statisticsLayout->addWidget(m_shuffleQuestionsCheckBox, 2, 0, 1, 2);
    m_statisticsLayout->addWidget(m_configProgressBar, 3, 0, 1, 2);
    
    // Description Group
    m_descriptionGroup = new QGroupBox("说明");
    m_descriptionLayout = new QVBoxLayout(m_descriptionGroup);
    
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(100);
    m_descriptionEdit->setPlainText(
        "配置说明:\n"
        "1. 选择科目查看其包含的题库\n"
        "2. 选择题库可以启用/禁用和设置抽取数量\n"
        "3. 抽取数量不能超过题库总题目数\n"
        "4. 至少启用一个题库才能开始练习"
    );
    m_descriptionEdit->setReadOnly(true);
    
    m_descriptionLayout->addWidget(m_descriptionEdit);
    
    // Add groups to right layout
    m_rightLayout->addWidget(m_subjectInfoGroup);
    m_rightLayout->addWidget(m_subjectActionsGroup);
    m_rightLayout->addWidget(m_bankDetailsGroup);
    m_rightLayout->addWidget(m_statisticsGroup);
    m_rightLayout->addWidget(m_descriptionGroup);
    m_rightLayout->addStretch();
    
    // 初始状态下隐藏题库详情组件
    m_bankDetailsGroup->setVisible(false);
    m_subjectActionsGroup->setVisible(false);
    
    // Bottom buttons
    m_bottomButtonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("保存配置");
    m_backButton = new QPushButton("返回");
    
    m_saveButton->setObjectName("primaryButton");
    m_backButton->setObjectName("secondaryButton");
    
    m_bottomButtonLayout->addStretch();
    m_bottomButtonLayout->addWidget(m_saveButton);
    m_bottomButtonLayout->addWidget(m_backButton);
    
    m_rightLayout->addLayout(m_bottomButtonLayout);
    
    // Add panels to splitter
    m_mainSplitter->addWidget(m_leftPanel);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 2);
    
    // Main layout for config widget
    QHBoxLayout *mainConfigLayout = new QHBoxLayout(mainConfigWidget);
    mainConfigLayout->setContentsMargins(0, 0, 0, 0);
    mainConfigLayout->addWidget(m_mainSplitter);
    
    // Create bank editor widget
    m_bankEditorWidget = new BankEditorWidget();
    
    // Add widgets to stacked widget
    m_stackedWidget->addWidget(mainConfigWidget);
    m_stackedWidget->addWidget(m_bankEditorWidget);
    
    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_stackedWidget);
    
    setLayout(mainLayout);
}

void ConfigWidget::setupConnections()
{
    connect(m_subjectTree, &QTreeWidget::currentItemChanged,
            this, &ConfigWidget::onSubjectSelectionChanged);
    
    // 添加题库选择变化的信号连接 - 也连接到onSubjectSelectionChanged
    connect(m_subjectTree, &QTreeWidget::itemClicked,
            this, &ConfigWidget::onSubjectSelectionChanged);

    connect(m_subjectTree, &QTreeWidget::itemChanged,
            this, &ConfigWidget::onBankCheckStateChanged);
    
    connect(m_extractCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConfigWidget::onExtractCountChanged);

    connect(m_extractCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                if (!m_extractCountSlider) {
                    return;
                }
                if (m_extractCountSlider->value() == value) {
                    return;
                }
                const QSignalBlocker blocker(m_extractCountSlider);
                m_extractCountSlider->setValue(value);
            });

    connect(m_extractCountSlider, &QSlider::valueChanged,
            this, [this](int value) {
                if (!m_extractCountSpinBox) {
                    return;
                }
                if (m_extractCountSpinBox->value() == value) {
                    return;
                }
                const QSignalBlocker blocker(m_extractCountSpinBox);
                m_extractCountSpinBox->setValue(value);
                onExtractCountChanged(value);
            });
    
    connect(m_addSubjectButton, &QPushButton::clicked,
            this, &ConfigWidget::onAddSubjectClicked);
    
    connect(m_createSubjectButton, &QPushButton::clicked,
            this, &ConfigWidget::onCreateSubjectClicked);
    
    connect(m_removeSubjectButton, &QPushButton::clicked,
            this, &ConfigWidget::onRemoveSubjectClicked);
    
    connect(m_removeBankButton, &QPushButton::clicked,
            this, &ConfigWidget::onRemoveBankClicked);
    
    connect(m_saveButton, &QPushButton::clicked,
            this, &ConfigWidget::onSaveClicked);
    
    connect(m_editBankButton, &QPushButton::clicked,
            this, &ConfigWidget::onEditBankClicked);
    
    connect(m_bankEditorWidget, &BankEditorWidget::backRequested,
            this, &ConfigWidget::onBankEditorBack);
    
    connect(m_backButton, &QPushButton::clicked,
            this, &ConfigWidget::onBackClicked);

    connect(m_createBankButton, &QPushButton::clicked,
            this, &ConfigWidget::onCreateBankClicked);

    connect(m_autoFetchBankButton, &QPushButton::clicked,
            this, &ConfigWidget::onAutoFetchBankClicked);

    connect(m_shuffleQuestionsCheckBox, &QCheckBox::toggled,
            this, [this](bool enabled) {
                if (m_isLoading || !m_configManager) {
                    return;
                }
                m_configManager->setShuffleQuestionsEnabled(enabled);
                m_configManager->saveConfig();
                emit configurationChanged();
            });
}

void ConfigWidget::applyStyles()
{
    setStyleSheet(
        "ConfigWidget {"
        "    background-color: #f8f9fa;"
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
        
        "#sectionLabel {"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    padding: 5px 0;"
        "}"
        
        "QTreeWidget {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "    alternate-background-color: #f8f9fa;"
        "    selection-background-color: #4A90E2;"
        "}"
        
        "QTreeWidget::item {"
        "    padding: 4px;"
        "    border-bottom: 1px solid #e9ecef;"
        "}"
        
        "QTreeWidget::item:selected {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "}"
        
        "QTreeWidget::item:hover {"
        "    background-color: #e3f2fd;"
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
        
        "#primaryButton {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
        
        "#primaryButton:hover {"
        "    background-color: #0056b3;"
        "}"
        
        "#editBankButton {"
        "    background-color: #17a2b8;"
        "    color: white;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
        
        "#editBankButton:hover {"
        "    background-color: #138496;"
        "}"
        
        "#secondaryButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "}"
        
        "#secondaryButton:hover {"
        "    background-color: #545b62;"
        "}"
        
        "QLineEdit, QSpinBox {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    background-color: white;"
        "}"
        
        "QLineEdit:focus, QSpinBox:focus {"
        "    border-color: #4A90E2;"
        "    outline: none;"
        "}"
        
        "QTextEdit {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    background-color: #f8f9fa;"
        "    font-size: 12px;"
        "}"
        
        "QProgressBar {"
        "    border: 1px solid #dee2e6;"
        "    border-radius: 4px;"
        "    text-align: center;"
        "    background-color: #f8f9fa;"
        "}"
        
        "QProgressBar::chunk {"
        "    background-color: #28a745;"
        "    border-radius: 3px;"
        "}"
        
        "QCheckBox {"
        "    font-weight: normal;"
        "}"
        
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "}"
        
        "QCheckBox::indicator:unchecked {"
        "    border: 2px solid #dee2e6;"
        "    border-radius: 3px;"
        "    background-color: white;"
        "}"
        
        "QCheckBox::indicator:checked {"
        "    border: 2px solid #4A90E2;"
        "    border-radius: 3px;"
        "    background-color: #4A90E2;"
        "    image: url(:/icons/check.png);"
        "}"
    );
}

void ConfigWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
}

void ConfigWidget::loadSubjects()
{
    if (!m_configManager) {
        return;
    }
    
    m_subjectTree->clear();
    
    QStringList subjects = m_configManager->getAvailableSubjects();
    qDebug() << "Loading subjects:" << subjects;
    
    for (const QString &subject : subjects) {
        // 使用cleanSubjectName方法处理科目名称
        QString cleanedSubject = cleanSubjectName(subject);
        qDebug() << "loadSubjects - Raw subject:" << subject << "Cleaned subject:" << cleanedSubject;
        
        QTreeWidgetItem *subjectItem = new QTreeWidgetItem(m_subjectTree);
        subjectItem->setText(0, subject); // 保留原始显示名称
        subjectItem->setData(0, Qt::UserRole, "subject");
        setItemIcon(subjectItem, "subject");
        
        // 获取题库数据 - 使用清理后的科目名称
        QuestionBank bank = m_configManager->getQuestionBank(cleanedSubject);
        qDebug() << "Loading banks for subject:" << cleanedSubject;
        
        int totalQuestions = 0;
        int enabledBanks = 0;
        int totalBanks = 0;
        
        // Add Choice banks
        QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
        qDebug() << "Choice banks count:" << choiceBanks.size();
        for (int i = 0; i < choiceBanks.size(); ++i) {
            const auto &bankInfo = choiceBanks[i];
            if (bankInfo.size > 0) {
                totalBanks++;
                QTreeWidgetItem *bankItem = new QTreeWidgetItem(subjectItem);
                bankItem->setText(0, bankInfo.name.isEmpty() ? QString("选择题_%1").arg(i+1) : bankInfo.name);
                bankItem->setText(1, bankInfo.chosen ? "已启用" : "未启用");
                bankItem->setText(2, QString::number(bankInfo.size));
                bankItem->setData(0, Qt::UserRole, "bank");
                bankItem->setData(0, Qt::UserRole + 1, QString("choice_%1").arg(i));
                bankItem->setFlags(bankItem->flags() | Qt::ItemIsUserCheckable);
                bankItem->setCheckState(0, bankInfo.chosen ? Qt::Checked : Qt::Unchecked);
                setItemIcon(bankItem, "bank");
                
                if (bankInfo.chosen) {
                    totalQuestions += bankInfo.chosennum;
                    enabledBanks++;
                }
            }
        }
        
        // Add True/False banks
        QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
        qDebug() << "TrueOrFalse banks count:" << trueOrFalseBanks.size();
        for (int i = 0; i < trueOrFalseBanks.size(); ++i) {
            const auto &bankInfo = trueOrFalseBanks[i];
            if (bankInfo.size > 0) {
                totalBanks++;
                QTreeWidgetItem *bankItem = new QTreeWidgetItem(subjectItem);
                bankItem->setText(0, bankInfo.name.isEmpty() ? QString("判断题_%1").arg(i+1) : bankInfo.name);
                bankItem->setText(1, bankInfo.chosen ? "已启用" : "未启用");
                bankItem->setText(2, QString::number(bankInfo.size));
                bankItem->setData(0, Qt::UserRole, "bank");
                bankItem->setData(0, Qt::UserRole + 1, QString("trueorfalse_%1").arg(i));
                bankItem->setFlags(bankItem->flags() | Qt::ItemIsUserCheckable);
                bankItem->setCheckState(0, bankInfo.chosen ? Qt::Checked : Qt::Unchecked);
                setItemIcon(bankItem, "bank");
                
                if (bankInfo.chosen) {
                    totalQuestions += bankInfo.chosennum;
                    enabledBanks++;
                }
            }
        }
        
        // Add Fill Blank banks
        QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
        qDebug() << "FillBlank banks count:" << fillBlankBanks.size();
        for (int i = 0; i < fillBlankBanks.size(); ++i) {
            const auto &bankInfo = fillBlankBanks[i];
            if (bankInfo.size > 0) {
                totalBanks++;
                QTreeWidgetItem *bankItem = new QTreeWidgetItem(subjectItem);
                bankItem->setText(0, bankInfo.name.isEmpty() ? QString("填空题_%1").arg(i+1) : bankInfo.name);
                bankItem->setText(1, bankInfo.chosen ? "已启用" : "未启用");
                bankItem->setText(2, QString::number(bankInfo.size));
                bankItem->setData(0, Qt::UserRole, "bank");
                bankItem->setData(0, Qt::UserRole + 1, QString("fillblank_%1").arg(i));
                bankItem->setFlags(bankItem->flags() | Qt::ItemIsUserCheckable);
                bankItem->setCheckState(0, bankInfo.chosen ? Qt::Checked : Qt::Unchecked);
                setItemIcon(bankItem, "bank");
                
                if (bankInfo.chosen) {
                    totalQuestions += bankInfo.chosennum;
                    enabledBanks++;
                }
            }
        }
        
        // 更新科目状态和统计信息
        QString subjectStatus = QString("%1/%2").arg(enabledBanks).arg(totalBanks);
        subjectItem->setText(1, enabledBanks > 0 ? subjectStatus : "未启用");
        subjectItem->setText(2, QString::number(totalQuestions));
        
        subjectItem->setExpanded(true);
        
        qDebug() << "Subject" << subject << "loaded with" << totalBanks << "banks," << enabledBanks << "enabled";
    }
}

void ConfigWidget::loadBanksForSubject(const QString &subject)
{
    // 此方法已被整合到loadSubjects中，保留空实现以避免编译错误
    Q_UNUSED(subject);
}

void ConfigWidget::onSubjectSelectionChanged()
{
    QTreeWidgetItem *current = m_subjectTree->currentItem();
    if (!current) {
        // 清空所有显示
        clearSubjectInfo();
        clearBankDetails();
        // 隐藏题库详情组件
        m_bankDetailsGroup->setVisible(false);
        return;
    }
    
    QString itemType = current->data(0, Qt::UserRole).toString();
    qDebug() << "Selection changed, item type:" << itemType << "text:" << current->text(0);
    
    if (itemType == "subject") {
        QString rawSubjectName = current->text(0);
        // 清理可能的emoji前缀
        m_currentSubject = cleanSubjectName(rawSubjectName);
        m_currentBank.clear();
        
        qDebug() << "Subject selection - Raw text:" << rawSubjectName << "Cleaned:" << m_currentSubject;
        
        // 同步当前科目到ConfigManager并保存配置
        if (m_configManager) {
            m_configManager->setCurrentSubject(m_currentSubject);
            m_configManager->saveConfig(); // 保存当前科目设置
            qDebug() << "Set current subject in ConfigManager:" << m_currentSubject;
        }
        
        // Update subject info
        m_subjectNameEdit->setText(m_currentSubject);
        if (m_configManager) {
            QString subjectPath = m_configManager->getSubjectPath(m_currentSubject);
            m_subjectPathEdit->setText(subjectPath);
            qDebug() << "Subject path:" << subjectPath;
        }
        
        // Clear bank details when selecting subject
        clearBankDetails();
        
        // 隐藏题库详情组件
        m_bankDetailsGroup->setVisible(false);
        m_subjectActionsGroup->setVisible(true);
        
        // 显示科目级别的统计信息
        updateSubjectStatistics();
        
    } else if (itemType == "bank") {
        QTreeWidgetItem *parent = current->parent();
        if (parent) {
            QString rawSubjectName = parent->text(0);
            // 清理可能的emoji前缀
            m_currentSubject = cleanSubjectName(rawSubjectName);
            m_currentBank = current->data(0, Qt::UserRole + 1).toString();
            
            qDebug() << "Bank selection - Raw parent text:" << rawSubjectName;
            qDebug() << "Bank selection - Cleaned subject:" << m_currentSubject;
            qDebug() << "Bank selection - Bank identifier:" << m_currentBank;
            qDebug() << "Bank selection - Current item text:" << current->text(0);
            
            // 同步当前科目到ConfigManager并保存配置
            if (m_configManager) {
                m_configManager->setCurrentSubject(m_currentSubject);
                m_configManager->saveConfig(); // 保存当前科目设置
                qDebug() << "Set current subject in ConfigManager:" << m_currentSubject;
            }
            
            // Update subject info
            m_subjectNameEdit->setText(m_currentSubject);
            if (m_configManager) {
                QString subjectPath = m_configManager->getSubjectPath(m_currentSubject);
                m_subjectPathEdit->setText(subjectPath);
                qDebug() << "Bank selection - Subject path:" << subjectPath;
                qDebug() << "Bank selection - Has question bank:" << m_configManager->hasQuestionBank(m_currentSubject);
            }
            
            // 确保题库详情组件可见
            m_bankDetailsGroup->setVisible(true);
            m_subjectActionsGroup->setVisible(false);
            
            qDebug() << "Selected bank:" << m_currentBank << "for subject:" << m_currentSubject;
            updateBankDetails();
        }
    }
}

void ConfigWidget::onBankSelectionChanged()
{
    // 这个方法现在由onSubjectSelectionChanged统一处理
    // 保留空实现以避免信号连接错误
}

void ConfigWidget::clearSubjectInfo()
{
    m_subjectNameEdit->clear();
    m_subjectPathEdit->clear();
}

void ConfigWidget::clearBankDetails()
{
    m_bankNameLabel->setText("题库名称: --");
    m_bankTypeLabel->setText("题库类型: --");
    m_totalQuestionsLabel->setText("题目总数: --");
    m_bankStatusLabel->setText("状态: --");
    m_extractCountSpinBox->setValue(1);
    m_extractCountSpinBox->setEnabled(false);
    m_extractCountSlider->setValue(1);
    m_extractCountSlider->setEnabled(false);
    m_extractPercentLabel->setText("(0%)");
}

void ConfigWidget::updateSubjectStatistics()
{
    if (!m_configManager || m_currentSubject.isEmpty()) {
        return;
    }
    
    // 使用cleanSubjectName方法处理科目名称
    QString cleanedSubject = cleanSubjectName(m_currentSubject);
    qDebug() << "updateSubjectStatistics - Raw subject:" << m_currentSubject << "Cleaned subject:" << cleanedSubject;
    
    QuestionBank bank = m_configManager->getQuestionBank(cleanedSubject);
    qDebug() << "Retrieved bank for subject:" << cleanedSubject;
    qDebug() << "Bank has choice banks:" << bank.getChoiceBanks().size();
    qDebug() << "Bank has trueorfalse banks:" << bank.getTrueOrFalseBanks().size();
    qDebug() << "Bank has fillblank banks:" << bank.getFillBlankBanks().size();
    
    int totalBanks = 0;
    int enabledBanks = 0;
    int totalQuestions = 0;
    
    // Count choice banks
    QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
    for (const auto &bankInfo : choiceBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    // Count true/false banks
    QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
    for (const auto &bankInfo : trueOrFalseBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    // Count fill blank banks
    QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
    for (const auto &bankInfo : fillBlankBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    // 在题库详情区域显示科目统计信息
    m_bankNameLabel->setText(QString("科目: %1").arg(m_currentSubject));
    m_bankTypeLabel->setText(QString("题库总数: %1").arg(totalBanks));
    m_totalQuestionsLabel->setText(QString("题目总数: %1").arg(totalQuestions));
    m_bankStatusLabel->setText(QString("已启用题库: %1").arg(enabledBanks));
}

void ConfigWidget::updateBankDetails()
{
    if (!m_configManager || m_currentSubject.isEmpty() || m_currentBank.isEmpty()) {
        return;
    }
    
    // 使用cleanSubjectName方法处理科目名称
    QString cleanedSubject = cleanSubjectName(m_currentSubject);
    qDebug() << "updateBankDetails - Raw subject:" << m_currentSubject << "Cleaned subject:" << cleanedSubject;
    
    QuestionBank bank = m_configManager->getQuestionBank(cleanedSubject);
    qDebug() << "Retrieved bank for subject:" << cleanedSubject;
    qDebug() << "Bank has choice banks:" << bank.getChoiceBanks().size();
    qDebug() << "Bank has trueorfalse banks:" << bank.getTrueOrFalseBanks().size();
    qDebug() << "Bank has fillblank banks:" << bank.getFillBlankBanks().size();
    
    // 检查科目是否有题库数据
    if (bank.getChoiceBanks().isEmpty() && bank.getTrueOrFalseBanks().isEmpty() && bank.getFillBlankBanks().isEmpty()) {
        qDebug() << "No question banks found for subject:" << cleanedSubject;
        
        // 显示错误信息
        m_bankNameLabel->setText(QString("题库名称: %1").arg("未找到题库数据"));
        m_bankTypeLabel->setText(QString("题库类型: %1").arg("-"));
        m_totalQuestionsLabel->setText(QString("题目总数: %1").arg("0"));
        m_bankStatusLabel->setText(QString("状态: %1").arg("错误"));
        
        // 禁用控件
        {
            const QSignalBlocker spinBlocker(m_extractCountSpinBox);
            m_extractCountSpinBox->setValue(0);
            m_extractCountSpinBox->setEnabled(false);
        }
        {
            const QSignalBlocker sliderBlocker(m_extractCountSlider);
            m_extractCountSlider->setValue(0);
            m_extractCountSlider->setEnabled(false);
        }
        m_extractPercentLabel->setText("(0%)");
        
        // 显示错误提示
        QMessageBox::warning(this, "题库加载错误", 
                           QString("无法加载科目 '%1' 的题库数据。\n\n可能的原因:\n1. 科目目录不存在或无法访问\n2. 科目名称包含特殊字符\n3. 题库文件格式错误\n\n请检查科目路径: %2")
                           .arg(cleanedSubject)
                           .arg(m_configManager->getSubjectPath(cleanedSubject)));
        
        return;
    }
    
    // Parse bank identifier
    qDebug() << "updateBankDetails: Parsing bank identifier:" << m_currentBank;
    QStringList parts = m_currentBank.split("_");
    qDebug() << "Split parts:" << parts;
    if (parts.size() < 2) {
        qDebug() << "Invalid bank identifier format:" << m_currentBank;
        return;
    }
    
    QString bankType = parts[0];
    int bankIndex = parts[1].toInt();
    qDebug() << "Parsed bankType:" << bankType << "bankIndex:" << bankIndex;
    
    // 调整索引，因为在loadSubjects中索引是从0开始的，但显示时是从1开始的
    // 例如：trueorfalse_0对应显示为"判断题_1"
    // 但我们不需要调整，因为标识符中存储的就是正确的索引值
    
    QuestionBankInfo bankInfo;
    bool found = false;
    QString bankTypeName;
    
    if (bankType == "choice") {
        QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
        if (bankIndex >= 0 && bankIndex < choiceBanks.size()) {
            bankInfo = choiceBanks[bankIndex];
            found = true;
            bankTypeName = "选择题";
        }
    } else if (bankType == "trueorfalse") {
        QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
        qDebug() << "Searching for trueorfalse bank, index:" << bankIndex << "total banks:" << trueOrFalseBanks.size();
        
        // 打印所有判断题题库的信息，帮助调试
        for (int i = 0; i < trueOrFalseBanks.size(); ++i) {
            qDebug() << "TrueOrFalse bank" << i << "name:" << trueOrFalseBanks[i].name 
                     << "size:" << trueOrFalseBanks[i].size 
                     << "chosen:" << trueOrFalseBanks[i].chosen;
        }
        
        if (bankIndex >= 0 && bankIndex < trueOrFalseBanks.size()) {
            bankInfo = trueOrFalseBanks[bankIndex];
            found = true;
            bankTypeName = "判断题";
            qDebug() << "Found trueorfalse bank:" << bankInfo.name << "size:" << bankInfo.size;
        } else {
            qDebug() << "trueorfalse bank index out of range:" << bankIndex << "vs" << trueOrFalseBanks.size();
            
            // 尝试使用索引0，看看是否能找到题库
            if (trueOrFalseBanks.size() > 0) {
                qDebug() << "Trying with index 0 instead...";
                bankInfo = trueOrFalseBanks[0];
                found = true;
                bankTypeName = "判断题";
                qDebug() << "Found trueorfalse bank with index 0:" << bankInfo.name << "size:" << bankInfo.size;
            }
        }
    } else if (bankType == "fillblank") {
        QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
        if (bankIndex >= 0 && bankIndex < fillBlankBanks.size()) {
            bankInfo = fillBlankBanks[bankIndex];
            found = true;
            bankTypeName = "填空题";
        }
    }
    
    if (!found) {
        qDebug() << "updateBankDetails: Bank not found:" << bankType << bankIndex;
        
        // 显示错误信息
        m_bankNameLabel->setText(QString("题库名称: %1").arg("未找到题库"));
        m_bankTypeLabel->setText(QString("题库类型: %1").arg(bankType == "choice" ? "选择题" : 
                                                      bankType == "trueorfalse" ? "判断题" : 
                                                      bankType == "fillblank" ? "填空题" : "未知"));
        m_totalQuestionsLabel->setText(QString("题目总数: %1").arg("0"));
        m_bankStatusLabel->setText(QString("状态: %1").arg("未找到"));
        
        // 禁用控件
        {
            const QSignalBlocker spinBlocker(m_extractCountSpinBox);
            m_extractCountSpinBox->setValue(0);
            m_extractCountSpinBox->setEnabled(false);
        }
        {
            const QSignalBlocker sliderBlocker(m_extractCountSlider);
            m_extractCountSlider->setValue(0);
            m_extractCountSlider->setEnabled(false);
        }
        m_extractPercentLabel->setText("(0%)");
        
        return;
    }
    
    qDebug() << "updateBankDetails: Found bank" << bankInfo.name << "size:" << bankInfo.size << "chosen:" << bankInfo.chosen << "chosennum:" << bankInfo.chosennum;
    
    // Update UI
    QString displayName = bankInfo.name.isEmpty() ? "未命名" : bankInfo.name;
    
    // 更新UI组件并添加调试输出
    QString nameText = QString("题库名称: %1").arg(displayName);
    QString typeText = QString("题库类型: %1").arg(bankTypeName);
    QString sizeText = QString("题目总数: %1").arg(bankInfo.size);
    QString statusText = QString("状态: %1").arg(bankInfo.chosen ? "已启用" : "未启用");
    
    m_bankNameLabel->setText(nameText);
    m_bankTypeLabel->setText(typeText);
    m_totalQuestionsLabel->setText(sizeText);
    m_bankStatusLabel->setText(statusText);
    
    qDebug() << "UI Updated:" << nameText << typeText << sizeText << statusText;
    
    // Setup extract count
    int minValue = 1;
    int maxValue = qMax(1, bankInfo.size);
    int currentValue = qMax(1, bankInfo.chosennum);
    
    {
        const QSignalBlocker spinBlocker(m_extractCountSpinBox);
        m_extractCountSpinBox->setMinimum(minValue);
        m_extractCountSpinBox->setMaximum(maxValue);
        m_extractCountSpinBox->setValue(currentValue);
        m_extractCountSpinBox->setEnabled(bankInfo.chosen);
    }
    {
        const QSignalBlocker sliderBlocker(m_extractCountSlider);
        m_extractCountSlider->setMinimum(minValue);
        m_extractCountSlider->setMaximum(maxValue);
        m_extractCountSlider->setValue(currentValue);
        m_extractCountSlider->setEnabled(bankInfo.chosen);
    }
    
    qDebug() << "SpinBox range:" << minValue << "to" << maxValue << "value:" << currentValue << "enabled:" << bankInfo.chosen;
    
    // Update percentage
    double percentage = bankInfo.size > 0 ? (double)bankInfo.chosennum / bankInfo.size * 100 : 0;
    QString percentText = QString("(%1%)").arg(QString::number(percentage, 'f', 1));
    m_extractPercentLabel->setText(percentText);
    
    qDebug() << "Percentage:" << percentText;
    
    // 强制刷新UI
    m_bankDetailsGroup->update();
    qDebug() << "Bank details group visibility:" << m_bankDetailsGroup->isVisible();
}

void ConfigWidget::onBankCheckStateChanged(QTreeWidgetItem *item, int column)
{
    if (m_isLoading || !m_configManager || !item || column != 0) {
        return;
    }

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType != "bank") {
        return;
    }

    QTreeWidgetItem *parent = item->parent();
    if (!parent) {
        return;
    }

    const bool enabled = item->checkState(0) == Qt::Checked;
    const QString rawSubjectName = parent->text(0);
    const QString cleanedSubject = cleanSubjectName(rawSubjectName);
    const QString bankIdentifier = item->data(0, Qt::UserRole + 1).toString();

    if (cleanedSubject.isEmpty() || bankIdentifier.isEmpty()) {
        return;
    }

    QuestionBank bank = m_configManager->getQuestionBank(cleanedSubject);

    QStringList parts = bankIdentifier.split("_");
    if (parts.size() < 2) {
        return;
    }

    const QString bankType = parts[0];
    const int bankIndex = parts[1].toInt();

    if (bankType == "choice") {
        bank.setBankChosen(QuestionType::Choice, bankIndex, enabled);
    } else if (bankType == "trueorfalse") {
        bank.setBankChosen(QuestionType::TrueOrFalse, bankIndex, enabled);
    } else if (bankType == "fillblank") {
        bank.setBankChosen(QuestionType::FillBlank, bankIndex, enabled);
    } else {
        return;
    }

    m_configManager->setQuestionBank(cleanedSubject, bank);
    m_configManager->saveConfig();

    item->setText(1, enabled ? "已启用" : "未启用");

    updateSubjectItemStatistics(parent);
    updateStatistics();

    if (m_subjectTree->currentItem() == item) {
        m_extractCountSpinBox->setEnabled(enabled);
        m_extractCountSlider->setEnabled(enabled);
        m_bankStatusLabel->setText(QString("状态: %1").arg(enabled ? "已启用" : "未启用"));
    }

    emit configurationChanged();
}

void ConfigWidget::updateSubjectItemStatistics(QTreeWidgetItem *subjectItem)
{
    if (!subjectItem || !m_configManager) {
        return;
    }
    
    QString subjectName = cleanSubjectName(subjectItem->text(0));
    QuestionBank bank = m_configManager->getQuestionBank(subjectName);
    
    int totalQuestions = 0;
    int enabledBanks = 0;
    int totalBanks = 0;
    
    // Count all banks and their statistics
    QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
    for (const auto &bankInfo : choiceBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
    for (const auto &bankInfo : trueOrFalseBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
    for (const auto &bankInfo : fillBlankBanks) {
        if (bankInfo.size > 0) {
            totalBanks++;
            totalQuestions += bankInfo.size;
            if (bankInfo.chosen) {
                enabledBanks++;
            }
        }
    }
    
    // Update subject item display
    QString statusText = QString("%1/%2 已启用").arg(enabledBanks).arg(totalBanks);
    subjectItem->setText(1, statusText);
    subjectItem->setText(2, QString::number(totalQuestions));
}

void ConfigWidget::onExtractCountChanged(int count)
{
    if (m_isLoading || !m_configManager || m_currentSubject.isEmpty() || m_currentBank.isEmpty()) {
        return;
    }

    if (m_extractCountSlider && m_extractCountSlider->value() != count) {
        const QSignalBlocker blocker(m_extractCountSlider);
        m_extractCountSlider->setValue(count);
    }
    
    qDebug() << "onExtractCountChanged:" << count << "for bank:" << m_currentBank;
    
    // 使用cleanSubjectName方法处理科目名称
    QString cleanedSubject = cleanSubjectName(m_currentSubject);
    qDebug() << "onExtractCountChanged - Raw subject:" << m_currentSubject << "Cleaned subject:" << cleanedSubject;
    
    QuestionBank bank = m_configManager->getQuestionBank(cleanedSubject);
    qDebug() << "Retrieved bank for subject:" << cleanedSubject;
    qDebug() << "Bank has choice banks:" << bank.getChoiceBanks().size();
    qDebug() << "Bank has trueorfalse banks:" << bank.getTrueOrFalseBanks().size();
    qDebug() << "Bank has fillblank banks:" << bank.getFillBlankBanks().size();
    
    // Parse bank identifier
    QStringList parts = m_currentBank.split("_");
    if (parts.size() >= 2) {
        QString bankType = parts[0];
        int bankIndex = parts[1].toInt();
        
        if (bankType == "choice") {
            bank.setBankChosenNum(QuestionType::Choice, bankIndex, count);
            QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
            if (bankIndex >= 0 && bankIndex < choiceBanks.size()) {
                double percentage = choiceBanks[bankIndex].size > 0 ? 
                                  (double)count / choiceBanks[bankIndex].size * 100.0 : 0.0;
                m_extractPercentLabel->setText(QString("(%1%)").arg(QString::number(percentage, 'f', 1)));
            }
        } else if (bankType == "trueorfalse") {
            bank.setBankChosenNum(QuestionType::TrueOrFalse, bankIndex, count);
            QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
            if (bankIndex >= 0 && bankIndex < trueOrFalseBanks.size()) {
                double percentage = trueOrFalseBanks[bankIndex].size > 0 ? 
                                  (double)count / trueOrFalseBanks[bankIndex].size * 100.0 : 0.0;
                m_extractPercentLabel->setText(QString("(%1%)").arg(QString::number(percentage, 'f', 1)));
            }
        } else if (bankType == "fillblank") {
            bank.setBankChosenNum(QuestionType::FillBlank, bankIndex, count);
            QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
            if (bankIndex >= 0 && bankIndex < fillBlankBanks.size()) {
                double percentage = fillBlankBanks[bankIndex].size > 0 ? 
                                  (double)count / fillBlankBanks[bankIndex].size * 100.0 : 0.0;
                m_extractPercentLabel->setText(QString("(%1%)").arg(QString::number(percentage, 'f', 1)));
            }
        }
        
        // 更新配置管理器中的数据
        m_configManager->setQuestionBank(cleanedSubject, bank);
        
        // 自动保存配置到文件
        m_configManager->saveConfig();
        
        // 更新父节点的统计信息
        QTreeWidgetItem *current = m_subjectTree->currentItem();
        QTreeWidgetItem *parent = current ? current->parent() : nullptr;
        if (parent) {
            updateSubjectItemStatistics(parent);
        }
        
        updateStatistics();
        emit configurationChanged();
    }
}

void ConfigWidget::updateStatistics()
{
    if (!m_configManager) {
        return;
    }
    
    QStringList subjects = m_configManager->getAvailableSubjects();
    int totalSubjects = subjects.size();
    int totalBanks = 0;
    int enabledBanks = 0;
    int totalQuestions = 0;
    
    for (const QString &subject : subjects) {
        QuestionBank bank = m_configManager->getQuestionBank(subject);
        
        // Count choice banks
        QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
        for (const auto &bankInfo : choiceBanks) {
            if (bankInfo.size > 0) {
                totalBanks++;
                if (bankInfo.chosen) {
                    enabledBanks++;
                    totalQuestions += bankInfo.chosennum;
                }
            }
        }
        
        // Count true/false banks
        QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
        for (const auto &bankInfo : trueOrFalseBanks) {
            if (bankInfo.size > 0) {
                totalBanks++;
                if (bankInfo.chosen) {
                    enabledBanks++;
                    totalQuestions += bankInfo.chosennum;
                }
            }
        }
        
        // Count fill blank banks
        QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
        for (const auto &bankInfo : fillBlankBanks) {
            if (bankInfo.size > 0) {
                totalBanks++;
                if (bankInfo.chosen) {
                    enabledBanks++;
                    totalQuestions += bankInfo.chosennum;
                }
            }
        }
    }
    
    m_totalSubjectsLabel->setText(QString("总科目数: %1").arg(totalSubjects));
    m_totalBanksLabel->setText(QString("总题库数: %1").arg(totalBanks));
    m_enabledBanksLabel->setText(QString("已启用: %1").arg(enabledBanks));
    m_statisticsQuestionsLabel->setText(QString("总题目数: %1").arg(totalQuestions));
    
    int progress = totalBanks > 0 ? (enabledBanks * 100 / totalBanks) : 0;
    m_configProgressBar->setValue(progress);
}

void ConfigWidget::onAddSubjectClicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择科目文件夹", "Subject");
    if (dirPath.isEmpty()) {
        return;
    }
    
    QDir dir(dirPath);
    QString subjectName = dir.dirName();
    
    // 检查科目是否已存在
    if (m_configManager && m_configManager->hasQuestionBank(subjectName)) {
        int ret = QMessageBox::question(this, "科目已存在", 
            QString("科目 '%1' 已存在，是否要重新添加？").arg(subjectName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    
    if (m_configManager) {
        // addSubject方法现在会自动扫描科目目录并加载题库数据
        m_configManager->addSubject(subjectName, dirPath);
        
        // 获取添加后的题库信息
        QuestionBank bank = m_configManager->getQuestionBank(subjectName);
        int totalBanks = bank.getChoiceBanks().size() + bank.getTrueOrFalseBanks().size() + bank.getFillBlankBanks().size();
        
        // 保存配置到文件
        if (!m_configManager->saveConfig()) {
            QMessageBox::warning(this, "保存失败", 
                QString("科目 '%1' 添加成功，但配置保存失败：%2")
                .arg(subjectName)
                .arg(m_configManager->getLastError()));
            return;
        }
        
        refreshData();
        emit configurationChanged();
        
        // 显示添加结果
        if (totalBanks > 0) {
            QMessageBox::information(this, "添加成功", 
                QString("科目 '%1' 添加成功！\n\n找到题库：\n- 选择题：%2 个\n- 判断题：%3 个\n- 填空题：%4 个\n\n配置已自动保存到 config.json")
                .arg(subjectName)
                .arg(bank.getChoiceBanks().size())
                .arg(bank.getTrueOrFalseBanks().size())
                .arg(bank.getFillBlankBanks().size()));
        } else {
            QMessageBox::information(this, "添加成功", 
                QString("科目 '%1' 添加成功！\n\n注意：在该科目文件夹中未找到有效的题库文件。\n请确保文件夹包含 Choice、TrueorFalse 或 FillBlank 子文件夹，\n并在其中放置相应的JSON题库文件。\n\n配置已自动保存到 config.json")
                .arg(subjectName));
        }
    }
}

void ConfigWidget::onCreateSubjectClicked()
{
    if (!m_configManager) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("创建科目");
    dialog.setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    QLineEdit *pathEdit = new QLineEdit(&dialog);
    pathEdit->setText("Subject");

    QWidget *pathRow = new QWidget(&dialog);
    QHBoxLayout *pathRowLayout = new QHBoxLayout(pathRow);
    pathRowLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton *browseButton = new QPushButton("浏览", pathRow);
    pathRowLayout->addWidget(pathEdit);
    pathRowLayout->addWidget(browseButton);

    formLayout->addRow("科目名称:", nameEdit);
    formLayout->addRow("保存路径:", pathRow);

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("创建");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    mainLayout->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, &dialog, [this, pathEdit]() {
        const QString dirPath = QFileDialog::getExistingDirectory(this, "选择保存路径", pathEdit->text().trimmed());
        if (!dirPath.isEmpty()) {
            pathEdit->setText(dirPath);
        }
    });

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString subjectName = nameEdit->text().trimmed();
        const QString basePath = pathEdit->text().trimmed();

        if (subjectName.isEmpty()) {
            QMessageBox::warning(&dialog, "创建失败", "科目名称不能为空。");
            return;
        }

        if (QRegularExpression(R"([\\/:*?"<>|])").match(subjectName).hasMatch()) {
            QMessageBox::warning(&dialog, "创建失败", "科目名称包含不允许的字符。");
            return;
        }

        if (basePath.isEmpty()) {
            QMessageBox::warning(&dialog, "创建失败", "保存路径不能为空。");
            return;
        }

        if (m_configManager->hasQuestionBank(subjectName)) {
            int ret = QMessageBox::question(&dialog, "科目已存在",
                QString("科目 '%1' 已存在，是否覆盖其配置并重新添加？").arg(subjectName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }
        }

        QDir baseDir(basePath);
        if (!baseDir.exists()) {
            QMessageBox::warning(&dialog, "创建失败", "保存路径不存在或无法访问。");
            return;
        }

        if (!baseDir.mkpath(subjectName)) {
            QMessageBox::warning(&dialog, "创建失败", "无法在保存路径下创建科目文件夹。");
            return;
        }

        const QString subjectDirPath = baseDir.filePath(subjectName);
        m_configManager->addSubject(subjectName, subjectDirPath);
        m_configManager->setCurrentSubject(subjectName);

        if (!m_configManager->saveConfig()) {
            QMessageBox::warning(&dialog, "创建失败",
                QString("科目创建成功，但配置保存失败：%1").arg(m_configManager->getLastError()));
            return;
        }

        dialog.accept();

        refreshData();
        emit configurationChanged();

        QMessageBox::information(this, "创建成功",
            QString("科目 '%1' 已创建并导入配置。\n\n路径：%2")
                .arg(subjectName, QDir::toNativeSeparators(subjectDirPath)));
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.exec();
}

void ConfigWidget::onRemoveSubjectClicked()
{
    QTreeWidgetItem *current = m_subjectTree->currentItem();
    if (!current) {
        return;
    }
    
    QString itemType = current->data(0, Qt::UserRole).toString();
    if (itemType != "subject") {
        QMessageBox::information(this, "提示", "请选择要删除的科目");
        return;
    }
    
    QString subject = current->text(0);
    int ret = QMessageBox::question(this, "确认删除", 
        QString("确定要删除科目 '%1' 吗？\n\n注意：这将从配置中移除该科目，但不会删除科目文件夹。").arg(subject),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes && m_configManager) {
        // 获取科目路径（用于日志显示）
        QString subjectPath = m_configManager->getSubjectPath(subject);
        
        // 从配置中移除科目
        m_configManager->removeSubject(subject);
        
        // 如果当前选中的是被删除的科目，清除当前选择
        if (m_currentSubject == subject) {
            m_currentSubject.clear();
            m_currentBank.clear();
            
            // 清除科目信息
            m_subjectNameEdit->clear();
            m_subjectPathEdit->clear();
            
            // 清除题库详情
            m_bankNameLabel->setText("题库名称: --");
            m_bankTypeLabel->setText("题库类型: --");
            m_totalQuestionsLabel->setText("题目总数: --");
            m_bankStatusLabel->setText("状态: --");
            m_extractCountSpinBox->setValue(1);
            m_extractCountSpinBox->setEnabled(false);
            m_extractCountSlider->setValue(1);
            m_extractCountSlider->setEnabled(false);
            m_extractPercentLabel->setText("(0%)");
        }
        
        // 保存配置到文件
        if (!m_configManager->saveConfig()) {
            QMessageBox::warning(this, "保存失败", 
                QString("科目 '%1' 删除成功，但配置保存失败：%2")
                .arg(subject)
                .arg(m_configManager->getLastError()));
            return;
        }
        
        // 刷新UI
        refreshData();
        emit configurationChanged();
        
        // 显示成功消息
        QMessageBox::information(this, "删除成功", 
            QString("科目 '%1' 已从配置中移除。\n\n科目文件夹仍保留在：\n%2\n\n配置已自动保存到 config.json")
            .arg(subject)
            .arg(subjectPath));
    }
}

void ConfigWidget::onCreateBankClicked()
{
    if (!m_configManager) {
        return;
    }

    if (m_currentSubject.isEmpty()) {
        QMessageBox::information(this, "提示", "请先在左侧选择一个科目。");
        return;
    }

    const QString subjectPath = m_configManager->getSubjectPath(m_currentSubject);
    if (subjectPath.trimmed().isEmpty()) {
        QMessageBox::warning(this, "创建失败", "当前科目路径为空。");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("创建题库");
    dialog.setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("选择题 (Choice)", "Choice");
    typeCombo->addItem("判断题 (TrueorFalse)", "TrueorFalse");
    typeCombo->addItem("填空题 (FillBlank)", "FillBlank");

    formLayout->addRow("题库名称:", nameEdit);
    formLayout->addRow("题库类型:", typeCombo);
    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("创建");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString bankName = nameEdit->text().trimmed();
        const QString typeStr = typeCombo->currentData().toString();

        if (bankName.isEmpty()) {
            QMessageBox::warning(&dialog, "创建失败", "题库名称不能为空。");
            return;
        }
        if (QRegularExpression(R"([\\/:*?"<>|])").match(bankName).hasMatch()) {
            QMessageBox::warning(&dialog, "创建失败", "题库名称包含不允许的字符。");
            return;
        }

        QString typeFolder;
        if (typeStr == "Choice") {
            typeFolder = "Choice";
        } else if (typeStr == "TrueorFalse") {
            typeFolder = "TrueorFalse";
        } else if (typeStr == "FillBlank") {
            typeFolder = "FillBlank";
        } else {
            QMessageBox::warning(&dialog, "创建失败", "未知题库类型。");
            return;
        }

        QDir subjectDir(subjectPath);
        if (!subjectDir.exists()) {
            QMessageBox::warning(&dialog, "创建失败", "科目路径不存在或无法访问。");
            return;
        }

        if (!subjectDir.mkpath(typeFolder)) {
            QMessageBox::warning(&dialog, "创建失败", "无法创建题型目录。");
            return;
        }

        const QString filePath = subjectDir.filePath(typeFolder + "/" + bankName + ".json");
        if (QFileInfo::exists(filePath)) {
            int ret = QMessageBox::question(&dialog, "文件已存在",
                QString("题库文件已存在：\n%1\n\n是否覆盖？").arg(QDir::toNativeSeparators(filePath)),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }
        }

        QJsonObject q;
        q["type"] = typeStr;
        q["question"] = QString("请在此编辑题干");
        if (typeStr == "Choice") {
            QJsonArray choices;
            choices.append("A. 选项A");
            choices.append("B. 选项B");
            choices.append("C. 选项C");
            choices.append("D. 选项D");
            q["choices"] = choices;
            q["answer"] = "A";
        } else if (typeStr == "TrueorFalse") {
            q["answer"] = "T";
        } else if (typeStr == "FillBlank") {
            q["BlankNum"] = 1;
            QJsonArray answers;
            answers.append("");
            q["answer"] = answers;
        }

        QJsonObject root;
        QJsonArray data;
        data.append(q);
        root["data"] = data;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::warning(&dialog, "创建失败", "无法写入题库文件。");
            return;
        }
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        dialog.accept();

        m_configManager->refreshSubjectBanks(m_currentSubject);
        if (!m_configManager->saveConfig()) {
            QMessageBox::warning(this, "提示", QString("题库创建成功，但配置保存失败：%1").arg(m_configManager->getLastError()));
        }

        refreshData();
        emit configurationChanged();

        QMessageBox::information(this, "创建成功",
            QString("题库已创建：\n%1").arg(QDir::toNativeSeparators(filePath)));
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}

void ConfigWidget::onAutoFetchBankClicked()
{
    if (!m_configManager) {
        return;
    }
    if (m_currentSubject.isEmpty()) {
        QMessageBox::information(this, "提示", "请先在左侧选择一个科目。");
        return;
    }

    PtaImportDialog dialog(m_configManager, m_currentSubject, this);
    dialog.exec();

    refreshData();
    emit configurationChanged();
}

void ConfigWidget::onRemoveBankClicked()
{
    QTreeWidgetItem *current = m_subjectTree->currentItem();
    if (!current) {
        QMessageBox::information(this, "提示", "请选择要删除的题库");
        return;
    }
    
    QString itemType = current->data(0, Qt::UserRole).toString();
    if (itemType != "bank") {
        QMessageBox::information(this, "提示", "请选择要删除的题库");
        return;
    }
    
    QTreeWidgetItem *parent = current->parent();
    if (!parent) {
        return;
    }
    
    QString subject = parent->text(0);
    QString bankId = current->data(0, Qt::UserRole + 1).toString();
    QString bankName = current->text(0);
    
    int ret = QMessageBox::question(this, "确认删除", 
        QString("确定要删除题库 '%1' 吗？\n\n注意：这将删除题库文件，操作不可恢复！").arg(bankName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes && m_configManager) {
        // 解析题库标识符
        QStringList parts = bankId.split("_");
        if (parts.size() >= 2) {
            QString bankType = parts[0];
            int bankIndex = parts[1].toInt();
            
            QuestionBank bank = m_configManager->getQuestionBank(subject);
            QString filePath;
            
            // 获取题库文件路径
            if (bankType == "choice") {
                QVector<QuestionBankInfo> choiceBanks = bank.getChoiceBanks();
                if (bankIndex >= 0 && bankIndex < choiceBanks.size()) {
                    QString subjectPath = m_configManager->getSubjectPath(subject);
                    filePath = QDir(subjectPath).filePath("Choice/" + choiceBanks[bankIndex].src);
                    
                    // 从配置中移除题库
                    choiceBanks.removeAt(bankIndex);
                    bank.setChoiceBanks(choiceBanks);
                }
            } else if (bankType == "trueorfalse") {
                QVector<QuestionBankInfo> trueOrFalseBanks = bank.getTrueOrFalseBanks();
                if (bankIndex >= 0 && bankIndex < trueOrFalseBanks.size()) {
                    QString subjectPath = m_configManager->getSubjectPath(subject);
                    filePath = QDir(subjectPath).filePath("TrueorFalse/" + trueOrFalseBanks[bankIndex].src);
                    
                    // 从配置中移除题库
                    trueOrFalseBanks.removeAt(bankIndex);
                    bank.setTrueOrFalseBanks(trueOrFalseBanks);
                }
            } else if (bankType == "fillblank") {
                QVector<QuestionBankInfo> fillBlankBanks = bank.getFillBlankBanks();
                if (bankIndex >= 0 && bankIndex < fillBlankBanks.size()) {
                    QString subjectPath = m_configManager->getSubjectPath(subject);
                    filePath = QDir(subjectPath).filePath("FillBlank/" + fillBlankBanks[bankIndex].src);
                    
                    // 从配置中移除题库
                    fillBlankBanks.removeAt(bankIndex);
                    bank.setFillBlankBanks(fillBlankBanks);
                }
            }
            
            // 删除题库文件
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                if (QFile::remove(filePath)) {
                    // 更新配置
                    m_configManager->setQuestionBank(subject, bank);
                    
                    // 刷新界面
                    refreshData();
                    emit configurationChanged();
                    
                    QMessageBox::information(this, "删除成功", 
                        QString("题库 '%1' 已成功删除").arg(bankName));
                } else {
                    QMessageBox::warning(this, "删除失败", 
                        QString("无法删除题库文件：%1").arg(filePath));
                }
            } else {
                // 即使文件不存在，也从配置中移除
                m_configManager->setQuestionBank(subject, bank);
                refreshData();
                emit configurationChanged();
                
                QMessageBox::information(this, "删除完成", 
                    QString("题库 '%1' 已从配置中移除（文件不存在）").arg(bankName));
            }
        }
    }
}

void ConfigWidget::onSaveClicked()
{
    if (m_configManager) {
        if (m_configManager->saveConfig()) {
            QMessageBox::information(this, "保存成功", "配置已保存");
        } else {
            QMessageBox::warning(this, "保存失败", "配置保存失败");
        }
    }
}

void ConfigWidget::onEditBankClicked()
{
    if (m_currentSubject.isEmpty() || m_currentBank.isEmpty()) {
        QMessageBox::warning(this, "编辑失败", "请先选择要编辑的题库。");
        return;
    }
    
    // Parse bank identifier to get bank type and index
    QStringList parts = m_currentBank.split("_");
    if (parts.size() < 2) {
        QMessageBox::warning(this, "编辑失败", "题库标识符格式错误。");
        return;
    }
    
    QString bankType = parts[0];
    m_currentBankIndex = parts[1].toInt();
    m_currentBankType = bankType;
    
    // Set bank info in editor
    m_bankEditorWidget->setBankInfo(m_currentSubject, bankType, m_currentBankIndex);
    
    // Switch to bank editor
    m_stackedWidget->setCurrentWidget(m_bankEditorWidget);
}

void ConfigWidget::onBankEditorBack()
{
    // Switch back to main config view
    m_stackedWidget->setCurrentWidget(m_stackedWidget->widget(0));
    
    // Refresh data to reflect any changes
    refreshData();
}

void ConfigWidget::onBackClicked()
{
    emit backRequested();
}

QTreeWidgetItem* ConfigWidget::findSubjectItem(const QString &subject)
{
    for (int i = 0; i < m_subjectTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_subjectTree->topLevelItem(i);
        if (item->text(0) == subject) {
            return item;
        }
    }
    return nullptr;
}

QTreeWidgetItem* ConfigWidget::findBankItem(QTreeWidgetItem *subjectItem, const QString &bankName)
{
    if (!subjectItem) {
        return nullptr;
    }
    
    for (int i = 0; i < subjectItem->childCount(); ++i) {
        QTreeWidgetItem *item = subjectItem->child(i);
        if (item->data(0, Qt::UserRole + 1).toString() == bankName) {
            return item;
        }
    }
    return nullptr;
}

QString ConfigWidget::getCurrentSubject() const
{
    return m_currentSubject;
}

QString ConfigWidget::getCurrentBank() const
{
    return m_currentBank;
}

QString ConfigWidget::cleanSubjectName(const QString &rawName) const
{
    // 清理可能的emoji前缀
    QString cleanName = rawName;
    
    // 处理"📚 "前缀 (可能显示为"?? ")
    if (cleanName.startsWith("📚 ")) {
        cleanName = cleanName.mid(3); // 移除"📚 "前缀
    } else if (cleanName.startsWith("?? ")) {
        cleanName = cleanName.mid(3); // 移除"?? "前缀
    }
    
    // 处理其他可能的前缀
    if (cleanName.startsWith("📝 ")) {
        cleanName = cleanName.mid(3); // 移除"📝 "前缀
    }
    
    return cleanName;
}

void ConfigWidget::setItemIcon(QTreeWidgetItem *item, const QString &iconType)
{
    // 不再使用文本前缀作为图标，避免编码问题
    // 可以在这里使用QIcon设置图标，或者不设置图标
    // 暂时不做任何操作，保留原始文本
    Q_UNUSED(iconType);
}
