
# TODO -- Modify the co de so that widgets are not heeded.
QT += sql
QT += widgets

DEFINES += \
	TEST_ISDS_LOGIN=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/common.cpp \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/crypto/crypto_pin.c \
	$${top_srcdir}src/crypto/crypto_pwd.c \
	$${top_srcdir}src/crypto/crypto_wrapped.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_login.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/settings/account.cpp \
	$${top_srcdir}src/settings/accounts.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}tests/test_isds_login.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/crypto/crypto_pin.h \
	$${top_srcdir}src/crypto/crypto_pwd.h \
	$${top_srcdir}src/crypto/crypto_wrapped.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_login.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/settings/account.h \
	$${top_srcdir}src/settings/accounts.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}tests/test_isds_login.h
