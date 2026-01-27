#ifndef QUESTIONASSISTANTWIDGET_H
#define QUESTIONASSISTANTWIDGET_H

#include <QWidget>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QVector>

#include "../utils/questionsearchindex.h"

class ConfigManager;
class QPushButton;
class QTabWidget;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QuestionPreviewWidget;
class QuestionSearchIndex;
class QWebEngineView;
class QLineEdit;
class QToolButton;
class PtaAssistController;
class QDoubleSpinBox;
class QTextEdit;

struct ParsedPtaQuestion {
    QString id;

    QString label;
    QString type;
    QString question;
    QStringList choices;
    int blankNum = 0;
    QMap<QString, QString> images;
};

struct PtaCacheEntry {
    QVector<SearchHit> hits;
    int selectedDocIndex = -1;
    bool filled = false;
    bool noMatch = false;
    double bestScore = 0.0;
};

class QuestionAssistantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuestionAssistantWidget(QWidget *parent = nullptr);
    ~QuestionAssistantWidget();

    void setConfigManager(ConfigManager *configManager);

signals:
    void backRequested();

private:
    void setupUI();
    void setupConnections();
    void setupSearchTab();
    void setupPtaTab();
    bool ensureIndexReady();
    void updatePtaQuestionItemVisual(const QString &ptaId);
    QString buildPtaQueryText(const ParsedPtaQuestion &ptaQuestion) const;
    void startPtaAutoAnswer();
    void stopPtaAutoAnswer();
    void processNextPtaAuto();
    void exportPtaNewQuestions();

protected:
    void showEvent(QShowEvent *event) override;

    ConfigManager *m_configManager;
    QuestionSearchIndex *m_searchIndex;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QPushButton *m_backButton;
    QLabel *m_titleLabel;
    QTabWidget *m_tabs;
    QWidget *m_searchTab;
    QWidget *m_ptaTab;

    QLabel *m_indexStatusLabel;
    QPlainTextEdit *m_queryEdit;
    QSpinBox *m_topKSpinBox;
    QPushButton *m_searchButton;
    // QPushButton *m_rebuildIndexButton; // Removed
    QTreeWidget *m_resultsTree; // Changed from QListWidget
    QuestionPreviewWidget *m_previewWidget;

    PtaAssistController *m_ptaController;
    QWebEngineView *m_ptaWebView;
    QToolButton *m_ptaBackButton;
    QToolButton *m_ptaForwardButton;
    QToolButton *m_ptaReloadButton;
    QLineEdit *m_ptaAddressBar;
    QPushButton *m_ptaParseButton;
    QDoubleSpinBox *m_ptaThresholdSpinBox;
    QPushButton *m_ptaAutoAnswerButton;
    QPushButton *m_ptaStopAutoButton;
    QPushButton *m_ptaExportNewButton;
    QListWidget *m_ptaQuestionList;
    QTextEdit *m_logEdit; // Log area
    QLabel *m_ptaPageTipLabel;
    QuestionPreviewWidget *m_ptaCurrentQuestionPreview;
    QSpinBox *m_ptaTopKSpinBox;
    QPushButton *m_ptaSearchButton;
    QTreeWidget *m_ptaResultsTree; // Changed from QListWidget
    QPushButton *m_ptaFillButton;
    QuestionPreviewWidget *m_ptaSelectedBankPreview;

    QString m_currentPtaId;
    QHash<QString, ParsedPtaQuestion> m_ptaQuestions;
    QHash<QString, PtaCacheEntry> m_ptaCache;
    bool m_ptaAutoRunning = false;
    QStringList m_ptaAutoQueue;
    int m_ptaAutoPos = 0;
    bool m_firstShow = true; // Auto load flag

    void log(const QString &msg);
};

#endif // QUESTIONASSISTANTWIDGET_H
