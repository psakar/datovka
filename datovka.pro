#-------------------------------------------------
#
# Project created by QtCreator 2014-03-24T10:01:11
#
#-------------------------------------------------

QT += core gui network sql
QT += printsupport

# Generate localisation.
system(lrelease datovka.pro)

# Required Qt versions
REQUIRED_MAJOR = 5
REQUIRED_MINOR = 2

lessThan(QT_MAJOR_VERSION, $${REQUIRED_MAJOR}) {
	error(Qt version $${REQUIRED_MAJOR}.$${REQUIRED_MINOR} is required.)
} else {
	QT += widgets
}

isEqual(QT_MAJOR_VERSION, $${REQUIRED_MAJOR}) {
	lessThan(QT_MINOR_VERSION, $${REQUIRED_MINOR}) {
		error(Qt version $${REQUIRED_MAJOR}.$${REQUIRED_MINOR} is required.)
	}
} else {
	warning(The current Qt version $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION} may not work.)
}

#lessThan(QT_MAJOR_VERSION)

#LIBISDS_PREFIX = "$$HOME/third_party/built"

DEFINES += \
	DEBUG=1

isEmpty(PREFIX) {
	PREFIX = "/usr/local"
}

APP_NAME = datovka

application.target = $${APP_NAME}
application.path = "$${PREFIX}/bin"
application.files = $${APP_NAME}

LOCALE_INST_DIR = "$${PREFIX}/share/$${APP_NAME}/localisations"

localisation.path = "$${LOCALE_INST_DIR}"
localisation.files = locale/datovka_cs.qm \
	locale/datovka_en.qm

DEFINES += LOCALE_INST_DIR="\"\\\"$${LOCALE_INST_DIR}\\\"\""

TEMPLATE = app

INSTALLS += application
INSTALLS += localisation

TEMPLATE = app

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic

INCLUDEPATH = \
	src \
	/usr/include/libxml2

LIBS = \
	-lisds

macx {
	# See https://bugreports.qt-project.org/browse/QTBUG-28097
	# for further details.
	QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc+
	CONFIG += c++11
	INCLUDEPATH += /usr/local/include \
		/opt/local/include
	LIBPATH += /usr/local/lib \
		/opt/local/lib
	QMAKE_MAC_SDK=macosx10.9
}

win32 {
	DEFINES += WIN32=1

	INCLUDEPATH = \
		src \
		mingw32built/include/libxml2 \
		mingw32built/include/

	LIBS = \
		e:/Git/qdatovka/mingw32built/bin/libisds-5.dll \
		e:/Git/qdatovka/mingw32built/bin/libeay32.dll

	SOURCES += src/compat/compat_win.c

	HEADERS += src/compat/compat_win.h
} else {
	LIBS += \
		-lcrypto
}

SOURCES += src/common.cpp \
    src/crypto/crypto.c \
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
    src/log/log.cpp \
    src/log/log_c.cpp \
    src/main.cpp \
    src/models/accounts_model.cpp \
    src/gui/dlg_about.cpp \
    src/gui/dlg_view_zfo.cpp \
    src/thread/worker.cpp \
    src/gui/dlg_signature_detail.cpp \
    src/gui/dlg_change_directory.cpp \
    src/gui/dlg_correspondence_overview.cpp

HEADERS += src/common.h \
    src/crypto/crypto.h \
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
    src/log/log.h \
    src/log/log_c.h \
    src/log/log_common.h \
    src/models/accounts_model.h \
    src/gui/dlg_about.h \
    src/gui/dlg_view_zfo.h \
    src/thread/worker.h \
    src/gui/dlg_signature_detail.h \
    src/gui/dlg_change_directory.h \
    src/gui/dlg_correspondence_overview.h

FORMS += src/gui/ui/datovka.ui \
    src/gui/ui/dlg_change_pwd.ui \
    src/gui/ui/dlg_contacts.ui \
    src/gui/ui/dlg_create_account.ui \
    src/gui/ui/dlg_ds_search.ui \
    src/gui/ui/dlg_preferences.ui \
    src/gui/ui/dlg_proxysets.ui \
    src/gui/ui/dlg_send_message.ui \
    src/gui/ui/dlg_about.ui \
    src/gui/ui/dlg_view_zfo.ui \
    src/gui/ui/dlg_signature_detail.ui \
    src/gui/ui/dlg_change_directory.ui \
    src/gui/ui/dlg_correspondence_overview.ui

RESOURCES += \
    res/resources.qrc

TRANSLATIONS += locale/datovka_en.ts \
    locale/datovka_cs.ts

OTHER_FILES +=
