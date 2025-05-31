#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QList>
#include "models/question.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class StartWidget;
class ConfigWidget;
class PracticeWidget;
class ReviewWidget;
class ConfigManager;
class PracticeManager;
class WrongAnswerSet;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showStartWidget();
    void showConfigWidget();
    void showPracticeWidget();
    void showReviewWidget();
    void startPractice();
    void startReview(const QList<Question> &questions);
    void resumePractice();
    void onPracticeFinished();
    void onSaveAndExit();  // 处理保存并退出
    void onPracticeCompletedAndClearSave();  // 练习完成并清除存档
    // onPracticeAborted method removed as practiceAborted signal doesn't exist

private:
    void setupUI();
    void setupConnections();
    void initializeManagers();
    
    Ui::MainWindow *ui;
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QStackedWidget *m_stackedWidget;
    
    // Widgets
    StartWidget *m_startWidget;
    ConfigWidget *m_configWidget;
    PracticeWidget *m_practiceWidget;
    ReviewWidget *m_reviewWidget;
    
    // Managers
    ConfigManager *m_configManager;
    PracticeManager *m_practiceManager;
    WrongAnswerSet *m_wrongAnswerSet;
};

#endif // MAINWINDOW_H
