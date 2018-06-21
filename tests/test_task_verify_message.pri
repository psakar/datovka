
QT += sql

DEFINES += \
	TEST_TASK_VERIFY_MESSAGE=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/common.cpp \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/datovka_shared/isds/account_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/box_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/error.cpp \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.cpp \
	$${top_srcdir}src/datovka_shared/isds/message_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.cpp \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/isds/account_conversion.cpp \
	$${top_srcdir}src/isds/box_conversion.cpp \
	$${top_srcdir}src/isds/error_conversion.cpp \
	$${top_srcdir}src/isds/internal_type_conversion.cpp \
	$${top_srcdir}src/isds/message_conversion.cpp \
	$${top_srcdir}src/isds/message_functions.cpp \
	$${top_srcdir}src/isds/services_box.cpp \
	$${top_srcdir}src/isds/services_login.cpp \
	$${top_srcdir}src/isds/services_message.cpp \
	$${top_srcdir}src/isds/session.cpp \
	$${top_srcdir}src/isds/type_description.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/worker/task_authenticate_message.cpp \
	$${top_srcdir}src/worker/task_verify_message.cpp \
	$${top_srcdir}tests/test_task_verify_message.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/datovka_shared/isds/account_interface.h \
	$${top_srcdir}src/datovka_shared/isds/box_interface.h \
	$${top_srcdir}src/datovka_shared/isds/error.h \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/message_interface.h \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/types.h \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/isds/account_conversion.h \
	$${top_srcdir}src/isds/box_conversion.h \
	$${top_srcdir}src/isds/conversion_internal.h \
	$${top_srcdir}src/isds/error_conversion.h \
	$${top_srcdir}src/isds/internal_type_conversion.h \
	$${top_srcdir}src/isds/message_conversion.h \
	$${top_srcdir}src/isds/message_functions.h \
	$${top_srcdir}src/isds/services.h \
	$${top_srcdir}src/isds/services_login.h \
	$${top_srcdir}src/isds/session.h \
	$${top_srcdir}src/isds/type_description.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/worker/message_emitter.h \
	$${top_srcdir}src/worker/task_authenticate_message.h \
	$${top_srcdir}src/worker/task_verify_message.h \
	$${top_srcdir}tests/test_task_verify_message.h
