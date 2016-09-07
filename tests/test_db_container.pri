
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
	$${top_srcdir}src/delegates/tag_item.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/dbs.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/message_db.cpp \
	$${top_srcdir}src/io/message_db_set.cpp \
	$${top_srcdir}src/io/message_db_set_container.cpp \
	$${top_srcdir}src/io/sqlite/db.cpp \
	$${top_srcdir}src/io/sqlite/table.cpp \
	$${top_srcdir}src/io/tag_db.cpp \
	$${top_srcdir}src/models/files_model.cpp \
	$${top_srcdir}src/models/messages_model.cpp \
	$${top_srcdir}src/models/table_model.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}tests/test_db_container.cpp

HEADERS += \
	$${top_srcdir}src/common.h \
	$${top_srcdir}src/crypto/crypto.h \
	$${top_srcdir}src/crypto/crypto_funcs.h \
	$${top_srcdir}src/delegates/tag_item.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/dbs.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/message_db.h \
	$${top_srcdir}src/io/message_db_set.h \
	$${top_srcdir}src/io/message_db_set_container.h \
	$${top_srcdir}src/io/sqlite/db.h \
	$${top_srcdir}src/io/sqlite/table.h \
	$${top_srcdir}src/io/tag_db.h \
	$${top_srcdir}src/models/files_model.h \
	$${top_srcdir}src/models/messages_model.h \
	$${top_srcdir}src/models/table_model.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}tests/test_db_container.h
