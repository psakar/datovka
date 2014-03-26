#-------------------------------------------------
#
# Project created by QtCreator 2014-03-24T10:01:11
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qdatovka
TEMPLATE = app


SOURCES += src/main.cpp\
    src/datovka.cpp \
    src/dlg_preferences.cpp \
    src/dlg_proxysets.cpp

HEADERS += src/datovka.h \
    src/dlg_preferences.h \
    src/dlg_proxysets.h

FORMS += ui/datovka.ui \
    ui/dlg_preferences.ui \
    ui/dlg_proxysets.ui

RESOURCES += \
    res/resources.qrc
