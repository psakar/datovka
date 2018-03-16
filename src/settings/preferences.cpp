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
#include "src/settings/registry.h"

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
#define ENTR_AFTER_START_SELECT "after_start_select"
#define ENTR_TOOLBAR_BUTTON_STYLE "toolbar_button_style"
#define ENTR_USE_GLOBAL_PATHS "use_global_paths"
#define ENTR_SAVE_ATTACHMENTS_PATH "save_attachments_path"
#define ENTR_ADD_FILE_TO_ATTACHMENTS_PATH "add_file_to_attachments_path"
#define ENTR_ALL_ATTACHMENTS_SAVE_ZFO_MSG "all_attachments_save_zfo_msg"
#define ENTR_ALL_ATTACHMENTS_SAVE_ZFO_DELINFO "all_attachments_save_zfo_delinfo"
#define ENTR_ALL_ATTACHMENTS_SAVE_PDF_MSGENVEL "all_attachments_save_pdf_msgenvel"
#define ENTR_ALL_ATTACHMENTS_SAVE_PDF_DELINFO "all_attachments_save_pdf_delinfo"
#define ENTR_MESSAGE_FILENAME_FORMAT "message_filename_format"
#define ENTR_DELIVERY_FILENAME_FORMAT "delivery_filename_format"
#define ENTR_ATTACHMENT_FILENAME_FORMAT "attachment_filename_format"
#define ENTR_DELIVERY_FILENAME_FORMAT_ALL_ATTACH "delivery_filename_format_all_attach"
#define ENTR_DELIVERY_INFO_FOR_EVERY_FILE "delivery_info_for_every_file"
#define ENTR_LANGUAGE "language"
#define ENTR_DATE_FORMAT "date_format"

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
    m_checkNewVersions(false),
#else /* !DISABLE_VERSION_CHECK_BY_DEFAULT */
    m_checkNewVersions(true),
#endif /* DISABLE_VERSION_CHECK_BY_DEFAULT */
    m_sendStatsWithVersionChecks(false),
    isdsDownloadTimeoutMs(ISDS_DOWNLOAD_TIMEOUT_MS),
    messageMarkAsReadTimeout(TIMER_MARK_MSG_READ_MS),
    certificateValidationDate(DOWNLOAD_DATE),
    checkCrl(true),
    timestampExpirBeforeDays(TIMESTAMP_EXPIR_BEFORE_DAYS),
    afterStartSelect(SELECT_NOTHING),
    toolbarButtonStyle(TEXT_UNDER_ICON),
    useGlobalPaths(false),
    saveAttachmentsPath(QDir::homePath()),
    addFileToAttachmentsPath(QDir::homePath()),
    allAttachmentsSaveZfoMsg(false),
    allAttachmentsSaveZfoDelinfo(false),
    allAttachmentsSavePdfMsgenvel(false),
    allAttachmentsSavePdfDelinfo(false),
    messageFilenameFormat(DEFAULT_MESSAGE_FILENAME_FORMAT),
    deliveryFilenameFormat(DEFAULT_DELIVERY_FILENAME_FORMAT),
    attachmentFilenameFormat(DEFAULT_ATTACHMENT_FILENAME_FORMAT),
    deliveryFilenameFormatAllAttach(DEFAULT_DELIVERY_ATTACH_FORMAT),
    deliveryInfoForEveryFile(false),
    language(Localisation::langSystem), /* Use local settings. */
    dateFormat(DATE_FORMAT_DEFAULT)
{
}

int Preferences::qtToolButtonStyle(enum ToolbarButtonStyle style)
{
	switch (style) {
	case Preferences::ICON_ONLY:
		return Qt::ToolButtonIconOnly;
		break;
	case Preferences::TEXT_BESIDE_ICON:
		return Qt::ToolButtonTextBesideIcon;
		break;
	case Preferences::TEXT_UNDER_ICON:
		return Qt::ToolButtonTextUnderIcon;
		break;
	default:
		Q_ASSERT(0);
		return Qt::ToolButtonTextUnderIcon; /* Default. */
		break;
	}
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

	m_checkNewVersions = settings.value(
	    PREF_GROUP "/" ENTR_CHECK_NEW_VERSIONS,
	    dlftlGlobPref.m_checkNewVersions).toBool();

	m_sendStatsWithVersionChecks = settings.value(
	    PREF_GROUP "/" ENTR_SEND_STATS_WITH_VERSION_CHECKS,
	    dlftlGlobPref.m_sendStatsWithVersionChecks).toBool();

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

	value = settings.value(PREF_GROUP "/" ENTR_AFTER_START_SELECT,
	    dlftlGlobPref.afterStartSelect).toInt();
	switch (value) {
	case SELECT_NEWEST:
		afterStartSelect = SELECT_NEWEST;
		break;
	case SELECT_LAST_VISITED:
		afterStartSelect = SELECT_LAST_VISITED;
		break;
	case SELECT_NOTHING:
		afterStartSelect = SELECT_NOTHING;
		break;
	default:
		afterStartSelect = dlftlGlobPref.afterStartSelect;
		Q_ASSERT(0);
		break;
	}

	value = settings.value(PREF_GROUP "/" ENTR_TOOLBAR_BUTTON_STYLE,
	    dlftlGlobPref.toolbarButtonStyle).toInt();
	switch (value) {
	case ICON_ONLY:
		toolbarButtonStyle = ICON_ONLY;
		break;
	case TEXT_BESIDE_ICON:
		toolbarButtonStyle = TEXT_BESIDE_ICON;
		break;
	case TEXT_UNDER_ICON:
		toolbarButtonStyle = TEXT_UNDER_ICON;
		break;
	default:
		toolbarButtonStyle = dlftlGlobPref.toolbarButtonStyle;
		Q_ASSERT(0);
		break;
	}

	useGlobalPaths = settings.value(PREF_GROUP "/" ENTR_USE_GLOBAL_PATHS,
	    dlftlGlobPref.useGlobalPaths).toBool();

	saveAttachmentsPath = settings.value(
	    PREF_GROUP "/" ENTR_SAVE_ATTACHMENTS_PATH,
	    dlftlGlobPref.saveAttachmentsPath).toString();
	if (saveAttachmentsPath.isEmpty()) {
		saveAttachmentsPath = dlftlGlobPref.saveAttachmentsPath;
	}

	addFileToAttachmentsPath = settings.value(
	    PREF_GROUP "/" ENTR_ADD_FILE_TO_ATTACHMENTS_PATH,
	    dlftlGlobPref.addFileToAttachmentsPath).toString();
	if (addFileToAttachmentsPath.isEmpty()) {
		addFileToAttachmentsPath =
		    dlftlGlobPref.addFileToAttachmentsPath;
	}

	allAttachmentsSaveZfoMsg = settings.value(
	    PREF_GROUP "/" ENTR_ALL_ATTACHMENTS_SAVE_ZFO_MSG,
	    dlftlGlobPref.allAttachmentsSaveZfoMsg).toBool();

	allAttachmentsSaveZfoDelinfo = settings.value(
	    PREF_GROUP "/" ENTR_ALL_ATTACHMENTS_SAVE_ZFO_DELINFO,
	    dlftlGlobPref.allAttachmentsSaveZfoDelinfo).toBool();

	allAttachmentsSavePdfMsgenvel = settings.value(
	    PREF_GROUP "/" ENTR_ALL_ATTACHMENTS_SAVE_PDF_MSGENVEL,
	    dlftlGlobPref.allAttachmentsSavePdfMsgenvel).toBool();

	allAttachmentsSavePdfDelinfo = settings.value(
	    PREF_GROUP "/" ENTR_ALL_ATTACHMENTS_SAVE_PDF_DELINFO,
	    dlftlGlobPref.allAttachmentsSavePdfDelinfo).toBool();

	messageFilenameFormat = settings.value(
	    PREF_GROUP "/" ENTR_MESSAGE_FILENAME_FORMAT,
	    dlftlGlobPref.messageFilenameFormat).toString();

	deliveryFilenameFormat = settings.value(
	    PREF_GROUP "/" ENTR_DELIVERY_FILENAME_FORMAT,
	    dlftlGlobPref.deliveryFilenameFormat).toString();

	attachmentFilenameFormat = settings.value(
	    PREF_GROUP "/" ENTR_ATTACHMENT_FILENAME_FORMAT,
	    dlftlGlobPref.attachmentFilenameFormat).toString();

	deliveryFilenameFormatAllAttach = settings.value(
	    PREF_GROUP "/" ENTR_DELIVERY_FILENAME_FORMAT_ALL_ATTACH,
	    dlftlGlobPref.deliveryFilenameFormatAllAttach).toString();

	deliveryInfoForEveryFile = settings.value(
	    PREF_GROUP "/" ENTR_DELIVERY_INFO_FOR_EVERY_FILE,
	    dlftlGlobPref.deliveryInfoForEveryFile).toBool();

	language = settings.value(PREF_GROUP "/" ENTR_LANGUAGE,
	    dlftlGlobPref.language).toString();
	if (language.isEmpty()) {
		language = dlftlGlobPref.language;
	}

	value = settings.value(PREF_GROUP "/" ENTR_DATE_FORMAT,
	    dlftlGlobPref.dateFormat).toInt();
	switch (value) {
	case DATE_FORMAT_LOCALE:
		dateFormat = DATE_FORMAT_LOCALE;
		break;
	case DATE_FORMAT_ISO:
		dateFormat = DATE_FORMAT_ISO;
		break;
	case DATE_FORMAT_DEFAULT:
		dateFormat = DATE_FORMAT_DEFAULT;
		break;
	default:
		dateFormat = dlftlGlobPref.dateFormat;
		Q_ASSERT(0);
		break;
	}
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

	if (canConfigureCheckNewVersions() &&
	    (dlftlGlobPref.m_checkNewVersions != m_checkNewVersions)) {
		settings.setValue(ENTR_CHECK_NEW_VERSIONS, m_checkNewVersions);
	}

	if (dlftlGlobPref.m_sendStatsWithVersionChecks !=
	    m_sendStatsWithVersionChecks) {
		settings.setValue(ENTR_SEND_STATS_WITH_VERSION_CHECKS,
		    m_sendStatsWithVersionChecks);
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

	if (dlftlGlobPref.afterStartSelect != afterStartSelect) {
		settings.setValue(ENTR_AFTER_START_SELECT, afterStartSelect);
	}

	if (dlftlGlobPref.toolbarButtonStyle != toolbarButtonStyle) {
		settings.setValue(ENTR_TOOLBAR_BUTTON_STYLE,
		    toolbarButtonStyle);
	}

	if (dlftlGlobPref.useGlobalPaths != useGlobalPaths) {
		settings.setValue(ENTR_USE_GLOBAL_PATHS, useGlobalPaths);
	}

	if (dlftlGlobPref.saveAttachmentsPath != saveAttachmentsPath) {
		settings.setValue(ENTR_SAVE_ATTACHMENTS_PATH,
		    saveAttachmentsPath);
	}

	if (dlftlGlobPref.addFileToAttachmentsPath !=
	    addFileToAttachmentsPath) {
		settings.setValue(ENTR_ADD_FILE_TO_ATTACHMENTS_PATH,
		    addFileToAttachmentsPath);
	}

	if (dlftlGlobPref.allAttachmentsSaveZfoMsg !=
	    allAttachmentsSaveZfoMsg) {
		settings.setValue(ENTR_ALL_ATTACHMENTS_SAVE_ZFO_MSG,
		    allAttachmentsSaveZfoMsg);
	}

	if (dlftlGlobPref.allAttachmentsSaveZfoDelinfo !=
	    allAttachmentsSaveZfoDelinfo) {
		settings.setValue(ENTR_ALL_ATTACHMENTS_SAVE_ZFO_DELINFO,
		    allAttachmentsSaveZfoDelinfo);
	}

	if (dlftlGlobPref.allAttachmentsSavePdfMsgenvel !=
	    allAttachmentsSavePdfMsgenvel) {
		settings.setValue(ENTR_ALL_ATTACHMENTS_SAVE_PDF_MSGENVEL,
		    allAttachmentsSavePdfMsgenvel);
	}

	if (dlftlGlobPref.allAttachmentsSavePdfDelinfo !=
	    allAttachmentsSavePdfDelinfo) {
		settings.setValue(ENTR_ALL_ATTACHMENTS_SAVE_PDF_DELINFO,
		    allAttachmentsSavePdfDelinfo);
	}

	if (dlftlGlobPref.messageFilenameFormat != messageFilenameFormat) {
		settings.setValue(ENTR_MESSAGE_FILENAME_FORMAT,
		    messageFilenameFormat);
	}

	if (dlftlGlobPref.deliveryFilenameFormat != deliveryFilenameFormat) {
		settings.setValue(ENTR_DELIVERY_FILENAME_FORMAT,
		    deliveryFilenameFormat);
	}

	if (dlftlGlobPref.attachmentFilenameFormat !=
	    attachmentFilenameFormat) {
		settings.setValue(ENTR_ATTACHMENT_FILENAME_FORMAT,
		    attachmentFilenameFormat);
	}

	if (dlftlGlobPref.deliveryFilenameFormatAllAttach !=
	    deliveryFilenameFormatAllAttach) {
		settings.setValue(ENTR_DELIVERY_FILENAME_FORMAT_ALL_ATTACH,
		    deliveryFilenameFormatAllAttach);
	}

	if (dlftlGlobPref.deliveryInfoForEveryFile !=
	    deliveryInfoForEveryFile) {
		settings.setValue(ENTR_DELIVERY_INFO_FOR_EVERY_FILE,
		    deliveryInfoForEveryFile);
	}

	if (dlftlGlobPref.language != language) {
		settings.setValue(ENTR_LANGUAGE, language);
	}

	if (dlftlGlobPref.dateFormat != dateFormat) {
		settings.setValue(ENTR_DATE_FORMAT, dateFormat);
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

bool Preferences::canConfigureCheckNewVersions(void)
{
#if defined(DISABLE_VERSION_NOTIFICATION)
	return false;
#elif defined(Q_OS_WIN) /* !DISABLE_VERSION_NOTIFICATION */
	/* Registry settings can override the default behaviour. */
	return !(
	    RegPreferences::haveEntry(RegPreferences::LOC_POL,
	        RegPreferences::ENTR_DISABLE_VER_NOTIF) ||
	    RegPreferences::haveEntry(RegPreferences::LOC_SYS,
	        RegPreferences::ENTR_DISABLE_VER_NOTIF) ||
	    RegPreferences::haveEntry(RegPreferences::LOC_USR,
	        RegPreferences::ENTR_DISABLE_VER_NOTIF));
#else /* !Q_OS_WIN */
	return true;
#endif /* Q_OS_WIN */
}

bool Preferences::checkNewVersions(void) const
{
	if (canConfigureCheckNewVersions()) {
		return m_checkNewVersions;
	} else {
#if defined(DISABLE_VERSION_NOTIFICATION)
		return false;
#elif defined(Q_OS_WIN) /* !DISABLE_VERSION_NOTIFICATION */
		if (RegPreferences::haveEntry(RegPreferences::LOC_POL,
		        RegPreferences::ENTR_DISABLE_VER_NOTIF)) {
			return !RegPreferences::disableVersionNotification(
			    RegPreferences::LOC_POL);
		} else if (RegPreferences::haveEntry(RegPreferences::LOC_SYS,
		        RegPreferences::ENTR_DISABLE_VER_NOTIF)) {
			return !RegPreferences::disableVersionNotification(
			    RegPreferences::LOC_SYS);
		} else if (RegPreferences::haveEntry(RegPreferences::LOC_USR,
		        RegPreferences::ENTR_DISABLE_VER_NOTIF)) {
			return !RegPreferences::disableVersionNotification(
			    RegPreferences::LOC_USR);
		} else {
			Q_ASSERT(0);
			return dlftlGlobPref.m_checkNewVersions;
		}
#else /* !Q_OS_WIN */
		return dlftlGlobPref.m_checkNewVersions;
#endif /* Q_OS_WIN */
	}
}

void Preferences::setCheckNewVersions(bool val)
{
	m_checkNewVersions = val;
}

bool Preferences::sendStatsWithVersionChecks(void) const
{
	return m_sendStatsWithVersionChecks;
}

void Preferences::setSendStatsWithVersionChecks(bool val)
{
	m_sendStatsWithVersionChecks = val;
}
