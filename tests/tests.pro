
QT += core gui network sql
QT += printsupport
QT += testlib

TEMPLATE = app
TARGET = tests

top_srcdir = ../

DEFINES += \
	DEBUG=1

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic

INCLUDEPATH += \
	$${top_srcdir}

LIBS += \
	-lcrypto

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
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/crypto/crypto_threads.cpp \
	$${top_srcdir}src/log/log.cpp \
	$${top_srcdir}src/log/log_c.cpp \
	$${top_srcdir}/tests/helper.c \
	$${top_srcdir}/tests/test_crypto.cpp \
	$${top_srcdir}/tests/tests.cpp

HEADERS = \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/crypto/crypto_threads.h \
	$${top_srcdir}src/log/log_c.h \
	$${top_srcdir}src/log/log_common.h \
	$${top_srcdir}src/log/log.h \
	$${top_srcdir}/tests/helper.h \
	$${top_srcdir}/tests/test_crypto.h \
