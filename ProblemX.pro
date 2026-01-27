QT       += core gui webenginewidgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets webenginewidgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    widgets/startwidget.cpp \
    widgets/configwidget.cpp \
    widgets/practicewidget.cpp \
    widgets/reviewwidget.cpp \
    widgets/bankeditorwidget.cpp \
    widgets/ptaimportdialog.cpp \
    widgets/questionassistantwidget.cpp \
    widgets/questionpreviewwidget.cpp \
    widgets/ptaassistcontroller.cpp \
    core/questionmanager.cpp \
    core/configmanager.cpp \
    core/practicemanager.cpp \
    core/wronganswerset.cpp \
    models/question.cpp \
    models/questionbank.cpp \
    utils/jsonutils.cpp \
    utils/bankscanner.cpp \
    utils/markdownrenderer.cpp \
    utils/textnormalize.cpp \
    utils/questionsearchindex.cpp

HEADERS += \
    mainwindow.h \
    widgets/startwidget.h \
    widgets/configwidget.h \
    widgets/practicewidget.h \
    widgets/reviewwidget.h \
    widgets/bankeditorwidget.h \
    widgets/ptaimportdialog.h \
    widgets/questionassistantwidget.h \
    widgets/questionpreviewwidget.h \
    widgets/ptaassistcontroller.h \
    core/questionmanager.h \
    core/configmanager.h \
    core/practicemanager.h \
    core/wronganswerset.h \
    models/question.h \
    models/questionbank.h \
    utils/jsonutils.h \
    utils/bankscanner.h \
    utils/markdownrenderer.h \
    utils/textnormalize.h \
    utils/questionsearchindex.h

FORMS += \
    mainwindow.ui

# Copy resources folder to build directory (same directory as exe)
CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/debug
    win32: QMAKE_POST_LINK += if not exist "$$shell_path($$OUT_PWD/debug/resources)" mkdir "$$shell_path($$OUT_PWD/debug/resources)" & xcopy /E /I /Y "$$shell_path($$PWD/resources)" "$$shell_path($$OUT_PWD/debug/resources)"
    unix: QMAKE_POST_LINK += mkdir -p $$OUT_PWD/debug/resources && cp -r $$PWD/resources/* $$OUT_PWD/debug/resources/
} else {
    DESTDIR = $$OUT_PWD/release
    win32: QMAKE_POST_LINK += if not exist "$$shell_path($$OUT_PWD/release/resources)" mkdir "$$shell_path($$OUT_PWD/release/resources)" & xcopy /E /I /Y "$$shell_path($$PWD/resources)" "$$shell_path($$OUT_PWD/release/resources)"
    unix: QMAKE_POST_LINK += mkdir -p $$OUT_PWD/release/resources && cp -r $$PWD/resources/* $$OUT_PWD/release/resources/
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
