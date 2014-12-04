

#include <QApplication>
#include <QDir>
#include <QFile>
#include "common.h"
#include "src/io/isds_sessions.h"

#define WIN_PREFIX "AppData/Roaming"
/*!< Default configuration folder location. */
#define DFLT_CONF_SUBDIR ".dsgui"
/*!< Default configuration file name. */
#define DFLT_CONF_FILE "dsgui.conf"
#define ACCOUNT_DB_FILE "messages.shelf.db"


GlobPreferences globPref;
GlobProxySettings globProxSet;

/* Defaults. */
static const
GlobPreferences dlftlGlobPref; /* Defaults. */
static const
GlobProxySettings dfltGlobProxSet;


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
    store_additional_data_on_disk(true),
    certificate_validation_date(DOWNLOAD_DATE),
    check_crl(true),
    check_new_versions(true),
    send_stats_with_version_checks(true),
    download_on_background(true),
    timer_value(10),
    download_at_start(false),
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

	download_on_background = settings.value(
	    "preferences/download_on_background",
	    dlftlGlobPref.download_on_background).toBool();

	download_at_start = settings.value(
	    "preferences/download_at_start",
	    dlftlGlobPref.download_at_start).toBool();

	timer_value = settings.value(
	    "preferences/timer_value", dlftlGlobPref.timer_value).toInt();

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

	if (dlftlGlobPref.timer_value != timer_value) {
		settings.setValue("timer_value", timer_value);
	}

	if (dlftlGlobPref.download_on_background != download_on_background) {
		settings.setValue("download_on_background", download_on_background);
	}

	if (dlftlGlobPref.download_at_start != download_at_start) {
		settings.setValue("download_at_start", download_at_start);
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
	QDir homeDir(QDir::homePath());

	if (homeDir.exists(WIN_PREFIX) && !homeDir.exists(confSubdir)) {
		/* Set windows directory. */
		homeDir.cd(WIN_PREFIX);
	}

	return homeDir.path() + "/" + confSubdir;
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
void removeQuoteFromAccountPassword(const QString &fileName)
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

	filePath += QDir::separator() + fName;

	if (QFile::exists(filePath)) {
		return filePath;
	} else {
		filePath.clear();
	}

	return filePath;
}
