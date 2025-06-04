QT       += core gui webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webenginewidgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    widgets/startwidget.cpp \
    widgets/configwidget.cpp \
    widgets/practicewidget.cpp \
    widgets/reviewwidget.cpp \
    core/questionmanager.cpp \
    core/configmanager.cpp \
    core/practicemanager.cpp \
    core/wronganswerset.cpp \
    models/question.cpp \
    models/questionbank.cpp \
    utils/jsonutils.cpp \
    utils/bankscanner.cpp \
    utils/markdownrenderer.cpp

HEADERS += \
    mainwindow.h \
    widgets/startwidget.h \
    widgets/configwidget.h \
    widgets/practicewidget.h \
    widgets/reviewwidget.h \
    core/questionmanager.h \
    core/configmanager.h \
    core/practicemanager.h \
    core/wronganswerset.h \
    models/question.h \
    models/questionbank.h \
    utils/jsonutils.h \
    utils/bankscanner.h \
    utils/markdownrenderer.h \
    Reference/json.hpp

FORMS += \
    mainwindow.ui

INCLUDEPATH += Reference

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
