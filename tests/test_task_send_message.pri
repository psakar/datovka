
QT += svg
QT += sql
QT += widgets

DEFINES += \
	TEST_TASK_SEND_MESSAGE=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/common.cpp \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pin.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pwd.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.cpp \
	$${top_srcdir}src/datovka_shared/graphics/graphics.cpp \
	$${top_srcdir}src/datovka_shared/io/records_management_db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.cpp \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/delegates/tag_item.cpp \
	$${top_srcdir}src/dimensions/dimensions.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/io/account_db.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/dbs.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/io/message_db.cpp \
	$${top_srcdir}src/io/message_db_set.cpp \
	$${top_srcdir}src/io/tag_db.cpp \
	$${top_srcdir}src/isds/isds_conversion.cpp \
	$${top_srcdir}src/models/accounts_model.cpp \
	$${top_srcdir}src/models/files_model.cpp \
	$${top_srcdir}src/models/messages_model.cpp \
	$${top_srcdir}src/models/table_model.cpp \
	$${top_srcdir}src/settings/account.cpp \
	$${top_srcdir}src/settings/accounts.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/worker/task.cpp \
	$${top_srcdir}src/worker/task_send_message.cpp \
	$${top_srcdir}tests/test_task_send_message.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pin.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pwd.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.h \
	$${top_srcdir}src/datovka_shared/graphics/graphics.h \
	$${top_srcdir}src/datovka_shared/io/records_management_db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.h \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/delegates/tag_item.h \
	$${top_srcdir}src/dimensions/dimensions.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/io/account_db.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/dbs.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/io/message_db.h \
	$${top_srcdir}src/io/message_db_set.h \
	$${top_srcdir}src/io/tag_db.h \
	$${top_srcdir}src/isds/isds_conversion.h \
	$${top_srcdir}src/models/accounts_model.h \
	$${top_srcdir}src/models/files_model.h \
	$${top_srcdir}src/models/messages_model.h \
	$${top_srcdir}src/models/table_model.h \
	$${top_srcdir}src/settings/account.h \
	$${top_srcdir}src/settings/accounts.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/worker/message_emitter.h \
	$${top_srcdir}src/worker/task.h \
	$${top_srcdir}src/worker/task_send_message.h \
	$${top_srcdir}tests/test_task_send_message.h
