#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
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
#include <QStackedWidget>
#include <QFileDialog>
#include <QMessageBox>
#include "../utils/bankscanner.h"

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSplitter;
class QProgressBar;
class QLineEdit;
class QComboBox;
class QTextEdit;
QT_END_NAMESPACE

class ConfigManager;
struct QuestionBank;
class BankEditorWidget;

class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(QWidget *parent = nullptr);
    ~ConfigWidget();

    void setConfigManager(ConfigManager *configManager);
    void refreshData();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onSubjectSelectionChanged();
    void onBankSelectionChanged();
    void onBankCheckStateChanged(QTreeWidgetItem *item, int column);
    void onExtractCountChanged(int count);
    void onAddSubjectClicked();
    void onCreateSubjectClicked();
    void onRemoveSubjectClicked();
    void onRemoveBankClicked(); // 新增删除题库的槽函数
    void onSaveClicked();
    void onEditBankClicked();
    void onBankEditorBack();
    void onBackClicked();
    void onCreateBankClicked();
    void onAutoFetchBankClicked();

signals:
    void backRequested();
    void configurationChanged();

private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    void loadSubjects();
    void loadBanksForSubject(const QString &subject);
    void updateBankDetails();
    void updateStatistics();
    void saveCurrentConfiguration();
    void resetToDefaults();
    
    // Helper methods
    QTreeWidgetItem* findSubjectItem(const QString &subject);
    QTreeWidgetItem* findBankItem(QTreeWidgetItem *subjectItem, const QString &bankName);
    QString getCurrentSubject() const;
    QString getCurrentBank() const;
    QString cleanSubjectName(const QString &rawName) const; // 清理科目名称中的emoji前缀
    void setItemIcon(QTreeWidgetItem *item, const QString &iconType);
    void clearSubjectInfo();
    void clearBankDetails();
    void updateSubjectStatistics();
    void updateSubjectItemStatistics(QTreeWidgetItem *subjectItem);
    
    // UI Components - Left Panel (Tree View)
    QSplitter *m_mainSplitter;
    QWidget *m_leftPanel;
    QVBoxLayout *m_leftLayout;
    QLabel *m_subjectLabel;
    QTreeWidget *m_subjectTree;
    QHBoxLayout *m_subjectButtonLayout;
    QPushButton *m_addSubjectButton;
    QPushButton *m_createSubjectButton;
    QPushButton *m_removeSubjectButton;
    QPushButton *m_removeBankButton; // 新增删除题库按钮
    
    // UI Components - Right Panel (Details)
    QWidget *m_rightPanel;
    QVBoxLayout *m_rightLayout;
    
    // Subject Info Group
    QGroupBox *m_subjectInfoGroup;
    QGridLayout *m_subjectInfoLayout;
    QLabel *m_subjectNameLabel;
    QLineEdit *m_subjectNameEdit;
    QLabel *m_subjectPathLabel;
    QLineEdit *m_subjectPathEdit;
    QPushButton *m_browsePathButton;

    // Subject Actions Group
    QGroupBox *m_subjectActionsGroup;
    QHBoxLayout *m_subjectActionsLayout;
    QPushButton *m_createBankButton;
    QPushButton *m_autoFetchBankButton;
    
    // Bank Details Group
    QGroupBox *m_bankDetailsGroup;
    QGridLayout *m_bankDetailsLayout;
    QLabel *m_bankNameLabel;
    QLabel *m_bankTypeLabel;
    QLabel *m_totalQuestionsLabel;
    QLabel *m_bankStatusLabel;
    QLabel *m_extractCountLabel;
    QSpinBox *m_extractCountSpinBox;
    QSlider *m_extractCountSlider;
    QLabel *m_extractPercentLabel;
    QPushButton *m_editBankButton;
    
    // Statistics Group
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_totalSubjectsLabel;
    QLabel *m_totalBanksLabel;
    QLabel *m_enabledBanksLabel;
    QLabel *m_statisticsQuestionsLabel;
    QCheckBox *m_shuffleQuestionsCheckBox;
    QProgressBar *m_configProgressBar;
    
    // Description Group
    QGroupBox *m_descriptionGroup;
    QVBoxLayout *m_descriptionLayout;
    QTextEdit *m_descriptionEdit;
    
    // Bottom Buttons
    QHBoxLayout *m_bottomButtonLayout;
    QPushButton *m_saveButton;
    QPushButton *m_backButton;
    
    // Data
    ConfigManager *m_configManager;
    bool m_isLoading;
    
    // Bank editor
    QStackedWidget *m_stackedWidget;
    BankEditorWidget *m_bankEditorWidget;
    QString m_currentSubject;
    QString m_currentBank;
    QString m_currentBankType;
    int m_currentBankIndex;
};

#endif // CONFIGWIDGET_H
