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
    src/preferences.cpp \
    src/proxysets.cpp

HEADERS  += src/datovka.h \
    src/preferences.h \
    src/proxysets.h

FORMS    += ui/datovka.ui \
    ui/preferences.ui \
    ui/proxysets.ui
