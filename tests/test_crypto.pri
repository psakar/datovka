
DEFINES += \
	TEST_CRYPTO=1

LIBS += \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/crypto/crypto_threads.cpp \
	$${top_srcdir}tests/test_crypto.cpp

HEADERS += \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/crypto/crypto_threads.h \
	$${top_srcdir}tests/test_crypto.h
