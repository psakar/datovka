
DEFINES += \
	TEST_CRYPTO_MESSAGE=1

LIBS += \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/crypto/crypto_threads.cpp \
	$${top_srcdir}src/crypto/crypto_version.cpp \
	$${top_srcdir}src/datovka_shared/crypto/crypto_trusted_certs.c \
	$${top_srcdir}tests/test_crypto_message.cpp

HEADERS += \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/crypto/crypto_threads.h \
	$${top_srcdir}src/crypto/crypto_version.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_trusted_certs.h \
	$${top_srcdir}tests/test_crypto_message.h
