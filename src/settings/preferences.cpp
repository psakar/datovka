/*
 * Copyright (C) 2014-2018 CZ.NIC
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
#include <QFile>

#include "src/common.h"
#include "src/datovka_shared/localisation/localisation.h"
#include "src/io/filesystem.h"
#include "src/settings/preferences.h"

/* Defaults. */
static const
Preferences dlftlGlobPref;

/*! Default configuration folder location. */
#define DFLT_CONF_SUBDIR ".dsgui"
/*! Default configuration file name. */
#define DFLT_CONF_FILE "dsgui.conf"
/*! Account database file name. */
#define ACCOUNT_DB_FILE "messages.shelf.db"
#define TAG_DB_FILE "tag.db"
#define RECORDS_MANAGEMENT_DB_FILE "records_management.db"

#define PREF_GROUP "preferences"
#define ENTR_AUTO_DOWNLOAD_WHOLE_MESSAGES "auto_download_whole_messages"
#define ENTR_DEFAULT_DOWNLOAD_SIGNED "default_download_signed"
#define ENTR_STORE_MESSAGES_ON_DISK "store_messages_on_disk"
#define ENTR_STORE_ADDITIONAL_DATA_ON_DISK "store_additional_data_on_disk"
#define ENTR_DOWNLOAD_ON_BACKGROUND "download_on_background"
#define ENTR_TIMER_VALUE "timer_value"
#define ENTR_DOWNLOAD_AT_START "download_at_start"
#define ENTR_CHECK_NEW_VERSIONS "check_new_versions"
#define ENTR_SEND_STATS_WITH_VERSION_CHECKS "send_stats_with_version_checks"
#define ENTR_ISDS_DOWNLOAD_TIMEOUT_MS "isds_download_timeout_ms"
#define ENTR_MESSAGE_MARK_AS_READ_TIMEOUT "message_mark_as_read_timeout"
#define ENTR_CERTIFICATE_VALIDATION_DATE "certificate_validation_date"
#define ENTR_CHECK_CRL "check_crl"
#define ENTR_TIMESTAMP_EXPIR_BEFORE_DAYS "timestamp_expir_before_days"

Preferences::Preferences(void)
    : confSubdir(DFLT_CONF_SUBDIR),
    loadFromConf(DFLT_CONF_FILE),
    saveToConf(DFLT_CONF_FILE),
    acntDbFile(ACCOUNT_DB_FILE),
    tagDbFile(TAG_DB_FILE),
    recMgmtDbFile(RECORDS_MANAGEMENT_DB_FILE),
    autoDownloadWholeMessages(false),
    defaultDownloadSigned(true),
    //storePasswordsOnDisk(false),
    storeMessagesOnDisk(true),
    storeAdditionalDataOnDisk(true),
    downloadOnBackground(false),
    timerValue(10),
    downloadAtStart(false),
#ifdef DISABLE_VERSION_CHECK_BY_DEFAULT
    checkNewVersions(false),
#else /* !DISABLE_VERSION_CHECK_BY_DEFAULT */
    checkNewVersions(true),
#endif /* DISABLE_VERSION_CHECK_BY_DEFAULT */
    sendStatsWithVersionChecks(false),
    isdsDownloadTimeoutMs(ISDS_DOWNLOAD_TIMEOUT_MS),
    messageMarkAsReadTimeout(TIMER_MARK_MSG_READ_MS),
    certificateValidationDate(DOWNLOAD_DATE),
    checkCrl(true),
    timestampExpirBeforeDays(TIMESTAMP_EXPIR_BEFORE_DAYS),
    toolbar_button_style(Qt::ToolButtonTextUnderIcon),
    date_format(DATE_FORMAT_DEFAULT),
    language(Localisation::langSystem), /* Use local settings. */
    after_start_select(SELECT_NOTHING),
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
    delivery_info_for_every_file(false)
{
}

bool Preferences::ensureConfPresence(void) const
{
	{
		QDir dir(confDir());
		if (!dir.exists()) {
			if (!dir.mkpath(".")) {
				return false;
			}
		}
	}
	{
		QFile file(loadConfPath());
		if (!file.exists()) {
			if (!file.open(QIODevice::ReadWrite)) {
				return false;
			}
			file.close();
		}
	}
	return true;
}

void Preferences::loadFromSettings(const QSettings &settings)
{
	int value;

	autoDownloadWholeMessages = settings.value(
	    PREF_GROUP "/" ENTR_AUTO_DOWNLOAD_WHOLE_MESSAGES,
	    dlftlGlobPref.autoDownloadWholeMessages).toBool();

	defaultDownloadSigned = settings.value(
	    PREF_GROUP "/" ENTR_DEFAULT_DOWNLOAD_SIGNED,
	    dlftlGlobPref.defaultDownloadSigned).toBool();

	storeMessagesOnDisk = settings.value(
	    PREF_GROUP "/" ENTR_STORE_MESSAGES_ON_DISK,
	    dlftlGlobPref.storeMessagesOnDisk).toBool();

	storeAdditionalDataOnDisk = settings.value(
	    PREF_GROUP "/" ENTR_STORE_ADDITIONAL_DATA_ON_DISK,
	    dlftlGlobPref.storeAdditionalDataOnDisk).toBool();

	downloadOnBackground = settings.value(
	    PREF_GROUP "/" ENTR_DOWNLOAD_ON_BACKGROUND,
	    dlftlGlobPref.downloadOnBackground).toBool();

	timerValue = settings.value(PREF_GROUP "/" ENTR_TIMER_VALUE,
	    dlftlGlobPref.timerValue).toInt();

	downloadAtStart = settings.value(PREF_GROUP "/" ENTR_DOWNLOAD_AT_START,
	    dlftlGlobPref.downloadAtStart).toBool();

	checkNewVersions = settings.value(
	    PREF_GROUP "/" ENTR_CHECK_NEW_VERSIONS,
	    dlftlGlobPref.checkNewVersions).toBool();

	sendStatsWithVersionChecks = settings.value(
	    PREF_GROUP "/" ENTR_SEND_STATS_WITH_VERSION_CHECKS,
	    dlftlGlobPref.sendStatsWithVersionChecks).toBool();

	isdsDownloadTimeoutMs = settings.value(
	    PREF_GROUP "/" ENTR_ISDS_DOWNLOAD_TIMEOUT_MS,
	    dlftlGlobPref.isdsDownloadTimeoutMs).toInt();

	messageMarkAsReadTimeout = settings.value(
	    PREF_GROUP "/" ENTR_MESSAGE_MARK_AS_READ_TIMEOUT,
	    dlftlGlobPref.messageMarkAsReadTimeout).toInt();

	value = settings.value(
	    PREF_GROUP "/" ENTR_CERTIFICATE_VALIDATION_DATE,
	    dlftlGlobPref.certificateValidationDate).toInt();
	switch (value) {
	case DOWNLOAD_DATE:
		certificateValidationDate = DOWNLOAD_DATE;
		break;
	case CURRENT_DATE:
		certificateValidationDate = CURRENT_DATE;
		break;
	default:
		certificateValidationDate =
		    dlftlGlobPref.certificateValidationDate;
		Q_ASSERT(0);
		break;
	}

	checkCrl = settings.value(PREF_GROUP "/" ENTR_CHECK_CRL,
	    dlftlGlobPref.checkCrl).toBool();

	timestampExpirBeforeDays = settings.value(
	    PREF_GROUP "/" ENTR_TIMESTAMP_EXPIR_BEFORE_DAYS,
	    dlftlGlobPref.timestampExpirBeforeDays).toInt();

	toolbar_button_style = settings.value(
	    "preferences/toolbar_button_style",
	    dlftlGlobPref.toolbar_button_style).toInt();

	use_global_paths = settings.value(
	    "preferences/use_global_paths",
	    dlftlGlobPref.use_global_paths).toBool();

	delivery_info_for_every_file = settings.value(
	    "preferences/delivery_info_for_every_file",
	    dlftlGlobPref.delivery_info_for_every_file).toBool();

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

void Preferences::saveToSettings(QSettings &settings) const
{
	settings.beginGroup(PREF_GROUP);

	/* Only values differing from defaults are written. */

	if (dlftlGlobPref.autoDownloadWholeMessages !=
	    autoDownloadWholeMessages) {
		settings.setValue(ENTR_AUTO_DOWNLOAD_WHOLE_MESSAGES,
		    autoDownloadWholeMessages);
	}

	if (dlftlGlobPref.defaultDownloadSigned != defaultDownloadSigned) {
		settings.setValue(ENTR_DEFAULT_DOWNLOAD_SIGNED,
		    defaultDownloadSigned);
	}

	if (dlftlGlobPref.storeMessagesOnDisk != storeMessagesOnDisk) {
		settings.setValue(ENTR_STORE_MESSAGES_ON_DISK,
		    storeMessagesOnDisk);
	}

	if (dlftlGlobPref.storeAdditionalDataOnDisk !=
	    storeAdditionalDataOnDisk) {
		settings.setValue(ENTR_STORE_ADDITIONAL_DATA_ON_DISK,
		    storeAdditionalDataOnDisk);
	}

	if (dlftlGlobPref.downloadOnBackground != downloadOnBackground) {
		settings.setValue(ENTR_DOWNLOAD_ON_BACKGROUND,
		    downloadOnBackground);
	}

	if (dlftlGlobPref.timerValue != timerValue) {
		settings.setValue(ENTR_TIMER_VALUE, timerValue);
	}

	if (dlftlGlobPref.downloadAtStart != downloadAtStart) {
		settings.setValue(ENTR_DOWNLOAD_AT_START, downloadAtStart);
	}

	if (dlftlGlobPref.checkNewVersions != checkNewVersions) {
		settings.setValue(ENTR_CHECK_NEW_VERSIONS, checkNewVersions);
	}

	if (dlftlGlobPref.sendStatsWithVersionChecks !=
	    sendStatsWithVersionChecks) {
		settings.setValue(ENTR_SEND_STATS_WITH_VERSION_CHECKS,
		    sendStatsWithVersionChecks);
	}

	if (dlftlGlobPref.isdsDownloadTimeoutMs != isdsDownloadTimeoutMs) {
		settings.setValue(ENTR_ISDS_DOWNLOAD_TIMEOUT_MS,
		    isdsDownloadTimeoutMs);
	}

	if (dlftlGlobPref.messageMarkAsReadTimeout !=
	    messageMarkAsReadTimeout) {
		settings.setValue(ENTR_MESSAGE_MARK_AS_READ_TIMEOUT,
		    messageMarkAsReadTimeout);
	}

	if (dlftlGlobPref.certificateValidationDate !=
	    certificateValidationDate) {
		settings.setValue(ENTR_CERTIFICATE_VALIDATION_DATE,
		    certificateValidationDate);
	}

	if (dlftlGlobPref.checkCrl != checkCrl) {
		settings.setValue(ENTR_CHECK_CRL, checkCrl);
	}

	if (dlftlGlobPref.timestampExpirBeforeDays !=
	    timestampExpirBeforeDays) {
		settings.setValue(ENTR_TIMESTAMP_EXPIR_BEFORE_DAYS,
		    timestampExpirBeforeDays);
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

QString Preferences::confDir(void) const
{
	return confDirPath(confSubdir);
}

QString Preferences::loadConfPath(void) const
{
	return confDir() + QDir::separator() + loadFromConf;
}

QString Preferences::saveConfPath(void) const
{
	return confDir() + QDir::separator() + saveToConf;
}

QString Preferences::acntDbPath(void) const
{
	return confDir() + QDir::separator() + acntDbFile;
}

QString Preferences::tagDbPath(void) const
{
	return confDir() + QDir::separator() + tagDbFile;
}

QString Preferences::recMgmtDbPath(void) const
{
	return confDir() + QDir::separator() + recMgmtDbFile;
}
