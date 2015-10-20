/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QNetworkProxyQuery>
#include <QTemporaryFile>
#include <QUrl>

#include "common.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"

#define WIN_PREFIX "AppData/Roaming"
/*!< Default configuration folder location. */
#define DFLT_CONF_SUBDIR ".dsgui"
/*!< Default configuration file name. */
#define DFLT_CONF_FILE "dsgui.conf"
#define ACCOUNT_DB_FILE "messages.shelf.db"


GlobPreferences globPref;
ProxiesSettings globProxSet;

/* Defaults. */
static const
GlobPreferences dlftlGlobPref; /* Defaults. */
static const
ProxiesSettings dfltGlobProxSet;


/*!
 * @brief Searches for the location of the supplied text file.
 */
static
QString suppliedTextFileLocation(const QString &fName);


/* ========================================================================= */
GlobPreferences::GlobPreferences(void)
/* ========================================================================= */
    : confSubdir(DFLT_CONF_SUBDIR),
    loadFromConf(DFLT_CONF_FILE),
    saveToConf(DFLT_CONF_FILE),
    accountDbFile(ACCOUNT_DB_FILE),
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


/* ========================================================================= */
GlobPreferences::~GlobPreferences(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void GlobPreferences::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void GlobPreferences::saveToSettings(QSettings &settings) const
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Return path to configuration directory.
 */
QString GlobPreferences::confDir(void) const
/* ========================================================================= */
{
#ifdef PORTABLE_APPLICATION
	QString dirPath;

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#  ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#  endif /* Q_OS_OSX */

	dirPath += QDir::separator() + confSubdir;
	return dirPath;

#else /* !PORTABLE_APPLICATION */
	QDir homeDir(QDir::homePath());

	if (homeDir.exists(WIN_PREFIX) && !homeDir.exists(confSubdir)) {
		/* Set windows directory. */
		homeDir.cd(WIN_PREFIX);
	}

	return homeDir.path() + QDir::separator() + confSubdir;
#endif /* PORTABLE_APPLICATION */
}


const QString ProxiesSettings::noProxyStr("None");
const QString ProxiesSettings::autoProxyStr("-1");


/* ========================================================================= */
ProxiesSettings::ProxySettings::ProxySettings(void)
/* ========================================================================= */
    : hostName("-1"),
    port(-1),
    userName(),
    password()
{
}


/* ========================================================================= */
ProxiesSettings::ProxiesSettings(void)
/* ========================================================================= */
    : https(),
    http()
{
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void ProxiesSettings::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	QString auxStr;
	bool ok;

	auxStr = settings.value("connection/https_proxy").toString();
	if (auxStr.isEmpty() || (noProxyStr == auxStr)) {
		https.hostName = noProxyStr;
		https.port = -1;
	} else if (autoProxyStr == auxStr) {
		https.hostName = autoProxyStr;
		https.port = -1;
	} else {
		if (auxStr.contains(":")) {
			https.hostName = auxStr.section(":", 0, -2);
			https.port =
			    auxStr.section(":", -1, -1).toInt(&ok, 10);
			if (!ok) {
				https.hostName = noProxyStr;
				https.port = -1;
			}
		} else {
			https.hostName = noProxyStr;
			https.port = -1;
		}
	}
	https.userName =
	    settings.value("connection/https_proxy_username").toString();
	https.password = fromBase64(
	    settings.value("connection/https_proxy_password").toString());

	auxStr = settings.value("connection/http_proxy").toString();
	if (auxStr.isEmpty() || (noProxyStr == auxStr)) {
		http.hostName = noProxyStr;
		http.port = -1;
	} else if (autoProxyStr == auxStr) {
		http.hostName = autoProxyStr;
		http.port = -1;
	} else {
		if (auxStr.contains(":")) {
			http.hostName = auxStr.section(":", 0, -2);
			http.port =
			    auxStr.section(":", -1, -1).toInt(&ok, 10);
			if (!ok) {
				http.hostName = noProxyStr;
				http.port = -1;
			}
		} else {
			http.hostName = noProxyStr;
			http.port = -1;
		}
	}
	http.userName =
	    settings.value("connection/http_proxy_username").toString();
	http.password = fromBase64(
	    settings.value("connection/http_proxy_password").toString());
}


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void ProxiesSettings::saveToSettings(QSettings &settings) const
/* ========================================================================= */
{
	QString auxStr;

	settings.beginGroup("connection");

	auxStr = https.hostName;
	if (https.port >= 0) {
		auxStr += ":" + QString::number(https.port, 10);
	}
	settings.setValue("https_proxy", auxStr);
	if (!https.userName.isEmpty()) {
		settings.setValue("https_proxy_username",
		    https.userName);
	}
	if (!https.password.isEmpty()) {
		settings.setValue("https_proxy_password",
		    toBase64(https.password));
	}

	auxStr = http.hostName;
	if (http.port >= 0) {
		auxStr += ":" + QString::number(http.port, 10);
	}
	settings.setValue("http_proxy", auxStr);
	if (!http.userName.isEmpty()) {
		settings.setValue("http_proxy_username", http.userName);
	}
	if (!http.password.isEmpty()) {
		settings.setValue("http_proxy_password",
		    toBase64(http.password));
	}

	settings.endGroup();
}


/* ========================================================================= */
ProxiesSettings::ProxySettings ProxiesSettings::detectHttpProxy(void)
/* ========================================================================= */
{
	ProxySettings settings;

	settings.hostName = noProxyStr;
	settings.port = -1;

	/* TODO -- Try to contact the proxy to check whether it works? */

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	QByteArray envVar = qgetenv("http_proxy");
	QUrl proxyUrl(QString::fromLatin1(envVar));

	if (proxyUrl.isValid()) {
		qDebug() << "Detected URL" << proxyUrl <<
		    proxyUrl.userName() << proxyUrl.password();
		settings.hostName = proxyUrl.host();
		settings.port = proxyUrl.port();
		settings.userName = proxyUrl.userName();
		settings.password = proxyUrl.password();
		return settings;
	}
#else
	QNetworkProxyQuery npq(QUrl("http://www.nic.cz"));

	QList<QNetworkProxy> listOfProxies =
	   QNetworkProxyFactory::systemProxyForQuery(npq);

	if (1 < listOfProxies.size()) {
		logWarning("%s/n", "Multiple proxies detected. Using first.");
	}

	foreach (QNetworkProxy p, listOfProxies) {
		if (!p.hostName().isEmpty()) {
			settings.hostName = p.hostName();
			settings.port = p.port();
			settings.userName = p.user();
			settings.password = p.password();
			return settings;
		}
	}
#endif /* Q_OS_UNIX && !Q_OS_MAC */

	return settings;
}


const QString dateTimeDisplayFormat("dd.MM.yyyy HH:mm:ss");
const QString dateDisplayFormat("dd.MM.yyyy");

QLocale programLocale;


/* ========================================================================= */
/*
 * Translates message type to text.
 */
const QString dmTypeToText(const QString &dmType)
/* ========================================================================= */
{
	if (dmType.size() != 1) {
		return QString();
	}

	switch (dmType[0].toLatin1()) {
	case 'K':
		return QObject::tr("Postal data message");
		break;
	case 'I':
		return QObject::tr("Initializing postal data message");
		break;
	case 'O':
		return QObject::tr("Reply postal data message");
		break;
	case 'X':
		return QObject::tr(
		    "Initializing postal data message - expired");
		break;
	case 'Y':
		return QObject::tr("Initializing postal data message - used");
		break;
	default:
		return QString();
		break;
	}
}


/* ========================================================================= */
/*
 * Translates author type to text.
 */
const QString authorTypeToText(const QString &authorType)
/* ========================================================================= */
{
	if ("PRIMARY_USER" == authorType) {
		/* Opravnena osoba nebo likvidator. */
		return QObject::tr("Primary user");
	} else if ("ENTRUSTED_USER" == authorType) {
		/* Poverena osoba. */
		return QObject::tr("Entrusted user");
	} else if ("ADMINISTRATOR" == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Administrator");
	} else if ( "OFFICIAL" == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Official");
	} else if ("VIRTUAL" == authorType) {
		/* Spisovka. */
		return QObject::tr("Virtual");
	} else {
		return QString();
	}
}


/* ========================================================================= */
/*
 * Returns message status description.
 */
const QString msgStatusToText(int status)
/* ========================================================================= */
{
	switch (status) {
	case 1:
		/* Zprava byla podana (vznikla v ISDS). */
		return QObject::tr("Message bas submitted "
		    "(originates at ISDS )");
		break;
	case 2:
		/*
		 * Datová zprava vcetne pisemnosti podepsana casovym razitkem.
		 */
		return QObject::tr("Data message and papers signed with "
		    "time-stamp.");
		break;
	case 3:
		/*
		 * Zprava neprosla AV kontrolou; nakazena pisemnost je smazana;
		 * konecny stav zpravy pred smazanim.
		 */
		return QObject::tr("Message did not pass through AV check; "
		    "infected paper deleted; final status before deletion.");
		break;
	case 4:
		/* Zprava dodana do ISDS (zapsan cas dodani). */
		return QObject::tr("Message handed into ISDS "
		    "(delivery time recorded).");
		break;
	case 5:
		/*
		 * Uplynulo 10 dnu od dodani verejne zpravy, ktera dosud nebyla
		 * dorucena prihlasenim (predpoklad dorucení fikci u neOVM DS);
		 * u komercni zpravy nemuze tento stav nastat.
		 */
		return QObject::tr("10 days have passed since the delivery of "
		    "the public message which has not been accepted by "
		    "logging-in (assumption of delivery by fiction in nonOVM "
		    "DS); this state cannot occur for commertial messages.");
		break;
	case 6:
		/*
		 * Osoba opravnena cist tuto zpravu se prihlasila - dodana
		 * zprava byla dorucena.",
		 */
		return QObject::tr("A person authorised to read this message "
		    "has logged-in -- delivered message has been accepted.");
		break;
	case 7:
		/* Zprava byla prectena (na portale nebo akci ESS). */
		return QObject::tr("Message has been read (on the portal or "
		    "by ESS action).");
		break;
	case 8:
		/*
		 * Zprava byla oznacena jako nedorucitelna, protoze DS adresata
		 * byla zpetne znepristupnena.
		 */
		return QObject::tr("Message marked as undeliverable because "
		    "the target DS has been made inaccessible.");
		break;
	case 9:
		/*
		 * Obsah zpravy byl smazan, obalka zpravy vcetne hashu
		 * presunuta do archivu.
		 */
		return QObject::tr("Message content deleted, envelope "
		    "including hashes has been moved into archive.");
		break;
	case 10:
		/* Zprava je v Datovem trezoru. */
		return QObject::tr("Message resides in data vault.");
		break;
	default:
		return QString();
		break;
	}
}


/* ========================================================================= */
/*
 * Return privilegs as html string from number representation.
 */
QString convertUserPrivilsToString(int userPrivils)
/* ========================================================================= */
{
	QString privStr;
	const QString sepPref("<li>- "), sepSuff("</li>");

	if (userPrivils == 255) {
		return QObject::tr("Full control");
	} else {
		privStr = QObject::tr("Restricted control");
	}

	if (userPrivils & PRIVIL_READ_NON_PERSONAL) {
		// "stahovat a číst došlé DZ"
		privStr += sepPref +
		    QObject::tr("download and read incoming DM") + sepSuff;
	}
	if (userPrivils & PRIVIL_READ_ALL) {
		// "stahovat a číst DZ určené do vlastních rukou"
		privStr += sepPref +
		    QObject::tr("download and read DM sent into own hands") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_CREATE_DM) {
		// "vytvářet a odesílat DZ, stahovat odeslané DZ"
		privStr += sepPref +
		    QObject::tr("create and send DM, download sent DM") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_VIEW_INFO) {
		// "načítat seznamy DZ, Dodejky a Doručenky"
		privStr += sepPref +
		    QObject::tr(
		        "retrieve DM lists, delivery and acceptance reports") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_SEARCH_DB) {
		// "vyhledávat DS"
		privStr += sepPref +
		    QObject::tr("search for data boxes") + sepSuff;
	}
	if (userPrivils & PRIVIL_OWNER_ADM) {
		// "spravovat DS"
		privStr += sepPref + QObject::tr("manage the data box") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_READ_VAULT) {
		// "číst zprávy v DT"
		privStr += sepPref +
		    QObject::tr("read message in data vault") + sepSuff;
	}
	if (userPrivils & PRIVIL_ERASE_VAULT) {
		// "mazat zprávy v DT"
		privStr += sepPref +
		    QObject::tr("erase messages from data vault") + sepSuff;
	}
	return privStr;
}


/* ========================================================================= */
/*
 * Changes all occurrences of '\' to '/' in given file.
 */
void fixBackSlashesInFile(const QString &fileName)
/* ========================================================================= */
{
	QString fileContent;
	QFile file(fileName);

	if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
		QTextStream in(&file);
		fileContent = in.readAll();
		file.close();
	} else {
		qDebug() << "Error: Cannot open file '" << fileName << "'";
		return;
	}

	fileContent.replace(QString("\\"), QString("/"));

	if (file.open(QIODevice::WriteOnly|QIODevice::Text)) {
		QTextStream in(&file);
		file.reset();
		in << fileContent;
		file.close();
	} else {
		qDebug() << "Error: Cannot write file '" << fileName << "'";
	}
}


/* ========================================================================= */
/*
 * Fix account password format = compatability with old datovka.
 */
void removeDoubleQuotesFromAccountPassword(const QString &fileName)
/* ========================================================================= */
{
	QString fileContent;
	QFile file(fileName);

	if (file.open(QIODevice::ReadOnly|QIODevice::Text)) {
		QTextStream in(&file);
		fileContent = in.readAll();
		file.close();
	} else {
		qDebug() << "Error: Cannot open file '" << fileName << "'";
		return;
	}

	fileContent.replace(QString("\""), QString(""));

	if (file.open(QIODevice::WriteOnly|QIODevice::Text)) {
		QTextStream in(&file);
		file.reset();
		in << fileContent;
		file.close();
	} else {
		qDebug() << "Error: Cannot write file '" << fileName << "'";
	}
}


/* ========================================================================= */
/*
 * Convert hex to dec number
 */
int convertHexToDecIndex(int value)
/* ========================================================================= */
{
	if (value == MESSAGESTATE_SENT) return 1;
	else if (value == MESSAGESTATE_STAMPED) return 2;
	else if (value == MESSAGESTATE_INFECTED) return 3;
	else if (value == MESSAGESTATE_DELIVERED) return 4;
	else if (value == MESSAGESTATE_SUBSTITUTED) return 5;
	else if (value == MESSAGESTATE_RECEIVED) return 6;
	else if (value == MESSAGESTATE_READ) return 7;
	else if (value == MESSAGESTATE_UNDELIVERABLE) return 8;
	else if (value == MESSAGESTATE_REMOVED) return 9;
	else if (value == MESSAGESTATE_IN_SAFE) return 10;
	else return 0;
}


/* ========================================================================= */
/*
 * Convert hash algorithm to string
 */
QString convertHashAlg(int value)
/* ========================================================================= */
{
	if (value == HASH_ALGORITHM_MD5) return "MD5";
	else if (value == HASH_ALGORITHM_SHA_1) return "SHA-1";
	else if (value == HASH_ALGORITHM_SHA_224) return "SHA-224";
	else if (value == HASH_ALGORITHM_SHA_256) return "SHA-256";
	else if (value == HASH_ALGORITHM_SHA_384) return "SHA-384";
	else if (value == HASH_ALGORITHM_SHA_512) return "SHA-512";
	else return "";
}


/* ========================================================================= */
/*
 * Convert hash algorithm to int
 */
int convertHashAlg2(QString value)
/* ========================================================================= */
{
	if (value == "MD5") return HASH_ALGORITHM_MD5;
	else if (value == "SHA-1") return HASH_ALGORITHM_SHA_1;
	else if (value == "SHA-224") return HASH_ALGORITHM_SHA_224;
	else if (value == "SHA-256") return HASH_ALGORITHM_SHA_256;
	else if (value == "SHA-384") return HASH_ALGORITHM_SHA_384;
	else if (value == "SHA-512") return HASH_ALGORITHM_SHA_512;
	else return 0;
}



/* ========================================================================= */
/*
 * Convert type of attachment files
 */
QString convertAttachmentType(int value)
/* ========================================================================= */
{
	if (value == FILEMETATYPE_MAIN) return "main";
	else if (value == FILEMETATYPE_ENCLOSURE) return "encl";
	else if (value == FILEMETATYPE_SIGNATURE) return "sign";
	else if (value == FILEMETATYPE_META) return "meta";
	else return "";
}

/* ========================================================================= */
/*
 * Convert type of author to string
 */
QString convertSenderTypeToString(int value)
/* ========================================================================= */
{
	if (value == SENDERTYPE_PRIMARY) return "PRIMARY_USER";
	else if (value == SENDERTYPE_ENTRUSTED) return "ENTRUSTED_USER";
	else if (value == SENDERTYPE_ADMINISTRATOR) return "ADMINISTRATOR";
	else if (value == SENDERTYPE_OFFICIAL) return "OFFICIAL";
	else if (value == SENDERTYPE_VIRTUAL) return "VIRTUAL";
	else if (value == SENDERTYPE_OFFICIAL_CERT) return "OFFICIAL_CERT";
	else if (value == SENDERTYPE_LIQUIDATOR) return "LIQUIDATOR";
	else return "";
}


/* ========================================================================= */
/*
 * Convert type of databox to string
 */
QString convertDbTypeToString(int value)
/* ========================================================================= */
{
	if (value == DBTYPE_SYSTEM) return "SYSTEM";
	else if (value == DBTYPE_OVM) return "OVM";
	else if (value == DBTYPE_OVM_NOTAR) return "OVM_NOTAR";
	else if (value == DBTYPE_OVM_EXEKUT) return "OVM_EXEKUT";
	else if (value == DBTYPE_OVM_REQ) return "OVM_REQ";
	else if (value == DBTYPE_PO) return "PO";
	else if (value == DBTYPE_PO_ZAK) return "PO_ZAK";
	else if (value == DBTYPE_PO_REQ) return "PO_REQ";
	else if (value == DBTYPE_PFO) return "PFO";
	else if (value == DBTYPE_PFO_ADVOK) return "PFO_ADVOK";
	else if (value == DBTYPE_PFO_DANPOR) return "PFO_DANPOR";
	else if (value == DBTYPE_PFO_INSSPR) return "PFO_INSSPR";
	else if (value == DBTYPE_FO) return "FO";
	else return "";
}


/* ========================================================================= */
/*
 * Convert type of user to string
 */
const QString & convertUserTypeToString(int value)
/* ========================================================================= */
{
	static const QString pu("PRIMARY_USER"), eu("ENTRUSTED_USER"),
	    a("ADMINISTRATOR"), l("LIQUIDATOR"), ou("OFFICIAL_USER"),
	    ocu("OFFICIAL_CERT_USER");
	static const QString empty;

	switch (value) {
	case USERTYPE_PRIMARY: return pu;
	case USERTYPE_ENTRUSTED: return eu;
	case USERTYPE_ADMINISTRATOR: return a;
	case USERTYPE_LIQUIDATOR: return l;
	case USERTYPE_OFFICIAL: return ou;
	case USERTYPE_OFFICIAL_CERT: return ocu;
	default: return empty;
	}
}


/* ========================================================================= */
/*
 * Convert type of databox to int
 */
int convertDbTypeToInt(QString value)
/* ========================================================================= */
{
	if (value == "OVM") return DBTYPE_OVM;
	else if (value == "OVM_NOTAR") return DBTYPE_OVM_NOTAR;
	else if (value == "OVM_EXEKUT") return DBTYPE_OVM_EXEKUT;
	else if (value == "OVM_REQ") return DBTYPE_OVM_REQ;
	else if (value == "PO") return DBTYPE_PO;
	else if (value == "PO_ZAK") return DBTYPE_PO_ZAK;
	else if (value == "PO_REQ") return DBTYPE_PO_REQ;
	else if (value == "PFO") return DBTYPE_PFO;
	else if (value == "PFO_ADVOK") return DBTYPE_PFO_ADVOK;
	else if (value == "PFO_DANPOR") return DBTYPE_PFO_DANPOR;
	else if (value == "PFO_INSSPR") return DBTYPE_PFO_INSSPR;
	else if (value == "FO") return DBTYPE_FO;
	else return DBTYPE_SYSTEM;
}

/* ========================================================================= */
/*
 * Convert event type to string
 */
QString convertEventTypeToString(int value)
/* ========================================================================= */
{
	if (value == EVENT_ACCEPTED_BY_RECIPIENT) return "EV4: ";
	else if (value == EVENT_ACCEPTED_BY_FICTION) return "EV2: ";
	else if (value == EVENT_UNDELIVERABLE) return "EV3: ";
	else if (value == EVENT_COMMERCIAL_ACCEPTED) return "EV1: ";
	else if (value == EVENT_ENTERED_SYSTEM) return "EV0: ";
	else if (value == EVENT_DELIVERED) return "EV5: ";
	else if (value == EVENT_PRIMARY_LOGIN) return "EV11: ";
	else if (value == EVENT_ENTRUSTED_LOGIN) return "EV12: ";
	else if (value == EVENT_SYSCERT_LOGIN) return "EV13: ";
	else if (value == EVENT_UKNOWN) return "";
	else return "";
}


/* ========================================================================= */
/*
 * Convert event type to string
 */
QString getdbStateText(int value)
/* ========================================================================= */
{
	if (value == DBSTATE_ACCESSIBLE)
		return QObject::tr("DS je přístupná, lze do ní dodávat zprávy, "
		    "na Portále lze vyhledat");
	else if (value == DBSTATE_TEMP_UNACCESSIBLE)
		return QObject::tr("DS je dočasně znepřístupněna (na vlastní "
		"žádost), může být později opět zpřístupněna");
	else if (value == DBSTATE_NOT_YET_ACCESSIBLE)
		return QObject::tr("DS je dosud neaktivní, dosud se do ní nikdo"
		" nepřihlásil z Portálu a nelze ji zpřístupnit pouze na "
		"základě doručení přístupových údajů");
	else if (value == DBSTATE_PERM_UNACCESSIBLE)
		return QObject::tr("DS je trvale znepřístupněna, čeká na smazání"
		" (může být opět zpřístupněna)");
	else if (value == DBSTATE_REMOVED)
		return QObject::tr("DS je smazána (přesto existuje v ISDS)");
	else return QObject::tr("Došlo k chybě při zjišťování stavu");
}


/* ========================================================================= */
/*
 * Converts base64 encoded string into plain text.
 */
QString fromBase64(const QString &base64)
/* ========================================================================= */
{
	return QString::fromUtf8(QByteArray::fromBase64(base64.toUtf8()));
}


/* ========================================================================= */
/*!
 * @brief Converts string into base64.
 */
QString toBase64(const QString &plain)
/* ========================================================================= */
{
	return QString::fromUtf8(plain.toUtf8().toBase64());
}

/* ========================================================================= */
int base64RealSize(const QByteArray &b64)
/* ========================================================================= */
{
	int b64size = b64.size();
	int cnt = 0;
	if (b64size >= 3) {
		for (int i = 1; i <= 3; ++i) {
			if ('=' == b64[b64size - i]) {
				++cnt;
			}
		}
	}
	return b64size * 3 / 4 - cnt;
}


#define CREDITS_FILE "AUTHORS"
#define LICENCE_FILE "COPYING"


/* ========================================================================= */
/*
 * Returns the content of the supplied text file.
 */
QString suppliedTextFileContent(enum text_file textFile)
/* ========================================================================= */
{
	const char *fileName = NULL;
	QString content;

	switch (textFile) {
	case TEXT_FILE_CREDITS:
		fileName = CREDITS_FILE;
		break;
	case TEXT_FILE_LICENCE:
		fileName = LICENCE_FILE;
		break;
	default:
		fileName = NULL;
	}

	if (NULL == fileName) {
		Q_ASSERT(0);
		return QString();
	}

	QString fileLocation = suppliedTextFileLocation(fileName);
	if (fileLocation.isEmpty()) {
		return QString();
	}

	QFile file(fileLocation);
	QTextStream textStream(&file);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		content = textStream.readAll();
	}

	file.close();

	return content;
}


#define LOCALE_SRC_PATH "locale"
/* ========================================================================= */
/*
 * Returns the path to directory where supplied localisation resides.
 */
QString appLocalisationDir(void)
/* ========================================================================= */
{
	QString dirPath;

#ifdef LOCALE_INST_DIR
	/*
	 * Search in installation location if supplied.
	 */

	dirPath = QString(LOCALE_INST_DIR);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}
#endif /* LOCALE_INST_DIR */

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	dirPath += QDir::separator() + QString(LOCALE_SRC_PATH);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}

	return dirPath;
}


/* ========================================================================= */
/*!
 * @brief Returns the path to directory where supplied localisation resides.
 */
QString qtLocalisationDir(void)
/* ========================================================================= */
{
	QString dirPath;

#ifdef LOCALE_INST_DIR
	/*
	 * This variable is set in UNIX-like systems. Use default Qt location
	 * directory.
	 */

	dirPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}
#endif /* LOCALE_INST_DIR */

	/* Search in application location. */
	dirPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(dirPath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		dirPath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	dirPath += QDir::separator() + QString(LOCALE_SRC_PATH);

	if (QFileInfo::exists(dirPath)) {
		return dirPath;
	} else {
		dirPath.clear();
	}

	return dirPath;
}
#undef LOCALE_SRC_PATH


/* ========================================================================= */
/*
 * Searches for the location of the supplied text file.
 */
static
QString suppliedTextFileLocation(const QString &fName)
/* ========================================================================= */
{
	QString filePath;

#ifdef TEXT_FILES_INST_DIR
	/*
	 * Search in installation location if supplied.
	 */

	filePath = QString(TEXT_FILES_INST_DIR) + QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}
#endif /* TEXT_FILES_INST_DIR */

	/* Search in application location. */
	filePath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_OSX
	{
		QDir directory(filePath);
		if ("MacOS" == directory.dirName()) {
			directory.cdUp();
		}
		filePath = directory.absolutePath() + QDir::separator() +
		    "Resources";
	}
#endif /* Q_OS_OSX */
	filePath += QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}

	return filePath;
}


/* ========================================================================= */
/*
 * Create and write data to file.
 */
enum WriteFileState writeFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError)
/* ========================================================================= */
{
	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return WF_ERROR;
	}

	QFile fout(fileName);
	if (!fout.open(QIODevice::WriteOnly)) {
		return WF_CANNOT_CREATE;
	}

	int written = fout.write(data);
	bool flushed = fout.flush();
	fout.close();

	if ((written != data.size()) || !flushed) {
		if (deleteOnError) {
			QFile::remove(fileName);
		}
		return WF_CANNOT_WRITE_WHOLE;
	}

	return WF_SUCCESS;
}


/* ========================================================================= */
/*
 * Create and write data to temporary file.
 */
QString writeTemporaryFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError)
/* ========================================================================= */
{
	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return QString();
	}

	QString nameCopy(fileName);
	nameCopy.replace(QRegExp("[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]"),
	    QString( "_" ));

	QTemporaryFile fout(QDir::tempPath() + QDir::separator() + nameCopy);
	if (!fout.open()) {
		return QString();
	}
	fout.setAutoRemove(false);

	/* Get whole path. */
	QString fullName = fout.fileName();

	int written = fout.write(data);
	bool flushed = fout.flush();
	fout.close();

	if ((written != data.size()) || !flushed) {
		if (deleteOnError) {
			QFile::remove(fullName);
		}
		return QString();
	}

	return fullName;
}


/* ========================================================================= */
/*
 * Create filename based on format string.
 */
QString createFilenameFromFormatString(QString pattern, const QString &dmID,
    const QString &dbID, const QString &userName, const QString &attachFilename,
    const QDateTime &dmDeliveryTime, QDateTime dmAcceptanceTime,
    QString dmAnnotation, QString dmSender)
/* ========================================================================= */
{
	debugFuncCall();

	// for future use
	(void) dmDeliveryTime;

	// known atrributes
	// {"%Y","%M","%D","%h","%m","%i","%s","%S","%d","%u","%f"};

	if (pattern.isEmpty()) {
		pattern = DEFAULT_TMP_FORMAT;
	}

	if (!dmAcceptanceTime.isValid()) {
		dmAcceptanceTime = QDateTime::currentDateTime();
	}

	QPair<QString, QString> pair;
	QList<QPair<QString, QString>> knowAtrrList;

	pair.first = "%Y";
	pair.second = dmAcceptanceTime.date().toString("yyyy");
	knowAtrrList.append(pair);
	pair.first = "%M";
	pair.second = dmAcceptanceTime.date().toString("MM");
	knowAtrrList.append(pair);
	pair.first = "%D";
	pair.second = dmAcceptanceTime.date().toString("dd");
	knowAtrrList.append(pair);
	pair.first = "%h";
	pair.second = dmAcceptanceTime.time().toString("hh");
	knowAtrrList.append(pair);
	pair.first = "%m";
	pair.second = dmAcceptanceTime.time().toString("mm");
	knowAtrrList.append(pair);
	pair.first = "%i";
	pair.second = dmID;
	knowAtrrList.append(pair);
	pair.first = "%s";
	pair.second = dmAnnotation.replace(" ","-");
	knowAtrrList.append(pair);
	pair.first = "%S";
	pair.second = dmSender.replace(" ","-");
	knowAtrrList.append(pair);
	pair.first = "%d";
	pair.second = dbID;
	knowAtrrList.append(pair);
	pair.first = "%u";
	pair.second = userName;
	knowAtrrList.append(pair);
	pair.first = "%f";
	pair.second = attachFilename;
	knowAtrrList.append(pair);

	for (int i = 0; i < knowAtrrList.length(); ++i) {
		pattern.replace(knowAtrrList[i].first, knowAtrrList[i].second);
	}

	/*
	* Eliminate illegal characters "\/:*?"<>|" in the filename.
	* All these characters are replaced by "_".
	*/
	pattern.replace(QRegExp("[" + QRegExp::escape( "\\/:*?\"<>|" ) + "]"),
	    QString( "_" ));

	return pattern;
}


/* ========================================================================= */
/*
 * Check if file exists in the path and return non-conflicting filename.
 */
QString getNonConflictingFileName(QString fileName)
/* ========================================================================= */
{
	debugFuncCall();

	if (QFile::exists(fileName)) {

		int fileCnt = 0;
		QFileInfo fi(fileName);
		// remember origin filename and path
		const QString base(fi.baseName());
		const QString path(fi.path());
		const QString suffix(fi.completeSuffix());

		do {
			fileCnt++;
			// create a new filename
			QString newName(base + "_" + QString::number(fileCnt));
			// join path and new filename
			fileName = path + QString(QDir::separator())
			    + newName + "." + suffix;
		} while (QFile::exists(fileName));
	}

	return fileName;
}
