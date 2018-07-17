
QT += svg
QT += sql
QT += widgets

DEFINES += \
	TEST_DB_CONTAINER=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/common.cpp \
	$${top_srcdir}src/crypto/crypto.c \
	$${top_srcdir}src/datovka_shared/graphics/graphics.cpp \
	$${top_srcdir}src/datovka_shared/io/records_management_db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.cpp \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.cpp \
	$${top_srcdir}src/datovka_shared/isds/message_interface.cpp \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.cpp \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/delegates/tag_item.cpp \
	$${top_srcdir}src/dimensions/dimensions.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/dbs.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/message_db.cpp \
	$${top_srcdir}src/io/message_db_set.cpp \
	$${top_srcdir}src/io/message_db_set_container.cpp \
	$${top_srcdir}src/io/tag_db.cpp \
	$${top_srcdir}src/isds/type_description.cpp \
	$${top_srcdir}src/models/messages_model.cpp \
	$${top_srcdir}src/models/table_model.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/settings/registry.cpp \
	$${top_srcdir}tests/test_db_container.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/datovka_shared/graphics/graphics.h \
	$${top_srcdir}src/datovka_shared/io/records_management_db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.h \
	$${top_srcdir}src/datovka_shared/isds/internal_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/message_interface.h \
	$${top_srcdir}src/datovka_shared/isds/type_conversion.h \
	$${top_srcdir}src/datovka_shared/isds/types.h \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/delegates/tag_item.h \
	$${top_srcdir}src/dimensions/dimensions.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/dbs.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/message_db.h \
	$${top_srcdir}src/io/message_db_set.h \
	$${top_srcdir}src/io/message_db_set_container.h \
	$${top_srcdir}src/io/tag_db.h \
	$${top_srcdir}src/isds/type_description.h \
	$${top_srcdir}src/models/messages_model.h \
	$${top_srcdir}src/models/table_model.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/settings/registry.h \
	$${top_srcdir}tests/test_db_container.h
