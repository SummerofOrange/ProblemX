#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
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
    void onBankEnabledChanged(bool enabled);
    void onExtractCountChanged(int count);
    void onAddSubjectClicked();
    void onRemoveSubjectClicked();
    void onRemoveBankClicked(); // 新增删除题库的槽函数
    void onRefreshClicked();
    void onSaveClicked();
    void onResetClicked();
    void onImportSubjectClicked();
    void onExportSubjectClicked();
    void onBackClicked();

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
    QPushButton *m_removeSubjectButton;
    QPushButton *m_removeBankButton; // 新增删除题库按钮
    QPushButton *m_importButton;
    QPushButton *m_exportButton;
    
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
    
    // Bank Details Group
    QGroupBox *m_bankDetailsGroup;
    QGridLayout *m_bankDetailsLayout;
    QLabel *m_bankNameLabel;
    QLabel *m_bankTypeLabel;
    QLabel *m_totalQuestionsLabel;
    QLabel *m_bankStatusLabel;
    QCheckBox *m_enableBankCheckBox;
    QLabel *m_extractCountLabel;
    QSpinBox *m_extractCountSpinBox;
    QLabel *m_extractPercentLabel;
    
    // Statistics Group
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_totalSubjectsLabel;
    QLabel *m_totalBanksLabel;
    QLabel *m_enabledBanksLabel;
    QLabel *m_statisticsQuestionsLabel;
    QProgressBar *m_configProgressBar;
    
    // Description Group
    QGroupBox *m_descriptionGroup;
    QVBoxLayout *m_descriptionLayout;
    QTextEdit *m_descriptionEdit;
    
    // Bottom Buttons
    QHBoxLayout *m_bottomButtonLayout;
    QPushButton *m_refreshButton;
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
    QPushButton *m_backButton;
    
    // Data
    ConfigManager *m_configManager;
    bool m_isLoading;
    QString m_currentSubject;
    QString m_currentBank;
};

#endif // CONFIGWIDGET_H