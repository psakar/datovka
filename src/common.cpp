

#include <QDir>
#include <QFile>
#include "common.h"
#include "src/io/isds_sessions.h"

#define WIN_PREFIX "AppData/Roaming"
#define CONF_SUBDIR ".dsgui"
#define CONF_FILE "dsgui.conf"
#define ACCOUNT_DB_FILE "messages.shelf.db"


GlobPreferences globPref;
GlobProxySettings globProxSet;


/* Defaults. */
static const
GlobPreferences dlftlGlobPref; /* Defaults. */
static const
GlobProxySettings dfltGlobProxSet;


/* ========================================================================= */
GlobPreferences::GlobPreferences(void)
/* ========================================================================= */
    : loadFromConf(CONF_FILE),
    saveToConf(CONF_FILE),
    accountDbFile(ACCOUNT_DB_FILE),
    auto_download_whole_messages(false),
    default_download_signed(true),
    //store_passwords_on_disk(false),
    store_messages_on_disk(true),
    store_additional_data_on_disk(true),
    certificate_validation_date(DOWNLOAD_DATE),
    check_crl(true),
    check_new_versions(true),
    send_stats_with_version_checks(true),
    date_format(DATE_FORMAT_DEFAULT),
    language("system"), /* Use local settings. */
    after_start_select(SELECT_NEWEST)
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
		after_start_select = SELECT_LAST_VISITED;
		break;
	default:
		after_start_select = dlftlGlobPref.after_start_select;
		Q_ASSERT(0);
		break;
	}
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

	settings.endGroup();
}


/* ========================================================================= */
/*
 * Return path to configuraton directory.
 */
QString GlobPreferences::confDir(void)
/* ========================================================================= */
{
	QDir homeDir(QDir::homePath());

	if (homeDir.exists(WIN_PREFIX) && !homeDir.exists(CONF_SUBDIR)) {
		/* Set windows directory. */
		homeDir.cd(WIN_PREFIX);
	}

	return homeDir.path() + "/" CONF_SUBDIR;
}


/* ========================================================================= */
GlobProxySettings::GlobProxySettings(void)
/* ========================================================================= */
    : https_proxy("-1"),
    http_proxy("-1")
{
}


/* ========================================================================= */
GlobProxySettings::~GlobProxySettings(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void GlobProxySettings::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	https_proxy = settings.value("connection/https_proxy",
	    dfltGlobProxSet.https_proxy).toString();
	http_proxy = settings.value("connection/http_proxy",
	    dfltGlobProxSet.http_proxy).toString();

	/* TODO */
}


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void GlobProxySettings::saveToSettings(QSettings &settings) const
/* ========================================================================= */
{
	settings.beginGroup("connection");

	if ("-1" == https_proxy) {
		settings.setValue("https_proxy", https_proxy);
	} else {
		/* TODO */
	}

	if ("-1" == http_proxy) {
		settings.setValue("http_proxy", http_proxy);
	} else {
		/* TODO */
	}

	settings.endGroup();
}


QString dateTimeDisplayFormat("dd.MM.yyyy HH:mm:ss");


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
 * Translatest author type to text.
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
		    "infected paper deleted; final status before deletion");
		break;
	case 4:
		/* Zprava dodana do ISDS (zapsan cas dodani). */
		return QObject::tr("Message handed into ISDS "
		    "(delivery time recorded)");
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
		    "by ESS action)");
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
 * Changes all occurrences of '\' to '/' in given file.
 */
void fixBackSlashesInFile(const QString &fileName)
/* ========================================================================= */
{
	QString line;
	QFile file(fileName);

	if (file.open(QIODevice::ReadWrite|QIODevice::Text)) {
		QTextStream in(&file);
		line = in.readAll();
		line.replace(QString("\\"), QString("/"));
		file.reset();
		in << line;
		file.close();
	} else {
		qDebug() << "Error: Cannot open file '" << fileName << "'";
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
 * Convert hash algotirhm to string
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
 * Convert hash algotirhm to int
 */
int convertHashAlg2(QString value)
/* ========================================================================= */
{
	if (value == "MD5") return HASH_ALGORITHM_MD5;
	else if (value == "SHA_1") return HASH_ALGORITHM_SHA_1;
	else if (value == "SHA_224") return HASH_ALGORITHM_SHA_224;
	else if (value == "SHA_256") return HASH_ALGORITHM_SHA_256;
	else if (value == "SHA_384") return HASH_ALGORITHM_SHA_384;
	else if (value == "SHA_512") return HASH_ALGORITHM_SHA_512;
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
