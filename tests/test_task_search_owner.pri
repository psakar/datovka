
QT += sql

DEFINES += \
	TEST_TASK_SEARCH_OWNER=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}src/global.cpp \
	$${top_srcdir}src/io/filesystem.cpp \
	$${top_srcdir}src/io/isds_sessions.cpp \
	$${top_srcdir}src/isds/box_conversion.cpp \
	$${top_srcdir}src/isds/box_interface.cpp \
	$${top_srcdir}src/isds/error_conversion.cpp \
	$${top_srcdir}src/isds/error.cpp \
	$${top_srcdir}src/isds/internal_conversion.cpp \
	$${top_srcdir}src/isds/internal_type_conversion.cpp \
	$${top_srcdir}src/isds/services_box.cpp \
	$${top_srcdir}src/isds/type_conversion.cpp \
	$${top_srcdir}src/isds/type_description.cpp \
	$${top_srcdir}src/settings/preferences.cpp \
	$${top_srcdir}src/worker/task_search_owner_fulltext.cpp \
	$${top_srcdir}src/worker/task_search_owner.cpp \
	$${top_srcdir}tests/test_task_search_owner.cpp

HEADERS += \
	$${top_srcdir}src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}src/global.h \
	$${top_srcdir}src/io/filesystem.h \
	$${top_srcdir}src/io/isds_sessions.h \
	$${top_srcdir}src/isds/box_conversion.h \
	$${top_srcdir}src/isds/box_interface.h \
	$${top_srcdir}src/isds/error_conversion.h \
	$${top_srcdir}src/isds/error.h \
	$${top_srcdir}src/isds/internal_conversion.h \
	$${top_srcdir}src/isds/internal_type_conversion.h \
	$${top_srcdir}src/isds/services.h \
	$${top_srcdir}src/isds/type_conversion.h \
	$${top_srcdir}src/isds/type_description.h \
	$${top_srcdir}src/isds/types.h \
	$${top_srcdir}src/settings/preferences.h \
	$${top_srcdir}src/worker/message_emitter.h \
	$${top_srcdir}src/worker/task_search_owner_fulltext.h \
	$${top_srcdir}src/worker/task_search_owner.h \
	$${top_srcdir}tests/test_task_search_owner.h
