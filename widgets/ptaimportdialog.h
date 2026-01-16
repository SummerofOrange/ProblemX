#ifndef PTAIMPORTDIALOG_H
#define PTAIMPORTDIALOG_H

#include <QDialog>
#include <QString>

class ConfigManager;
class QWebEngineView;
class QPushButton;
class QSplitter;
class QPlainTextEdit;
class BankEditorWidget;
class QWidget;
class QVBoxLayout;
class QHBoxLayout;
class QCheckBox;

class PtaImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PtaImportDialog(ConfigManager *configManager, const QString &subjectName, QWidget *parent = nullptr);
    ~PtaImportDialog();

private slots:
    void parseChoice();
    void parseTrueOrFalse();
    void parseFillBlank();
    void saveCurrentBank();

private:
    void logLine(const QString &line);
    void loadParsedJsonToEditor(const QString &jsonText, const QString &typeStr);
    QString createTempBankFilePath(const QString &typeStr) const;
    QString typeToFolder(const QString &typeStr) const;
    bool downloadImagesAndRewriteJson(const QString &jsonPath);
    QString pickUniqueAssetFilePath(const QString &assetDir, const QString &fileName) const;

    ConfigManager *m_configManager;
    QString m_subjectName;
    QString m_subjectPath;

    QSplitter *m_splitter;
    QWidget *m_leftPanel;
    QVBoxLayout *m_leftLayout;
    QWebEngineView *m_webView;

    QWidget *m_rightPanel;
    QVBoxLayout *m_rightLayout;
    QHBoxLayout *m_parseButtonLayout;
    QPushButton *m_parseChoiceButton;
    QPushButton *m_parseTrueOrFalseButton;
    QPushButton *m_parseFillBlankButton;

    QPlainTextEdit *m_output;
    BankEditorWidget *m_editor;

    QHBoxLayout *m_bottomLayout;
    QCheckBox *m_downloadImagesCheckBox;
    QPushButton *m_saveButton;
    QPushButton *m_exitButton;

    QString m_currentType;
    QString m_tempFilePath;
};

#endif // PTAIMPORTDIALOG_H

