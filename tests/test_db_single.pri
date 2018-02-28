
QT += sql

DEFINES += \
	TEST_DB_SINGLE=1

SOURCES += \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.cpp \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/io/db_tables.cpp \
	$${top_srcdir}src/io/tag_db.cpp \
	$${top_srcdir}tests/test_db_single.cpp

HEADERS += \
	$${top_srcdir}src/datovka_shared/io/sqlite/db.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/db_single.h \
	$${top_srcdir}src/datovka_shared/io/sqlite/table.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/io/db_tables.h \
	$${top_srcdir}src/io/tag_db.h \
	$${top_srcdir}tests/test_db_single.h
