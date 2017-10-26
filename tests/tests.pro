
QT += core
QT += gui
QT += network
QT += testlib

top_srcdir = ../

TEMPLATE = app
TARGET = tests

include($${top_srcdir}pri/version.pri)

DEFINES += \
	DEBUG=1 \
	VERSION=\\\"$${VERSION}\\\"

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic \
	-Wdate-time -Wformat -Werror=format-security

INCLUDEPATH += \
	$${top_srcdir}

macx {
	QMAKE_CXXFLAGS += -arch i386

	QMAKE_CXXFLAGS += -stdlib=libc++
	CONFIG += c++11

	INCLUDEPATH += \
		/usr/local/include \
		/opt/local/include
	LIBPATH += \
		/usr/local/lib \
		/opt/local/lib
}

# MOC files are generated only directly from *.cpp files when using testlib.
# Adding a custom compiler rule does not help.

SOURCES = \
	$${top_srcdir}src/log/log.cpp \
	$${top_srcdir}src/log/log_c.cpp \
	$${top_srcdir}tests/helper.c \
	$${top_srcdir}tests/helper_qt.cpp \
	$${top_srcdir}tests/tests.cpp

HEADERS = \
	$${top_srcdir}src/log/log_c.h \
	$${top_srcdir}src/log/log_common.h \
	$${top_srcdir}src/log/log.h \
	$${top_srcdir}tests/helper.h \
	$${top_srcdir}tests/helper_qt.h

include(test_crypto_message.pri)
include(test_crypto_pin.pri)
include(test_db_container.pri)
include(test_message_db_set.pri)
include(test_isds_login.pri)
include(test_task_send_message.pri)
include(test_task_downloads.pri)

# Replace possible double slashes with a single slash. Also remove duplicated
# entries.
TMP = ""
for(src, SOURCES): TMP += $$replace(src, //, /)
SOURCES = $$unique(TMP)
TMP = ""
for(hdr, HEADERS): TMP += $$replace(hdr, //, /)
HEADERS = $$unique(TMP)
