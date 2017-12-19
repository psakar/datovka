
DEFINES += \
	TEST_CRYPTO_PIN_TOKEN=1

LIBS += \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/common.cpp \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pin.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pwd.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.cpp \
	$${top_srcdir}src/datovka_shared/settings/pin.cpp \
	$${top_srcdir}src/settings/records_management.cpp \
	$${top_srcdir}tests/test_crypto_pin_token.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pin.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pwd.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.h \
	$${top_srcdir}src/datovka_shared/settings/pin.h \
	$${top_srcdir}src/settings/records_management.h \
	$${top_srcdir}tests/test_crypto_pin_token.h
