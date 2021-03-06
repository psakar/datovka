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
# Qt prior to version 5.4.1 may not behave correctly on some drop events.
# Version 5.4.1 should be fine.
sufficientQtVersion(5, 3, 4, 1)

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
	-Wdate-time -Wformat -Werror=format-security

#INCLUDEPATH +=

LIBS = \
	-lisds

isEqual(WITH_BUILT_LIBS, 1) {
	warning(Linking with locally built libraries.)
} else {
	INCLUDEPATH += \
		/usr/include/libxml2
}

isEqual(STATIC, 1) {
	warning(Linking statically.)
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

!isEmpty(DISABLE_VERSION_NOTIFICATION) {
	warning(Disabling version notification.)
	DEFINES += DISABLE_VERSION_NOTIFICATION=1
}

macx {
	ICON = res/datovka.icns

	OSX_MIN="10.6" # Qt 5.5.1 and earlier.
	isEqual(QT_MAJOR_VERSION, 5) {
		greaterThan(QT_MINOR_VERSION, 5) {
			OSX_MIN="10.7" # Qt 5.6.0.
		}
	}

	contains(QT_ARCH, x86_64):contains(QT_ARCH, i386) {
		error(Both architectures i386 and x86_64 specified.)
	} else:contains(QT_ARCH, x86_64) {
		QMAKE_CXXFLAGS += -arch x86_64
		message(Building for x86_64.)
	} else:contains(QT_ARCH, i386) {
		message(Building for i386.)
		QMAKE_CXXFLAGS += -arch i386
	} else {
		error(Unknown architecture.)
	}

	LIB_LOCATION=""
	isEqual(WITH_BUILT_LIBS, 1) {
		isEqual(STATIC, 1):contains(QT_ARCH, x86_64):!contains(QT_ARCH, i386) {
			LIB_LOCATION=libs/static_built_x86_64
		} else:isEqual(STATIC, 1):!contains(QT_ARCH, x86_64):contains(QT_ARCH, i386) {
			LIB_LOCATION=libs/static_built_i386
		} else:!isEqual(STATIC, 1):contains(QT_ARCH, x86_64):!contains(QT_ARCH, i386) {
			LIB_LOCATION=libs/shared_built_x86_64
		} else:!isEqual(STATIC, 1):!contains(QT_ARCH, x86_64):contains(QT_ARCH, i386) {
			LIB_LOCATION=libs/shared_built_i386
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

	!equals(LIB_LOCATION, "") {
		INCLUDEPATH += \
			$${LIB_LOCATION}/include \
			$${LIB_LOCATION}/include/libxml2
		LIBPATH += \
			$${LIB_LOCATION}/lib

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
        RC_FILE += res/datovka.rc

	VERSION_COMMAS=$$replace(VERSION, '\.', ',')
	DEFINES += \
		WIN32=1 \
		VERSION_COMMAS=$${VERSION_COMMAS} \
		INRC_FILEDESCR=\\\"Datovka\\\" \
		INRC_ORIG_FNAME=\\\"$${APP_NAME}.exe\\\"

	LIB_LOCATION=libs/shared_built

	INCLUDEPATH = \
		$${LIB_LOCATION}/include/ \
		$${LIB_LOCATION}/include/libxml2

	LIBS = \
		$${_PRO_FILE_PWD_}/$${LIB_LOCATION}/bin/libisds-5.dll \
		$${_PRO_FILE_PWD_}/$${LIB_LOCATION}/bin/libeay32.dll

	SOURCES += src/compat/compat_win.c

	HEADERS += src/compat/compat_win.h
} else {
	LIBS += \
		-lcrypto
}

!isEmpty(UNSPECIFIED_VARIABLE) {
	# This block of configuration code is only a hack. It is not intended
	# to be actually used in code configuration. Its sole purpose is
	# to trick the lupdate tool into reading header file content and
	# to force it to handle namespaces properly.
	# If this is not done then errors like:
	# 'Qualifying with unknown namespace/class ::Isds'
	# are being issued. The translator is then unable to find the proper
	# translation strings and some translations are thus not loaded by
	# the running application.
	# See e.g. https://stackoverflow.com/questions/6504902/lupdate-error-qualifying-with-unknown-namespace-class

	warning(UNSPECIFIED_VARIABLE set to \'$${UNSPECIFIED_VARIABLE}\'.)
	INCLUDEPATH += .
}

SOURCES += \
    src/about.cpp \
    src/cli/cli.cpp \
    src/cli/cli_login.cpp \
    src/cli/cli_parser.cpp \
    src/cli/cli_pin.cpp \
    src/cli/cmd_compose.cpp \
    src/cli/cmd_tokeniser.cpp \
    src/common.cpp \
    src/crypto/crypto.c \
    src/crypto/crypto_threads.cpp \
    src/crypto/crypto_version.cpp \
    src/crypto/cms/cms_lib.c \
    src/crypto/cms/cms_asn1.c \
    src/crypto/cms/cms_att.c \
    src/crypto/cms/cms_io.c \
    src/crypto/cms/cms_smime.c \
    src/crypto/cms/cms_err.c \
    src/crypto/cms/cms_sd.c \
    src/crypto/cms/cms_dd.c \
    src/crypto/cms/cms_cd.c \
    src/crypto/cms/cms_env.c \
    src/crypto/cms/cms_enc.c \
    src/crypto/cms/cms_ess.c \
    src/crypto/cms/cms_pwri.c \
    src/datovka_shared/crypto/crypto_pin.c \
    src/datovka_shared/crypto/crypto_pwd.c \
    src/datovka_shared/crypto/crypto_trusted_certs.c \
    src/datovka_shared/crypto/crypto_wrapped.cpp \
    src/datovka_shared/gov_services/helper.cpp \
    src/datovka_shared/gov_services/service/gov_mv_crr_vbh.cpp \
    src/datovka_shared/gov_services/service/gov_mv_ir_vp.cpp \
    src/datovka_shared/gov_services/service/gov_mv_rt_vt.cpp \
    src/datovka_shared/gov_services/service/gov_mv_rtpo_vt.cpp \
    src/datovka_shared/gov_services/service/gov_mv_skd_vp.cpp \
    src/datovka_shared/gov_services/service/gov_mv_vr_vp.cpp \
    src/datovka_shared/gov_services/service/gov_mv_zr_vp.cpp \
    src/datovka_shared/gov_services/service/gov_service_form_field.cpp \
    src/datovka_shared/gov_services/service/gov_service.cpp \
    src/datovka_shared/gov_services/service/gov_services_all.cpp \
    src/datovka_shared/gov_services/service/gov_szr_rob_vu.cpp \
    src/datovka_shared/gov_services/service/gov_szr_rob_vvu.cpp \
    src/datovka_shared/gov_services/service/gov_szr_ros_vv.cpp \
    src/datovka_shared/graphics/graphics.cpp \
    src/datovka_shared/io/records_management_db.cpp \
    src/datovka_shared/io/sqlite/db.cpp \
    src/datovka_shared/io/sqlite/db_single.cpp \
    src/datovka_shared/io/sqlite/table.cpp \
    src/datovka_shared/isds/account_interface.cpp \
    src/datovka_shared/isds/box_interface.cpp \
    src/datovka_shared/isds/error.cpp \
    src/datovka_shared/isds/internal_conversion.cpp \
    src/datovka_shared/isds/message_interface.cpp \
    src/datovka_shared/isds/type_conversion.cpp \
    src/datovka_shared/localisation/localisation.cpp \
    src/datovka_shared/log/global.cpp \
    src/datovka_shared/log/log.cpp \
    src/datovka_shared/log/log_c.cpp \
    src/datovka_shared/log/log_device.cpp \
    src/datovka_shared/log/memory_log.cpp \
    src/datovka_shared/records_management/conversion.cpp \
    src/datovka_shared/records_management/io/records_management_connection.cpp \
    src/datovka_shared/records_management/json/entry_error.cpp \
    src/datovka_shared/records_management/json/helper.cpp \
    src/datovka_shared/records_management/json/service_info.cpp \
    src/datovka_shared/records_management/json/stored_files.cpp \
    src/datovka_shared/records_management/json/upload_file.cpp \
    src/datovka_shared/records_management/json/upload_hierarchy.cpp \
    src/datovka_shared/records_management/models/upload_hierarchy_proxy_model.cpp \
    src/datovka_shared/settings/pin.cpp \
    src/datovka_shared/settings/records_management.cpp \
    src/datovka_shared/utility/strings.cpp \
    src/datovka_shared/worker/pool.cpp \
    src/delegates/tag_item.cpp \
    src/delegates/tags_delegate.cpp \
    src/dimensions/dimensions.cpp \
    src/global.cpp \
    src/gov_services/gui/dlg_gov_service.cpp \
    src/gov_services/gui/dlg_gov_services.cpp \
    src/gov_services/models/gov_service_list_model.cpp \
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
    src/gui/dlg_pin_input.cpp \
    src/gui/dlg_pin_setup.cpp \
    src/gui/dlg_preferences.cpp \
    src/gui/dlg_proxysets.cpp \
    src/gui/dlg_send_message.cpp \
    src/gui/dlg_signature_detail.cpp \
    src/gui/dlg_tag.cpp \
    src/gui/dlg_tags.cpp \
    src/gui/dlg_timestamp_expir.cpp \
    src/gui/dlg_view_log.cpp \
    src/gui/dlg_view_zfo.cpp \
    src/gui/dlg_yes_no_checkbox.cpp \
    src/gui/helper.cpp \
    src/gui/icon_container.cpp \
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
    src/io/sqlite/delayed_access_db.cpp \
    src/io/tag_db.cpp \
    src/isds/account_conversion.cpp \
    src/isds/box_conversion.cpp \
    src/isds/internal_type_conversion.cpp \
    src/isds/error_conversion.cpp \
    src/isds/message_conversion.cpp \
    src/isds/message_functions.cpp \
    src/isds/services_account.cpp \
    src/isds/services_box.cpp \
    src/isds/services_login.cpp \
    src/isds/services_message.cpp \
    src/isds/session.cpp \
    src/isds/to_text_conversion.cpp \
    src/isds/type_description.cpp \
    src/main.cpp \
    src/model_interaction/account_interaction.cpp \
    src/model_interaction/attachment_interaction.cpp \
    src/models/accounts_model.cpp \
    src/models/attachments_model.cpp \
    src/models/combo_box_model.cpp \
    src/models/data_box_contacts_model.cpp \
    src/models/messages_model.cpp \
    src/models/sort_filter_proxy_model.cpp \
    src/models/table_model.cpp \
    src/models/tags_model.cpp \
    src/records_management/gui/dlg_records_management.cpp \
    src/records_management/gui/dlg_records_management_stored.cpp \
    src/records_management/gui/dlg_records_management_upload.cpp \
    src/records_management/gui/dlg_records_management_upload_progress.cpp \
    src/records_management/models/upload_hierarchy_model.cpp \
    src/records_management/widgets/svg_view.cpp \
    src/settings/account.cpp \
    src/settings/accounts.cpp \
    src/settings/preferences.cpp \
    src/settings/proxy.cpp \
    src/settings/registry.cpp \
    src/single/single_instance.cpp \
    src/views/attachment_table_view.cpp \
    src/views/external_size_hint_tool_button.cpp \
    src/views/lowered_table_view.cpp \
    src/views/lowered_table_widget.cpp \
    src/views/lowered_tree_view.cpp \
    src/views/table_home_end_filter.cpp \
    src/views/table_key_press_filter.cpp \
    src/views/table_space_selection_filter.cpp \
    src/views/table_tab_ignore_filter.cpp \
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
    src/cli/cli_pin.h \
    src/cli/cmd_compose.h \
    src/cli/cmd_tokeniser.h \
    src/common.h \
    src/crypto/crypto.h \
    src/crypto/cms/cms.h \
    src/crypto/crypto_funcs.h \
    src/crypto/crypto_threads.h \
    src/crypto/crypto_version.h \
    src/datovka_shared/crypto/crypto_pin.h \
    src/datovka_shared/crypto/crypto_pwd.h \
    src/datovka_shared/crypto/crypto_trusted_certs.h \
    src/datovka_shared/crypto/crypto_wrapped.h \
    src/datovka_shared/gov_services/helper.h \
    src/datovka_shared/gov_services/service/gov_mv_crr_vbh.h \
    src/datovka_shared/gov_services/service/gov_mv_ir_vp.h \
    src/datovka_shared/gov_services/service/gov_mv_rt_vt.h \
    src/datovka_shared/gov_services/service/gov_mv_rtpo_vt.h \
    src/datovka_shared/gov_services/service/gov_mv_skd_vp.h \
    src/datovka_shared/gov_services/service/gov_mv_vr_vp.h \
    src/datovka_shared/gov_services/service/gov_mv_zr_vp.h \
    src/datovka_shared/gov_services/service/gov_service_form_field.h \
    src/datovka_shared/gov_services/service/gov_service.h \
    src/datovka_shared/gov_services/service/gov_services_all.h \
    src/datovka_shared/gov_services/service/gov_szr_rob_vu.h \
    src/datovka_shared/gov_services/service/gov_szr_rob_vvu.h \
    src/datovka_shared/gov_services/service/gov_szr_ros_vv.h \
    src/datovka_shared/graphics/graphics.h \
    src/datovka_shared/io/records_management_db.h \
    src/datovka_shared/io/sqlite/db.h \
    src/datovka_shared/io/sqlite/db_single.h \
    src/datovka_shared/io/sqlite/table.h \
    src/datovka_shared/isds/account_interface.h \
    src/datovka_shared/isds/box_interface.h \
    src/datovka_shared/isds/error.h \
    src/datovka_shared/isds/internal_conversion.h \
    src/datovka_shared/isds/message_interface.h \
    src/datovka_shared/isds/type_conversion.h \
    src/datovka_shared/isds/types.h \
    src/datovka_shared/localisation/localisation.h \
    src/datovka_shared/log/global.h \
    src/datovka_shared/log/log_c.h \
    src/datovka_shared/log/log_common.h \
    src/datovka_shared/log/log_device.h \
    src/datovka_shared/log/log.h \
    src/datovka_shared/log/memory_log.h \
    src/datovka_shared/records_management/conversion.h \
    src/datovka_shared/records_management/io/records_management_connection.h \
    src/datovka_shared/records_management/json/entry_error.h \
    src/datovka_shared/records_management/json/helper.h \
    src/datovka_shared/records_management/json/service_info.h \
    src/datovka_shared/records_management/json/stored_files.h \
    src/datovka_shared/records_management/json/upload_file.h \
    src/datovka_shared/records_management/json/upload_hierarchy.h \
    src/datovka_shared/records_management/models/upload_hierarchy_proxy_model.h \
    src/datovka_shared/settings/pin.h \
    src/datovka_shared/settings/records_management.h \
    src/datovka_shared/utility/strings.h \
    src/datovka_shared/worker/pool.h \
    src/delegates/tag_item.h \
    src/delegates/tags_delegate.h \
    src/dimensions/dimensions.h \
    src/global.h \
    src/gov_services/gui/dlg_gov_service.h \
    src/gov_services/gui/dlg_gov_services.h \
    src/gov_services/models/gov_service_list_model.h \
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
    src/gui/dlg_pin_input.h \
    src/gui/dlg_pin_setup.h \
    src/gui/dlg_preferences.h \
    src/gui/dlg_proxysets.h \
    src/gui/dlg_send_message.h \
    src/gui/dlg_signature_detail.h \
    src/gui/dlg_tag.h \
    src/gui/dlg_tags.h \
    src/gui/dlg_timestamp_expir.h \
    src/gui/dlg_view_log.h \
    src/gui/dlg_view_zfo.h \
    src/gui/dlg_yes_no_checkbox.h \
    src/gui/helper.h \
    src/gui/icon_container.h \
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
    src/io/sqlite/delayed_access_db.h \
    src/io/tag_db.h \
    src/isds/account_conversion.h \
    src/isds/box_conversion.h \
    src/isds/conversion_internal.h \
    src/isds/internal_type_conversion.h \
    src/isds/error_conversion.h \
    src/isds/message_conversion.h \
    src/isds/message_functions.h \
    src/isds/services.h \
    src/isds/services_internal.h \
    src/isds/services_login.h \
    src/isds/session.h \
    src/isds/to_text_conversion.h \
    src/isds/type_description.h \
    src/model_interaction/account_interaction.h \
    src/model_interaction/attachment_interaction.h \
    src/models/accounts_model.h \
    src/models/attachments_model.h \
    src/models/combo_box_model.h \
    src/models/data_box_contacts_model.h \
    src/models/messages_model.h \
    src/models/sort_filter_proxy_model.h \
    src/models/table_model.h \
    src/models/tags_model.h \
    src/records_management/gui/dlg_records_management.h \
    src/records_management/gui/dlg_records_management_stored.h \
    src/records_management/gui/dlg_records_management_upload.h \
    src/records_management/gui/dlg_records_management_upload_progress.h \
    src/records_management/models/upload_hierarchy_model.h \
    src/records_management/widgets/svg_view.h \
    src/settings/account.h \
    src/settings/accounts.h \
    src/settings/preferences.h \
    src/settings/proxy.h \
    src/settings/registry.h \
    src/single/single_instance.h \
    src/views/attachment_table_view.h \
    src/views/external_size_hint_tool_button.h \
    src/views/lowered_table_view.h \
    src/views/lowered_table_widget.h \
    src/views/lowered_tree_view.h \
    src/views/table_home_end_filter.h \
    src/views/table_key_press_filter.h \
    src/views/table_space_selection_filter.h \
    src/views/table_tab_ignore_filter.h \
    src/worker/message_emitter.h \
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
    src/gov_services/ui/dlg_gov_service.ui \
    src/gov_services/ui/dlg_gov_services.ui \
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
    src/gui/ui/dlg_pin_input.ui \
    src/gui/ui/dlg_pin_setup.ui \
    src/gui/ui/dlg_preferences.ui \
    src/gui/ui/dlg_proxysets.ui \
    src/gui/ui/dlg_send_message.ui \
    src/gui/ui/dlg_signature_detail.ui \
    src/gui/ui/dlg_tag.ui \
    src/gui/ui/dlg_tags.ui \
    src/gui/ui/dlg_timestamp_expir.ui \
    src/gui/ui/dlg_view_log.ui \
    src/gui/ui/dlg_view_zfo.ui \
    src/records_management/ui/dlg_records_management.ui \
    src/records_management/ui/dlg_records_management_progress.ui \
    src/records_management/ui/dlg_records_management_upload.ui

RESOURCES += \
    res/resources.qrc

TRANSLATIONS += locale/datovka_en.ts \
    locale/datovka_cs.ts

OTHER_FILES +=
