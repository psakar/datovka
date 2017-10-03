#-------------------------------------------------
#
# Project created by QtCreator 2014-03-24T10:01:11
#
#-------------------------------------------------

QT += core gui network svg sql widgets
QT += printsupport

TEMPLATE = app
APP_NAME = datovka

include(pri/version.pri)
include(pri/check_qt_version.pri)

# Generate localisation. Must be run manually.
#system(lrelease datovka.pro)

# Copy Qt localisation on architectures.
macx {
	warning(Copying Qt translation from $$[QT_INSTALL_DATA].)
	system(cp $$[QT_INSTALL_DATA]/translations/qtbase_cs.qm locale/qtbase_cs.qm)
}
win32 {
	warning(Copying Qt translation from $$[QT_INSTALL_DATA].)
	system(copy $$[QT_INSTALL_DATA]/translations/qtbase_cs.qm locale/qtbase_cs.qm)
}

# Qt 5.2.1 contains a bug causing the application to crash on some drop events.
# Version 5.3.2 should be fine.
sufficientQtVersion(5, 3, 3, 2)

DEFINES += \
	DEBUG=1 \
	VERSION=\\\"$${VERSION}\\\"

unix:!macx {
	isEmpty(PREFIX) {
		PREFIX = "/usr/local"
	}

	BINDIR="$${PREFIX}/bin"
	DATADIR="$${PREFIX}/share"

	isEmpty(TEXT_FILES_INST_DIR) {
		TEXT_FILES_INST_DIR = "$${DATADIR}/doc/$${APP_NAME}"
	}
	LOCALE_INST_DIR = "$${DATADIR}/$${APP_NAME}/localisations"

	application.target = $${APP_NAME}
	application.path = "$${BINDIR}"
	application.files = $${APP_NAME}

	desktop.path = "$${DATADIR}/applications"
	desktop.files += deployment/datovka.desktop

	appdata.path = "$${DATADIR}/appdata"
	appdata.files += deployment/datovka.appdata.xml

	icon16.path = "$${DATADIR}/icons/hicolor/16x16/apps"
	icon16.files += "res/icons/16x16/datovka.png"

	icon24.path = "$${DATADIR}/icons/hicolor/24x24/apps"
	icon24.files += "res/icons/24x24/datovka.png"

	icon32.path = "$${DATADIR}/icons/hicolor/32x32/apps"
	icon32.files += "res/icons/32x32/datovka.png"

	icon48.path = "$${DATADIR}/icons/hicolor/48x48/apps"
	icon48.files += "res/icons/48x48/datovka.png"

	icon64.path = "$${DATADIR}/icons/hicolor/64x64/apps"
	icon64.files += "res/icons/64x64/datovka.png"

	icon128.path = "$${DATADIR}/icons/hicolor/128x128/apps"
	icon128.files += "res/icons/128x128/datovka.png"

	icon256.path = "$${DATADIR}/icons/hicolor/256x256/apps"
	icon256.files += "res/icons/256x256/datovka.png"

	localisation.path = "$${LOCALE_INST_DIR}"
	localisation.files += locale/datovka_cs.qm \
		locale/datovka_en.qm

	additional.path = "$${TEXT_FILES_INST_DIR}"
	additional.files = \
		AUTHORS \
		COPYING

	DEFINES += DATADIR=\\\"$$DATADIR\\\" \
		PKGDATADIR=\\\"$$PKGDATADIR\\\" \
		LOCALE_INST_DIR="\"\\\"$${LOCALE_INST_DIR}\\\"\"" \
		TEXT_FILES_INST_DIR="\"\\\"$${TEXT_FILES_INST_DIR}\\\"\""

	# Portable version cannot be installed.
	isEmpty(PORTABLE_APPLICATION) {
		INSTALLS += application \
			desktop \
			appdata \
			icon16 \
			icon24 \
			icon32 \
			icon48 \
			icon64 \
			icon128 \
			icon256 \
			localisation \
			additional
	}
}

QMAKE_CXXFLAGS = \
	-g -O0 -std=c++11 \
	-Wall -Wextra -pedantic \
	-Wdate-time -Wformat -Werror=format-security \
	-Isrc/views

INCLUDEPATH += \
	src

LIBS = \
	-lisds

isEqual(STATIC, 1) {
	warning(Linking statically.)
} else {
	INCLUDEPATH += \
		/usr/include/libxml2
}

!isEmpty(PORTABLE_APPLICATION) {
	warning(Building portable version.)
	DEFINES += PORTABLE_APPLICATION=1
	TARGET = $${APP_NAME}-portable
}

!isEmpty(DISABLE_VERSION_CHECK_BY_DEFAULT) {
	warning(Disabling version check by default.)
	DEFINES += DISABLE_VERSION_CHECK_BY_DEFAULT=1
}

macx {
	ICON = res/datovka.icns

	OSX_MIN="10.6" # Qt 5.5.1 and earlier.
	isEqual(QT_MAJOR_VERSION, 5) {
		greaterThan(QT_MINOR_VERSION, 5) {
			OSX_MIN="10.7" # Qt 5.6.0.
		}
	}

	# See https://bugreports.qt.io/browse/QTBUG-28097
	# for further details.
	QMAKE_CXXFLAGS += -mmacosx-version-min=$$OSX_MIN
	QMAKE_CXXFLAGS += -stdlib=libc++
	CONFIG += c++11
	isEmpty(SDK_VER) {
		# There is no way how to pass this variable into lrelease so
		# it must be set manually.
		SDK_VER = 10.11
	}
	QMAKE_MAC_SDK = macosx$${SDK_VER}
	QMAKE_MACOSX_DEPLOYMENT_TARGET = $$OSX_MIN

	QMAKE_INFO_PLIST = deployment/datovka.plist
	SED_EXT = -e
	QMAKE_POST_LINK += sed -i "$${SED_EXT}" "s/@VERSION@/$${VERSION}/g" "./$${TARGET}.app/Contents/Info.plist";
	QMAKE_POST_LINK += rm -f "./$${TARGET}.app/Contents/Info.plist$${SED_EXT}";

	isEqual(STATIC, 1) {
		QMAKE_CXXFLAGS += -arch i386

		INCLUDEPATH += \
			libs_static/built/include \
			libs_static/built/include/libxml2
		LIBPATH += \
			libs_static/built/lib

		LIBS += \
			-lexpat \
			-lxml2 \
			-lcurl \
			-liconv
	} else {
		INCLUDEPATH += /usr/local/include \
			/opt/local/include
		LIBPATH += /usr/local/lib \
			/opt/local/lib
	}

	localisation.path = "Contents/Resources/locale"
	localisation.files += locale/datovka_cs.qm \
		locale/datovka_en.qm
	localisation.files += locale/qtbase_cs.qm

	additional.path = "Contents/Resources"
	additional.files = \
		AUTHORS \
		COPYING \
		ChangeLog

	QMAKE_BUNDLE_DATA +=\
		localisation \
		additional
}

win32 {
        RC_FILE += res/icon.rc

	DEFINES += WIN32=1

	INCLUDEPATH = \
		src \
		mingw32built/include/libxml2 \
		mingw32built/include/

	LIBS = \
		$${_PRO_FILE_PWD_}/mingw32built/bin/libisds-5.dll \
		$${_PRO_FILE_PWD_}/mingw32built/bin/libeay32.dll

	SOURCES += src/compat/compat_win.c

	HEADERS += src/compat/compat_win.h
} else {
	LIBS += \
		-lcrypto
}

SOURCES += \
    src/about.cpp \
    src/cli/cli.cpp \
    src/cli/cli_login.cpp \
    src/cli/cli_parser.cpp \
    src/common.cpp \
    src/crypto/crypto.c \
    src/crypto/crypto_threads.cpp \
    src/crypto/crypto_version.cpp \
    src/delegates/tag_item.cpp \
    src/delegates/tags_delegate.cpp \
    src/dimensions/dimensions.cpp \
    src/graphics/graphics.cpp \
    src/gui/datovka.cpp \
    src/gui/dlg_about.cpp \
    src/gui/dlg_account_from_db.cpp \
    src/gui/dlg_change_directory.cpp \
    src/gui/dlg_change_pwd.cpp \
    src/gui/dlg_contacts.cpp \
    src/gui/dlg_correspondence_overview.cpp \
    src/gui/dlg_create_account.cpp \
    src/gui/dlg_ds_search.cpp \
    src/gui/dlg_import_zfo.cpp \
    src/gui/dlg_import_zfo_result.cpp \
    src/gui/dlg_msg_box_informative.cpp \
    src/gui/dlg_msg_search.cpp \
    src/gui/dlg_preferences.cpp \
    src/gui/dlg_proxysets.cpp \
    src/gui/dlg_send_message.cpp \
    src/gui/dlg_signature_detail.cpp \
    src/gui/dlg_tag.cpp \
    src/gui/dlg_tags.cpp \
    src/gui/dlg_timestamp_expir.cpp \
    src/gui/dlg_view_zfo.cpp \
    src/gui/dlg_yes_no_checkbox.cpp \
    src/initialisation.cpp \
    src/io/account_db.cpp \
    src/io/db_tables.cpp \
    src/io/dbs.cpp \
    src/io/exports.cpp \
    src/io/file_downloader.cpp \
    src/io/filesystem.cpp \
    src/io/imports.cpp \
    src/io/isds_helper.cpp \
    src/io/isds_login.cpp \
    src/io/isds_sessions.cpp \
    src/io/message_db.cpp \
    src/io/message_db_set.cpp \
    src/io/message_db_set_container.cpp \
    src/io/message_db_set_delegated.cpp \
    src/io/message_db_single.cpp \
    src/io/records_management_db.cpp \
    src/io/sqlite/db.cpp \
    src/io/sqlite/table.cpp \
    src/io/tag_db.cpp \
    src/isds/isds_conversion.cpp \
    src/localisation/localisation.cpp \
    src/log/log.cpp \
    src/log/log_c.cpp \
    src/main.cpp \
    src/model_interaction/account_interaction.cpp \
    src/model_interaction/attachment_interaction.cpp \
    src/models/accounts_model.cpp \
    src/models/combo_box_model.cpp \
    src/models/data_box_contacts_model.cpp \
    src/models/files_model.cpp \
    src/models/messages_model.cpp \
    src/models/sort_filter_proxy_model.cpp \
    src/models/table_model.cpp \
    src/models/tags_model.cpp \
    src/records_management/conversion.cpp \
    src/records_management/gui/dlg_records_management.cpp \
    src/records_management/gui/dlg_records_management_stored.cpp \
    src/records_management/gui/dlg_records_management_upload.cpp \
    src/records_management/io/records_management_connection.cpp \
    src/records_management/json/entry_error.cpp \
    src/records_management/json/helper.cpp \
    src/records_management/json/service_info.cpp \
    src/records_management/json/stored_files.cpp \
    src/records_management/json/upload_file.cpp \
    src/records_management/json/upload_hierarchy.cpp \
    src/records_management/models/upload_hierarchy_model.cpp \
    src/records_management/models/upload_hierarchy_proxy_model.cpp \
    src/records_management/widgets/svg_view.cpp \
    src/settings/account.cpp \
    src/settings/accounts.cpp \
    src/settings/preferences.cpp \
    src/settings/proxy.cpp \
    src/settings/records_management.cpp \
    src/single/single_instance.cpp \
    src/views/attachment_table_view.cpp \
    src/views/lowered_table_view.cpp \
    src/views/lowered_table_widget.cpp \
    src/views/lowered_tree_view.cpp \
    src/views/table_home_end_filter.cpp \
    src/views/table_key_press_filter.cpp \
    src/views/table_space_selection_filter.cpp \
    src/worker/message_emitter.cpp \
    src/worker/pool.cpp \
    src/worker/task.cpp \
    src/worker/task_authenticate_message.cpp \
    src/worker/task_change_pwd.cpp \
    src/worker/task_download_credit_info.cpp \
    src/worker/task_download_message.cpp \
    src/worker/task_download_message_list.cpp \
    src/worker/task_download_owner_info.cpp \
    src/worker/task_download_password_info.cpp \
    src/worker/task_download_user_info.cpp \
    src/worker/task_erase_message.cpp \
    src/worker/task_import_message.cpp \
    src/worker/task_import_zfo.cpp \
    src/worker/task_keep_alive.cpp \
    src/worker/task_records_management_stored_messages.cpp \
    src/worker/task_search_owner.cpp \
    src/worker/task_search_owner_fulltext.cpp \
    src/worker/task_send_message.cpp \
    src/worker/task_split_db.cpp \
    src/worker/task_vacuum_db_set.cpp \
    src/worker/task_verify_message.cpp

HEADERS += \
    src/about.h \
    src/cli/cli.h \
    src/cli/cli_login.h \
    src/cli/cli_parser.h \
    src/common.h \
    src/crypto/crypto.h \
    src/crypto/crypto_funcs.h \
    src/crypto/crypto_threads.h \
    src/crypto/crypto_version.h \
    src/delegates/tag_item.h \
    src/delegates/tags_delegate.h \
    src/dimensions/dimensions.h \
    src/graphics/graphics.h \
    src/gui/datovka.h \
    src/gui/dlg_about.h \
    src/gui/dlg_account_from_db.h \
    src/gui/dlg_change_directory.h \
    src/gui/dlg_change_pwd.h \
    src/gui/dlg_contacts.h \
    src/gui/dlg_correspondence_overview.h \
    src/gui/dlg_create_account.h \
    src/gui/dlg_ds_search.h \
    src/gui/dlg_import_zfo.h \
    src/gui/dlg_import_zfo_result.h \
    src/gui/dlg_msg_box_informative.h \
    src/gui/dlg_msg_search.h \
    src/gui/dlg_preferences.h \
    src/gui/dlg_proxysets.h \
    src/gui/dlg_send_message.h \
    src/gui/dlg_signature_detail.h \
    src/gui/dlg_tag.h \
    src/gui/dlg_tags.h \
    src/gui/dlg_timestamp_expir.h \
    src/gui/dlg_view_zfo.h \
    src/gui/dlg_yes_no_checkbox.h \
    src/initialisation.h \
    src/io/account_db.h \
    src/io/db_tables.h \
    src/io/dbs.h \
    src/io/exports.h \
    src/io/file_downloader.h \
    src/io/filesystem.h \
    src/io/imports.h \
    src/io/isds_helper.h \
    src/io/isds_login.h \
    src/io/isds_sessions.h \
    src/io/message_db.h \
    src/io/message_db_set.h \
    src/io/message_db_set_container.h \
    src/io/message_db_single.h \
    src/io/records_management_db.h \
    src/io/sqlite/db.h \
    src/io/sqlite/table.h \
    src/io/tag_db.h \
    src/isds/isds_conversion.h \
    src/localisation/localisation.h \
    src/log/log_c.h \
    src/log/log_common.h \
    src/log/log.h \
    src/model_interaction/account_interaction.h \
    src/model_interaction/attachment_interaction.h \
    src/models/accounts_model.h \
    src/models/combo_box_model.h \
    src/models/data_box_contacts_model.h \
    src/models/files_model.h \
    src/models/messages_model.h \
    src/models/sort_filter_proxy_model.h \
    src/models/table_model.h \
    src/models/tags_model.h \
    src/records_management/conversion.h \
    src/records_management/gui/dlg_records_management.h \
    src/records_management/gui/dlg_records_management_stored.h \
    src/records_management/gui/dlg_records_management_upload.h \
    src/records_management/io/records_management_connection.h \
    src/records_management/json/entry_error.h \
    src/records_management/json/helper.h \
    src/records_management/json/service_info.h \
    src/records_management/json/stored_files.h \
    src/records_management/json/upload_file.h \
    src/records_management/json/upload_hierarchy.h \
    src/records_management/models/upload_hierarchy_model.h \
    src/records_management/models/upload_hierarchy_proxy_model.h \
    src/records_management/widgets/svg_view.h \
    src/settings/account.h \
    src/settings/accounts.h \
    src/settings/preferences.h \
    src/settings/proxy.h \
    src/settings/records_management.h \
    src/single/single_instance.h \
    src/views/attachment_table_view.h \
    src/views/lowered_table_view.h \
    src/views/lowered_table_widget.h \
    src/views/lowered_tree_view.h \
    src/views/table_home_end_filter.h \
    src/views/table_key_press_filter.h \
    src/views/table_space_selection_filter.h \
    src/worker/message_emitter.h \
    src/worker/pool.h \
    src/worker/task.h \
    src/worker/task_authenticate_message.h \
    src/worker/task_change_pwd.h \
    src/worker/task_download_credit_info.h \
    src/worker/task_download_message.h \
    src/worker/task_download_message_list.h \
    src/worker/task_download_owner_info.h \
    src/worker/task_download_password_info.h \
    src/worker/task_download_user_info.h \
    src/worker/task_erase_message.h \
    src/worker/task_import_message.h \
    src/worker/task_import_zfo.h \
    src/worker/task_keep_alive.h \
    src/worker/task_records_management_stored_messages.h \
    src/worker/task_search_owner.h \
    src/worker/task_search_owner_fulltext.h \
    src/worker/task_send_message.h \
    src/worker/task_split_db.h \
    src/worker/task_vacuum_db_set.h \
    src/worker/task_verify_message.h

FORMS += \
    src/gui/ui/datovka.ui \
    src/gui/ui/dlg_about.ui \
    src/gui/ui/dlg_account_from_db.ui \
    src/gui/ui/dlg_change_directory.ui \
    src/gui/ui/dlg_change_pwd.ui \
    src/gui/ui/dlg_contacts.ui \
    src/gui/ui/dlg_correspondence_overview.ui \
    src/gui/ui/dlg_create_account.ui \
    src/gui/ui/dlg_ds_search.ui \
    src/gui/ui/dlg_import_zfo_result.ui \
    src/gui/ui/dlg_import_zfo.ui \
    src/gui/ui/dlg_msg_search.ui \
    src/gui/ui/dlg_preferences.ui \
    src/gui/ui/dlg_proxysets.ui \
    src/gui/ui/dlg_send_message.ui \
    src/gui/ui/dlg_signature_detail.ui \
    src/gui/ui/dlg_tag.ui \
    src/gui/ui/dlg_tags.ui \
    src/gui/ui/dlg_timestamp_expir.ui \
    src/gui/ui/dlg_view_zfo.ui \
    src/records_management/ui/dlg_records_management.ui \
    src/records_management/ui/dlg_records_management_stored.ui \
    src/records_management/ui/dlg_records_management_upload.ui

RESOURCES += \
    res/resources.qrc

TRANSLATIONS += locale/datovka_en.ts \
    locale/datovka_cs.ts

OTHER_FILES +=
