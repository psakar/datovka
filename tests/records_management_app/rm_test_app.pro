
QT += core network
QT += gui svg widgets

top_srcdir = ../..

TEMPLATE = app
APP_NAME = ds_test_app

include($${top_srcdir}/pri/version.pri)
include($${top_srcdir}/pri/check_qt_version.pri)

sufficientQtVersion(5, 2, 3, 2)

isEmpty(MOC_DIR) {
	MOC_DIR = gen_moc
}
isEmpty(OBJECTS_DIR) {
	OBJECTS_DIR = gen_objects
}
isEmpty(UI_DIR) {
	UI_DIR = gen_ui
}
CONFIG += object_parallel_to_source

DEFINES += \
	DEBUG=1 \
	VERSION=\\\"$${VERSION}\\\"

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic \
	-Wdate-time -Wformat -Werror=format-security

INCLUDEPATH += \
	$${top_srcdir}

SOURCES += \
	$${top_srcdir}/src/datovka_shared/localisation/localisation.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/conversion.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/io/records_management_connection.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/entry_error.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/helper.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/service_info.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/stored_files.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/upload_file.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/json/upload_hierarchy.cpp \
	$${top_srcdir}/src/datovka_shared/records_management/models/upload_hierarchy_proxy_model.cpp \
	$${top_srcdir}/src/records_management/models/upload_hierarchy_model.cpp \
	$${top_srcdir}/tests/records_management_app/gui/app.cpp \
	$${top_srcdir}/tests/records_management_app/gui/dialogue_service_info.cpp \
	$${top_srcdir}/tests/records_management_app/gui/dialogue_stored_files.cpp \
	$${top_srcdir}/tests/records_management_app/json/documents.cpp \
	main.cpp

HEADERS += \
	$${top_srcdir}/src/datovka_shared/localisation/localisation.h \
	$${top_srcdir}/src/datovka_shared/records_management/conversion.h \
	$${top_srcdir}/src/datovka_shared/records_management/io/records_management_connection.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/entry_error.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/helper.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/service_info.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/stored_files.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/upload_file.h \
	$${top_srcdir}/src/datovka_shared/records_management/json/upload_hierarchy.h \
	$${top_srcdir}/src/datovka_shared/records_management/models/upload_hierarchy_proxy_model.h \
	$${top_srcdir}/src/records_management/models/upload_hierarchy_model.h \
	$${top_srcdir}/tests/records_management_app/gui/app.h \
	$${top_srcdir}/tests/records_management_app/gui/dialogue_service_info.h \
	$${top_srcdir}/tests/records_management_app/gui/dialogue_stored_files.h \
	$${top_srcdir}/tests/records_management_app/json/documents.h

FORMS += \
	$${top_srcdir}/tests/records_management_app/ui/app.ui \
	$${top_srcdir}/tests/records_management_app/ui/dialogue_service_info.ui \
	$${top_srcdir}/tests/records_management_app/ui/dialogue_stored_files.ui

RESOURCES +=
TRANSLATIONS +=
OTHER_FILES +=
