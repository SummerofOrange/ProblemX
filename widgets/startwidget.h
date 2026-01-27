#ifndef STARTWIDGET_H
#define STARTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QPropertyAnimation;
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

class ConfigManager;

class StartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StartWidget(QWidget *parent = nullptr);
    ~StartWidget();

    void setConfigManager(ConfigManager *configManager);

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

public:
    void checkForCheckpoint();

private slots:
    void onStartPracticeClicked();
    void onResumePracticeClicked();
    void onConfigureClicked();
    void onReviewWrongAnswersClicked();
    void onAssistantClicked();
    void onAboutClicked();
    void onExitClicked();

signals:
    void startPracticeRequested();
    void configureRequested();
    void reviewWrongAnswersRequested();
    void assistantRequested();
    void aboutRequested();
    void exitRequested();
    void resumePracticeRequested();

private:
    void setupUI();
    void setupConnections();
    void setupAnimations();
    void applyStyles();
    void updateButtonStates();
    
    // UI Components
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_logoLabel;
    QPushButton *m_startButton;
    QPushButton *m_resumeButton;
    QPushButton *m_configButton;
    QPushButton *m_reviewButton;
    QPushButton *m_assistantButton;
    QPushButton *m_aboutButton;
    QPushButton *m_exitButton;
    
    // Layouts
    QVBoxLayout *m_mainLayout;
    QVBoxLayout *m_titleLayout;
    QVBoxLayout *m_buttonLayout;
    QHBoxLayout *m_logoLayout;
    
    // Animations
    QPropertyAnimation *m_fadeInAnimation;
    QGraphicsOpacityEffect *m_opacityEffect;
    
    // Data
    ConfigManager *m_configManager;
    bool m_hasCheckpoint;
};

#endif // STARTWIDGET_H
