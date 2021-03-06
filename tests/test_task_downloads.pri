
QT += svg
QT += sql
QT += widgets

DEFINES += \
	TEST_TASK_DOWNLOADS=1

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
	$${top_srcdir}src/datovka_shared/crypto/crypto_trusted_certs.c \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.cpp \
	$${top_srcdir}src/datovka_shared/graphics/graphics.cpp \
	$${top_srcdir}src/datovka_shared/io/records_management_db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.cpp \
	$${top_srcdir}src/datovka_shared/isds/account_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/box_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/error.cpp \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.cpp \
	$${top_srcdir}src/datovka_shared/isds/message_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.cpp \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/datovka_shared/worker/pool.cpp \
	$${top_srcdir}src/delegates/tag_item.cpp \
	$${top_srcdir}src/dimensions/dimensions.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/gui/icon_container.cpp \
	$${top_srcdir}src/io/account_db.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/dbs.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/io/message_db.cpp \
	$${top_srcdir}src/io/message_db_set.cpp \
	$${top_srcdir}src/io/message_db_set_delegated.cpp \
	$${top_srcdir}src/io/sqlite/delayed_access_db.cpp \
	$${top_srcdir}src/io/tag_db.cpp \
	$${top_srcdir}src/isds/account_conversion.cpp \
	$${top_srcdir}src/isds/box_conversion.cpp \
	$${top_srcdir}src/isds/error_conversion.cpp \
	$${top_srcdir}src/isds/internal_type_conversion.cpp \
	$${top_srcdir}src/isds/message_conversion.cpp \
	$${top_srcdir}src/isds/services_box.cpp \
	$${top_srcdir}src/isds/services_login.cpp \
	$${top_srcdir}src/isds/services_message.cpp \
	$${top_srcdir}src/isds/session.cpp \
	$${top_srcdir}src/isds/type_description.cpp \
	$${top_srcdir}src/models/accounts_model.cpp \
	$${top_srcdir}src/models/messages_model.cpp \
	$${top_srcdir}src/models/table_model.cpp \
	$${top_srcdir}src/settings/account.cpp \
	$${top_srcdir}src/settings/accounts.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/settings/registry.cpp \
	$${top_srcdir}src/worker/task.cpp \
	$${top_srcdir}src/worker/task_download_message.cpp \
	$${top_srcdir}src/worker/task_download_message_list.cpp \
	$${top_srcdir}tests/test_task_downloads.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pin.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_pwd.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_trusted_certs.h \
	$${top_srcdir}src/datovka_shared/crypto/crypto_wrapped.h \
	$${top_srcdir}src/datovka_shared/graphics/graphics.h \
	$${top_srcdir}src/datovka_shared/io/records_management_db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.h \
	$${top_srcdir}src/datovka_shared/isds/account_interface.h \
	$${top_srcdir}src/datovka_shared/isds/box_interface.h \
	$${top_srcdir}src/datovka_shared/isds/error.h \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/message_interface.h \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/types.h \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/datovka_shared/worker/pool.h \
	$${top_srcdir}src/delegates/tag_item.h \
	$${top_srcdir}src/dimensions/dimensions.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/gui/icon_container.h \
	$${top_srcdir}src/io/account_db.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/dbs.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/io/message_db.h \
	$${top_srcdir}src/io/message_db_set.h \
	$${top_srcdir}src/io/sqlite/delayed_access_db.h \
	$${top_srcdir}src/io/tag_db.h \
	$${top_srcdir}src/isds/account_conversion.h \
	$${top_srcdir}src/isds/box_conversion.h \
	$${top_srcdir}src/isds/conversion_internal.h \
	$${top_srcdir}src/isds/error_conversion.h \
	$${top_srcdir}src/isds/internal_type_conversion.h \
	$${top_srcdir}src/isds/message_conversion.h \
	$${top_srcdir}src/isds/services.h \
	$${top_srcdir}src/isds/services_login.h \
	$${top_srcdir}src/isds/session.h \
	$${top_srcdir}src/isds/type_description.h \
	$${top_srcdir}src/models/accounts_model.h \
	$${top_srcdir}src/models/messages_model.h \
	$${top_srcdir}src/models/table_model.h \
	$${top_srcdir}src/settings/account.h \
	$${top_srcdir}src/settings/accounts.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/settings/registry.h \
	$${top_srcdir}src/worker/message_emitter.h \
	$${top_srcdir}src/worker/task.h \
	$${top_srcdir}src/worker/task_download_message.h \
	$${top_srcdir}src/worker/task_download_message_list.h \
	$${top_srcdir}tests/test_task_downloads.h
