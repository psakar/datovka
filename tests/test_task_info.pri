
QT += sql

DEFINES += \
	TEST_TASK_INFO=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.cpp \
	$${top_srcdir}src/datovka_shared/isds/account_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/box_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/error.cpp \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.cpp \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.cpp \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/io/account_db.cpp \
	$${top_srcdir}src/io/dbs.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/isds/account_conversion.cpp \
	$${top_srcdir}src/isds/box_conversion.cpp \
	$${top_srcdir}src/isds/internal_type_conversion.cpp \
	$${top_srcdir}src/isds/error_conversion.cpp \
	$${top_srcdir}src/isds/services_account.cpp \
	$${top_srcdir}src/isds/services_box.cpp \
	$${top_srcdir}src/isds/services_login.cpp \
	$${top_srcdir}src/isds/session.cpp \
	$${top_srcdir}src/isds/type_description.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/worker/task_download_credit_info.cpp \
	$${top_srcdir}src/worker/task_download_owner_info.cpp \
	$${top_srcdir}src/worker/task_download_password_info.cpp \
	$${top_srcdir}src/worker/task_download_user_info.cpp \
	$${top_srcdir}tests/test_task_info.cpp

HEADERS += \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.h \
	$${top_srcdir}src/datovka_shared/isds/account_interface.h \
	$${top_srcdir}src/datovka_shared/isds/box_interface.h \
	$${top_srcdir}src/datovka_shared/isds/error.h \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/types.h \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/io/account_db.h \
	$${top_srcdir}src/io/dbs.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/isds/account_conversion.h \
	$${top_srcdir}src/isds/box_conversion.h \
	$${top_srcdir}src/isds/internal_type_conversion.h \
	$${top_srcdir}src/isds/error_conversion.h \
	$${top_srcdir}src/isds/services.h \
	$${top_srcdir}src/isds/services_login.h \
	$${top_srcdir}src/isds/session.h \
	$${top_srcdir}src/isds/type_description.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/worker/message_emitter.h \
	$${top_srcdir}src/worker/task_download_credit_info.h \
	$${top_srcdir}src/worker/task_download_owner_info.h \
	$${top_srcdir}src/worker/task_download_password_info.h \
	$${top_srcdir}src/worker/task_download_user_info.h \
	$${top_srcdir}tests/test_task_info.h
