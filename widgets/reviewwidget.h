#ifndef REVIEWWIDGET_H
#define REVIEWWIDGET_H

#include <QWidget>
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
#include <QSplitter>
#include <QFrame>
#include <QScrollArea>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include "../models/question.h"
#include "../core/wronganswerset.h"
#include "../utils/markdownrenderer.h"  // 新增

class ConfigManager;
class PracticeManager;



class ReviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReviewWidget(QWidget *parent = nullptr);
    ~ReviewWidget();
    
    void setConfigManager(ConfigManager *configManager);
    void setPracticeManager(PracticeManager *practiceManager);
    
    void loadWrongAnswers();
    void refreshWrongAnswers();
    
    // 新的错题集合管理
    void setWrongAnswerSet(WrongAnswerSet *wrongAnswerSet);
    WrongAnswerSet* getWrongAnswerSet() const;
    
signals:
    void backRequested();
    void startReviewRequested(const QList<Question> &questions);
    
protected:
    void showEvent(QShowEvent *event) override;
    
private slots:
    void onSubjectFilterChanged();
    void onTypeFilterChanged();
    void onDateFilterChanged();
    void onResolvedFilterChanged();
    void onSortOrderChanged();
    void onWrongAnswerSelected(QListWidgetItem *item);
    void onStartReviewClicked();
    void onClearResolvedClicked();
    void onExportClicked();
    void onImportClicked();
    void onDeleteSelectedClicked();
    void onMarkResolvedClicked();
    void onMarkUnresolvedClicked();
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onBackClicked();
    void onRefreshClicked();
    
    // 错题导入询问处理
    void onWrongAnswersImportRequested(const QList<Question> &wrongQuestions, const QString &subject);
private:
    void setupUI();
    void setupConnections();
    void applyStyles();
    
    void updateWrongAnswersList();
    void updateStatistics();
    void updateFilterOptions();
    void updateSelectionButtons();
    void displayWrongAnswerDetails(const WrongAnswerItem &item);
    void clearWrongAnswerDetails();
    
    QVector<WrongAnswerItem> getFilteredWrongAnswerItems() const;
    QStringList getSelectedIds() const;
    
    void exportWrongAnswers();
    void importWrongAnswers();
    
    // 错题复习相关
    Question convertWrongAnswerItemToQuestion(const WrongAnswerItem &item) const;
    
    void markRecordsAsResolved(const QVector<int> &indices);
    void markRecordsAsUnresolved(const QVector<int> &indices);
    void deleteRecords(const QVector<int> &indices);
    
    QString getQuestionTypeString(const QString &type) const;
    QString formatDateTime(const QDateTime &dateTime) const;
    
    // 新的错题集合管理方法
    void updateFromWrongAnswerSet();
    
private:
    ConfigManager *m_configManager;
    PracticeManager *m_practiceManager;
    WrongAnswerSet *m_wrongAnswerSet;
    
    QVector<WrongAnswerItem> m_currentItems;    // 当前显示的错题项目
    QVector<WrongAnswerItem> m_filteredItems;   // 筛选后的错题项目
    
    // UI Components
    QSplitter *m_mainSplitter;
    
    // Left panel - filters and list
    QWidget *m_leftPanel;
    QVBoxLayout *m_leftLayout;
    
    // Filter group
    QGroupBox *m_filterGroup;
    QGridLayout *m_filterLayout;
    QLabel *m_subjectLabel;
    QComboBox *m_subjectCombo;
    QLabel *m_typeLabel;
    QComboBox *m_typeCombo;
    QLabel *m_dateLabel;
    QDateEdit *m_dateFromEdit;
    QDateEdit *m_dateToEdit;
    QCheckBox *m_showResolvedCheck;
    QLabel *m_sortLabel;
    QComboBox *m_sortCombo;
    QPushButton *m_refreshButton;
    
    // Statistics group
    QGroupBox *m_statisticsGroup;
    QGridLayout *m_statisticsLayout;
    QLabel *m_totalLabel;
    QLabel *m_unresolvedLabel;
    QLabel *m_resolvedLabel;
    QLabel *m_selectedLabel;
    QProgressBar *m_resolvedProgressBar;
    
    // Wrong answers list
    QGroupBox *m_listGroup;
    QVBoxLayout *m_listLayout;
    QListWidget *m_wrongAnswersList;
    
    // Selection buttons
    QHBoxLayout *m_selectionLayout;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    
    // Action buttons
    QGridLayout *m_actionLayout;
    QPushButton *m_startReviewButton;
    QPushButton *m_markResolvedButton;
    QPushButton *m_markUnresolvedButton;
    QPushButton *m_deleteSelectedButton;
    QPushButton *m_clearResolvedButton;
    
    // Import/Export buttons
    QHBoxLayout *m_importExportLayout;
    QPushButton *m_exportButton;
    QPushButton *m_importButton;
    
    // Control buttons
    QHBoxLayout *m_controlLayout;
    QPushButton *m_backButton;
    
    // Right panel - details
    QWidget *m_rightPanel;
    QVBoxLayout *m_rightLayout;
    
    // Details group - 修改为使用MarkdownRenderer
    QGroupBox *m_detailsGroup;
    QVBoxLayout *m_detailsLayout;
    QScrollArea *m_detailsScrollArea;
    QWidget *m_detailsContent;
    QVBoxLayout *m_detailsContentLayout;
    
    QLabel *m_detailSubjectLabel;
    QLabel *m_detailTypeLabel;
    MarkdownRenderer *m_detailQuestionRenderer;  // 替换QLabel
    QLabel *m_detailImageLabel;
    MarkdownRenderer *m_detailChoicesRenderer;   // 替换QLabel
    MarkdownRenderer *m_detailCorrectAnswerRenderer;  // 替换QLabel
    MarkdownRenderer *m_detailUserAnswerRenderer;     // 替换QLabel
    QLabel *m_detailTimestampLabel;
    QLabel *m_detailReviewCountLabel;
    QLabel *m_detailStatusLabel;
    
    // Current selection
    int m_currentRecordIndex;
};

#endif // REVIEWWIDGET_H