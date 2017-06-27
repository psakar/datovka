/*
 * Copyright (C) 2014-2017 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include <QDir>

#include "src/common.h"
#include "src/io/filesystem.h"
#include "src/settings/preferences.h"

GlobPreferences globPref;

/* Defaults. */
static const
GlobPreferences dlftlGlobPref;

/*! Default configuration folder location. */
#define DFLT_CONF_SUBDIR ".dsgui"
/*! Default configuration file name. */
#define DFLT_CONF_FILE "dsgui.conf"
/*! Account database file name. */
#define ACCOUNT_DB_FILE "messages.shelf.db"
#define TAG_DB_FILE "tag.db"
#define DOCUMENT_SERVICE_DB_FILE "document_service.db"
#define TAG_WEBDATOVKA_DB_FILE "mojeid-tag.db"

GlobPreferences::GlobPreferences(void)
    : confSubdir(DFLT_CONF_SUBDIR),
    loadFromConf(DFLT_CONF_FILE),
    saveToConf(DFLT_CONF_FILE),
    accountDbFile(ACCOUNT_DB_FILE),
    tagDbFile(TAG_DB_FILE),
    documentServiceDbFile(DOCUMENT_SERVICE_DB_FILE),
    tagWebDatovkaDbFile(TAG_WEBDATOVKA_DB_FILE),
    auto_download_whole_messages(false),
    default_download_signed(true),
    //store_passwords_on_disk(false),
    store_messages_on_disk(true),
    toolbar_button_style(Qt::ToolButtonTextUnderIcon),
    store_additional_data_on_disk(true),
    certificate_validation_date(DOWNLOAD_DATE),
    check_crl(true),
#ifdef DISABLE_VERSION_CHECK_BY_DEFAULT
    check_new_versions(false),
#else /* !DISABLE_VERSION_CHECK_BY_DEFAULT */
    check_new_versions(true),
#endif /* DISABLE_VERSION_CHECK_BY_DEFAULT */
    send_stats_with_version_checks(false),
    download_on_background(false),
    timer_value(10),
    download_at_start(false),
    date_format(DATE_FORMAT_DEFAULT),
    language("system"), /* Use local settings. */
    after_start_select(SELECT_NOTHING),
    message_mark_as_read_timeout(TIMER_MARK_MSG_READ_MS),
    use_global_paths(false),
    save_attachments_path(QDir::homePath()),
    add_file_to_attachments_path(QDir::homePath()),
    all_attachments_save_zfo_delinfo(false),
    all_attachments_save_zfo_msg(false),
    all_attachments_save_pdf_msgenvel(false),
    all_attachments_save_pdf_delinfo(false),
    message_filename_format(DEFAULT_MESSAGE_FILENAME_FORMAT),
    delivery_filename_format(DEFAULT_DELIVERY_FILENAME_FORMAT),
    attachment_filename_format(DEFAULT_ATTACHMENT_FILENAME_FORMAT),
    delivery_filename_format_all_attach(DEFAULT_DELIVERY_ATTACH_FORMAT),
    delivery_info_for_every_file(false),
    isds_download_timeout_ms(ISDS_DOWNLOAD_TIMEOUT_MS),
    timestamp_expir_before_days(TIMESTAMP_EXPIR_BEFORE_DAYS)
{
}

GlobPreferences::~GlobPreferences(void)
{
}

void GlobPreferences::loadFromSettings(const QSettings &settings)
{
	int value;

	auto_download_whole_messages = settings.value(
	    "preferences/auto_download_whole_messages",
	    dlftlGlobPref.auto_download_whole_messages).toBool();

	default_download_signed = settings.value(
	    "preferences/default_download_signed",
	    dlftlGlobPref.default_download_signed).toBool();

	store_messages_on_disk = settings.value(
	    "preferences/store_messages_on_disk",
	    dlftlGlobPref.store_messages_on_disk).toBool();

	store_additional_data_on_disk = settings.value(
	    "preferences/store_additional_data_on_disk",
	    dlftlGlobPref.store_additional_data_on_disk).toBool();

	download_on_background = settings.value(
	    "preferences/download_on_background",
	    dlftlGlobPref.download_on_background).toBool();

	download_at_start = settings.value(
	    "preferences/download_at_start",
	    dlftlGlobPref.download_at_start).toBool();

	toolbar_button_style = settings.value(
	    "preferences/toolbar_button_style",
	    dlftlGlobPref.toolbar_button_style).toInt();

	timer_value = settings.value(
	    "preferences/timer_value", dlftlGlobPref.timer_value).toInt();

	isds_download_timeout_ms = settings.value(
	    "preferences/isds_download_timeout_ms",
	    dlftlGlobPref.isds_download_timeout_ms).toInt();

	timestamp_expir_before_days = settings.value(
	    "preferences/timestamp_expir_before_days",
	    dlftlGlobPref.timestamp_expir_before_days).toInt();

	message_mark_as_read_timeout = settings.value(
	    "preferences/message_mark_as_read_timeout",
	    dlftlGlobPref.message_mark_as_read_timeout).toInt();

	use_global_paths = settings.value(
	    "preferences/use_global_paths",
	    dlftlGlobPref.use_global_paths).toBool();

	value = settings.value("preferences/certificate_validation_date",
	    dlftlGlobPref.certificate_validation_date).toInt();
	switch (value) {
	case DOWNLOAD_DATE:
		certificate_validation_date = DOWNLOAD_DATE;
		break;
	case CURRENT_DATE:
		certificate_validation_date = CURRENT_DATE;
		break;
	default:
		certificate_validation_date =
		    dlftlGlobPref.certificate_validation_date;
		Q_ASSERT(0);
		break;
	}

	check_crl = settings.value("preferences/check_crl",
	    dlftlGlobPref.check_crl).toBool();

	check_new_versions = settings.value("preferences/check_new_versions",
	    dlftlGlobPref.check_new_versions).toBool();

	delivery_info_for_every_file = settings.value(
	    "preferences/delivery_info_for_every_file",
	    dlftlGlobPref.delivery_info_for_every_file).toBool();

	send_stats_with_version_checks = settings.value(
	    "preferences/send_stats_with_version_checks",
	    dlftlGlobPref.send_stats_with_version_checks).toBool();

	value = settings.value("preferences/date_format",
	    dlftlGlobPref.date_format).toInt();
	switch (value) {
	case DATE_FORMAT_LOCALE:
		date_format = DATE_FORMAT_LOCALE;
		break;
	case DATE_FORMAT_ISO:
		date_format = DATE_FORMAT_ISO;
		break;
	case DATE_FORMAT_DEFAULT:
		date_format = DATE_FORMAT_DEFAULT;
		break;
	default:
		date_format = dlftlGlobPref.date_format;
		Q_ASSERT(0);
		break;
	}

	language = settings.value("preferences/language",
	    dlftlGlobPref.language).toString();
	if (language.isEmpty()) {
		language = dlftlGlobPref.language;
	}

	value = settings.value("preferences/after_start_select",
	    dlftlGlobPref.after_start_select).toInt();
	switch (value) {
	case SELECT_NEWEST:
		after_start_select = SELECT_NEWEST;
		break;
	case SELECT_LAST_VISITED:
		after_start_select = SELECT_LAST_VISITED;
		break;
	case SELECT_NOTHING:
		after_start_select = SELECT_NOTHING;
		break;
	default:
		after_start_select = dlftlGlobPref.after_start_select;
		Q_ASSERT(0);
		break;
	}

	save_attachments_path = settings.value(
	    "preferences/save_attachments_path",
	    dlftlGlobPref.save_attachments_path).toString();
	if (save_attachments_path.isEmpty()) {
		save_attachments_path = dlftlGlobPref.save_attachments_path;
	}

	add_file_to_attachments_path = settings.value(
	    "preferences/add_file_to_attachments_path",
	    dlftlGlobPref.add_file_to_attachments_path).toString();
	if (add_file_to_attachments_path.isEmpty()) {
		add_file_to_attachments_path =
		    dlftlGlobPref.add_file_to_attachments_path;
	}

	all_attachments_save_zfo_msg = settings.value(
	    "preferences/all_attachments_save_zfo_msg",
	    dlftlGlobPref.all_attachments_save_zfo_msg).toBool();

	all_attachments_save_zfo_delinfo = settings.value(
	    "preferences/all_attachments_save_zfo_delinfo",
	    dlftlGlobPref.all_attachments_save_zfo_delinfo).toBool();

	all_attachments_save_pdf_msgenvel = settings.value(
	    "preferences/all_attachments_save_pdf_msgenvel",
	    dlftlGlobPref.all_attachments_save_pdf_msgenvel).toBool();

	all_attachments_save_pdf_delinfo = settings.value(
	    "preferences/all_attachments_save_pdf_delinfo",
	    dlftlGlobPref.all_attachments_save_pdf_delinfo).toBool();

	message_filename_format = settings.value(
	    "preferences/message_filename_format",
	    dlftlGlobPref.message_filename_format).toString();

	delivery_filename_format = settings.value(
	    "preferences/delivery_filename_format",
	    dlftlGlobPref.delivery_filename_format).toString();

	attachment_filename_format = settings.value(
	    "preferences/attachment_filename_format",
	    dlftlGlobPref.attachment_filename_format).toString();

	delivery_filename_format_all_attach = settings.value(
	    "preferences/delivery_filename_format_all_attach",
	    dlftlGlobPref.delivery_filename_format_all_attach).toString();
}

void GlobPreferences::saveToSettings(QSettings &settings) const
{
	settings.beginGroup("preferences");

	/* Only values differing from defaults are written. */

	if (dlftlGlobPref.auto_download_whole_messages !=
	    auto_download_whole_messages) {
		settings.setValue("auto_download_whole_messages",
		    auto_download_whole_messages);
	}

	if (dlftlGlobPref.default_download_signed != default_download_signed) {
		settings.setValue("default_download_signed",
		    default_download_signed);
	}

	if (dlftlGlobPref.store_messages_on_disk != store_messages_on_disk) {
		settings.setValue("store_messages_on_disk",
		    store_messages_on_disk);
	}

	if (dlftlGlobPref.store_additional_data_on_disk !=
	    store_additional_data_on_disk) {
		settings.setValue("store_additional_data_on_disk",
		    store_additional_data_on_disk);
	}

	if (dlftlGlobPref.certificate_validation_date !=
	    certificate_validation_date) {
		settings.setValue("certificate_validation_date",
		    certificate_validation_date);
	}

	if (dlftlGlobPref.check_crl != check_crl) {
		settings.setValue("check_crl", check_crl);
	}

	if (dlftlGlobPref.check_new_versions != check_new_versions) {
		settings.setValue("check_new_versions", check_new_versions);
	}

	if (dlftlGlobPref.send_stats_with_version_checks !=
	    send_stats_with_version_checks) {
		settings.setValue("send_stats_with_version_checks",
		    send_stats_with_version_checks);
	}

	if (dlftlGlobPref.date_format != date_format) {
		settings.setValue("date_format", date_format);
	}

	if (dlftlGlobPref.language != language) {
		settings.setValue("language", language);
	}

	if (dlftlGlobPref.after_start_select != after_start_select) {
		settings.setValue("after_start_select", after_start_select);
	}

	if (dlftlGlobPref.toolbar_button_style != toolbar_button_style) {
		settings.setValue("toolbar_button_style", toolbar_button_style);
	}

	if (dlftlGlobPref.timer_value != timer_value) {
		settings.setValue("timer_value", timer_value);
	}

	if (dlftlGlobPref.isds_download_timeout_ms !=
	    isds_download_timeout_ms) {
		settings.setValue("isds_download_timeout_ms",
		    isds_download_timeout_ms);
	}

	if (dlftlGlobPref.timestamp_expir_before_days !=
	    timestamp_expir_before_days) {
		settings.setValue("timestamp_expir_before_days",
		    timestamp_expir_before_days);
	}

	if (dlftlGlobPref.message_mark_as_read_timeout !=
	    message_mark_as_read_timeout) {
		settings.setValue("message_mark_as_read_timeout",
		    message_mark_as_read_timeout);
	}

	if (dlftlGlobPref.download_on_background != download_on_background) {
		settings.setValue("download_on_background",
		    download_on_background);
	}

	if (dlftlGlobPref.download_at_start != download_at_start) {
		settings.setValue("download_at_start", download_at_start);
	}

	if (dlftlGlobPref.use_global_paths != use_global_paths) {
		settings.setValue("use_global_paths", use_global_paths);
	}

	if (dlftlGlobPref.save_attachments_path != save_attachments_path) {
		settings.setValue("save_attachments_path",
		    save_attachments_path);
	}
	if (dlftlGlobPref.add_file_to_attachments_path !=
	    add_file_to_attachments_path) {
		settings.setValue("add_file_to_attachments_path",
		    add_file_to_attachments_path);
	}

	if (dlftlGlobPref.all_attachments_save_zfo_msg !=
	    all_attachments_save_zfo_msg) {
		settings.setValue("all_attachments_save_zfo_msg",
		    all_attachments_save_zfo_msg);
	}

	if (dlftlGlobPref.all_attachments_save_zfo_delinfo !=
	    all_attachments_save_zfo_delinfo) {
		settings.setValue("all_attachments_save_zfo_delinfo",
		    all_attachments_save_zfo_delinfo);
	}


	if (dlftlGlobPref.all_attachments_save_pdf_msgenvel !=
	    all_attachments_save_pdf_msgenvel) {
		settings.setValue("all_attachments_save_pdf_msgenvel",
		    all_attachments_save_pdf_msgenvel);
	}

	if (dlftlGlobPref.all_attachments_save_pdf_delinfo !=
	    all_attachments_save_pdf_delinfo) {
		settings.setValue("all_attachments_save_pdf_delinfo",
		    all_attachments_save_pdf_delinfo);
	}

	if (dlftlGlobPref.message_filename_format !=
	    message_filename_format) {
		settings.setValue("message_filename_format",
		    message_filename_format);
	}

	if (dlftlGlobPref.delivery_filename_format !=
	    delivery_filename_format) {
		settings.setValue("delivery_filename_format",
		    delivery_filename_format);
	}

	if (dlftlGlobPref.attachment_filename_format !=
	    attachment_filename_format) {
		settings.setValue("attachment_filename_format",
		    attachment_filename_format);
	}

	if (dlftlGlobPref.delivery_filename_format_all_attach !=
	    delivery_filename_format_all_attach) {
		settings.setValue("delivery_filename_format_all_attach",
		    delivery_filename_format_all_attach);
	}

	if (dlftlGlobPref.delivery_info_for_every_file !=
	    delivery_info_for_every_file) {
		settings.setValue("delivery_info_for_every_file",
		    delivery_info_for_every_file);
	}

	settings.endGroup();
}

QString GlobPreferences::confDir(void) const
{
	return confDirPath(confSubdir);
}

QString GlobPreferences::loadConfPath(void) const
{
	return confDir() + QDir::separator() + loadFromConf;
}

QString GlobPreferences::saveConfPath(void) const
{
	return confDir() + QDir::separator() + saveToConf;
}

QString GlobPreferences::accountDbPath(void) const
{
	return confDir() + QDir::separator() + accountDbFile;
}

QString GlobPreferences::tagDbPath(void) const
{
	return confDir() + QDir::separator() + tagDbFile;
}

QString GlobPreferences::documentServiceDbPath(void) const
{
	return confDir() + QDir::separator() + documentServiceDbFile;
}

QString GlobPreferences::tagWebDatovkaDbPath(void) const
{
	return confDir() + QDir::separator() + tagWebDatovkaDbFile;
}
