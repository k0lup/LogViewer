QT += widgets
CONFIG += c++11
TEMPLATE = app
TARGET = LogViewer

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/logparser.cpp \
    src/logmodel.cpp \
    src/logfilterproxymodel.cpp \
    src/highlightrule.cpp \
    src/highlightrulesdialog.cpp \
    src/highlightruleeditdialog.cpp

HEADERS += \
    src/mainwindow.h \
    src/logentry.h \
    src/logparser.h \
    src/logmodel.h \
    src/logfilterproxymodel.h \
    src/highlightrule.h \
    src/highlightrulesdialog.h \
    src/highlightruleeditdialog.h

RESOURCES +=

DEFINES += QT_DEPRECATED_WARNINGS
