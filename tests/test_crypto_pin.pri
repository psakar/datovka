
DEFINES += \
	TEST_CRYPTO_PIN=1

LIBS += \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/crypto/crypto_pin.c \
	$${top_srcdir}src/crypto/crypto_pwd.c \
	$${top_srcdir}src/crypto/crypto_wrapped.cpp \
	$${top_srcdir}tests/test_crypto_pin.cpp

HEADERS += \
	$${top_srcdir}src/crypto/crypto_pin.h \
	$${top_srcdir}src/crypto/crypto_pwd.h \
	$${top_srcdir}src/crypto/crypto_wrapped.h \
	$${top_srcdir}tests/test_crypto_pin.h
