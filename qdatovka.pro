#-------------------------------------------------
#
# Project created by QtCreator 2014-03-24T10:01:11
#
#-------------------------------------------------

QT += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#LIBISDS_PREFIX = "$$HOME/third_party/built"

TARGET = qdatovka
TEMPLATE = app

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic

macx {
	# See https://bugreports.qt-project.org/browse/QTBUG-28097
	# for further details.
	QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc+
	CONFIG +=c++11
}

INCLUDEPATH = \
	src \
	/usr/include/libxml2
#LIBPATH = \
#//	/usr/local/lib
LIBS = \
	-lisds

SOURCES += src/common.cpp \
    src/gui/datovka.cpp \
    src/gui/dlg_change_pwd.cpp \
    src/gui/dlg_contacts.cpp \
    src/gui/dlg_create_account.cpp \
    src/gui/dlg_ds_search.cpp \
    src/gui/dlg_preferences.cpp \
    src/gui/dlg_proxysets.cpp \
    src/gui/dlg_send_message.cpp \
    src/io/account_db.cpp \
    src/io/db_tables.cpp \
    src/io/dbs.cpp \
    src/io/isds_sessions.cpp \
    src/io/message_db.cpp \
    src/io/pkcs7.cpp \
    src/main.cpp \
    src/models/accounts_model.cpp

HEADERS += src/common.h \
    src/gui/datovka.h \
    src/gui/dlg_change_pwd.h \
    src/gui/dlg_contacts.h \
    src/gui/dlg_create_account.h \
    src/gui/dlg_ds_search.h \
    src/gui/dlg_preferences.h \
    src/gui/dlg_proxysets.h \
    src/gui/dlg_send_message.h \
    src/io/account_db.h \
    src/io/db_tables.h \
    src/io/dbs.h \
    src/io/isds_sessions.h \
    src/io/message_db.h \
    src/io/pkcs7.h \
    src/models/accounts_model.h

FORMS += src/gui/ui/datovka.ui \
    src/gui/ui/dlg_change_pwd.ui \
    src/gui/ui/dlg_contacts.ui \
    src/gui/ui/dlg_create_account.ui \
    src/gui/ui/dlg_ds_search.ui \
    src/gui/ui/dlg_preferences.ui \
    src/gui/ui/dlg_proxysets.ui \
    src/gui/ui/dlg_send_message.ui

RESOURCES += \
    res/resources.qrc

TRANSLATIONS += locale/datovka_en.ts \
    locale/datovka_cs.ts

OTHER_FILES +=
