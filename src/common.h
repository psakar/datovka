

#ifndef _COMMON_H_
#define _COMMON_H_


#include <QSettings>
#include <QString>
#include <QDebug>

#define TMP_ATTACHMENT_PREFIX "qdatovka_XXXXXX_"

#define ICON_14x14_PATH ":/icons/14x14/"
#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"
#define VERSION "0.2.0"
#define TIMEOUT_MS 10000 /* libisds connection time-out. */
#define RUN_FIRST_ACTION_MS 3000 // 3 sec run action after datovka start
#define TIMER_DEFAULT_TIMEOUT_MS 600000 // 10 min timer period
#define DLG_ISDS_KEEPALIVE_MS 180000 // 3 min dialog isds ping timer period
#define MAX_ATTACHMENT_SIZE 10000000 // 10MB
#define TIMER_STATUS_TIMEOUT_MS 5000 // 5s will message in status bar shown
#define DATOVKA_ONLINE_HELP_URL "https://labs.nic.cz/page/2425/"
#define DATOVKA_OFFLINE_HELP_URL "file:///help/index.html"
#define DATOVKA_CHECK_NEW_VERSION_URL "https://secure.nic.cz/files/datove_schranky/Version"
#define DATOVKA_DOWNLOAD_URL "https://labs.nic.cz/page/2425/"

/* return values of Datovka login methods */
typedef enum {
	USER_NAME = 0,
	CERTIFICATE = 1,
	USER_CERTIFICATE = 2,
	HOTP = 3,
	TOTP = 4
} LoginMethodsIndex;

/* return values of Datovka message state */
typedef enum {
	UNSETTLED = 0,
	IN_PROGRESS = 1,
	SETTLED = 2
} MessageProcessState;

/* return values of Datovka functions */
typedef enum {
	Q_SUCCESS = 0,   // all operations success
	Q_CANCEL,        // operation cancelled or file dialog cancelled
	Q_GLOBAL_ERROR,  // any qdatovka error
	Q_CONNECT_ERROR, // ISDS login error
	Q_NETWORK_ERROR, // error
	Q_ISDS_ERROR,
	Q_SQL_ERROR,
	Q_FILE_ERROR,
	Q_DIALOG_ERROR,
	Q_NOTEQUAL
} qdatovka_error;


/*
 * Defined roles across the application.
 */
#define ROLE_ACNT_CONF_SETTINGS (Qt::UserRole + 1) /*
                                                    * Used to access
                                                    * configuration data.
                                                    */
#define ROLE_ACNT_UNREAD_MSGS (Qt::UserRole + 2) /*
                                                  * Used to store number of
                                                  * unread messages.
                                                  */
#define ROLE_MSGS_DB_ENTRY_TYPE (Qt::UserRole + 3) /*
                                                    * Used to determine the db
                                                    * data type of the column.
                                                    */


#define strongAccountInfoLine(title, value) \
	(QString("<div><strong>") + (title) + ": </strong>" + (value) + \
	"</div>")
#define accountInfoLine(title, value) \
	(QString("<div>") + (title) + ": " + (value) + "</div>")
#define indentDivStart "<div style=\"margin-left: 12px;\">"
#define divEnd "</div>"

#define strongMessagePdf(title) \
	(QString("<strong>") + (title) + QString("</strong>"))

#define messageTableSectionPdf(title) \
	(QString("<table width=\"100%\" style=\"background-color: #FFFF00; padding: 40px 20px 40px 20px; font-size: 18px;\"><tr><td>") + \
	title + QString("</td></tr></table>"))

#define messageTableInfoStartPdf() \
	(QString("<table style=\"margin-left: 10px; margin-top: 10px; margin-bottom: 30px; font-size: 16px;\">"))

#define messageTableInfoPdf(title, value) \
	(QString("<tr><td>") + title + QString(": ") + \
	QString("</td><td>") + value + QString("</td></tr>"))

#define messageTableInfoEndPdf() (QString("</table>"))





class GlobPreferences {

public:
	typedef enum {
		DOWNLOAD_DATE = 1,
		CURRENT_DATE = 2
	} CertValDate;

	typedef enum {
		DATE_FORMAT_LOCALE = 1,
		DATE_FORMAT_ISO = 2,
		DATE_FORMAT_DEFAULT = 3//,
		//DATE_FORMAT_CUSTOM = 4
	} DateFmt;

	typedef enum {
		SELECT_NEWEST = 1,
		SELECT_LAST_VISITED = 2,
		SELECT_NOTHING = 3
	} SelectType;


	GlobPreferences(void);
	~GlobPreferences(void);

	QString confSubdir; /*!< Configuration directory. */
	QString loadFromConf; /*!< Configuration file to load from. */
	QString saveToConf; /*!< Configuration file to save to. */
	const QString accountDbFile; /*!< Account db file. */
	bool auto_download_whole_messages;
	bool default_download_signed; /*!< Default downloading method. */
	//bool store_passwords_on_disk;
	bool store_messages_on_disk;
	bool store_additional_data_on_disk;
	CertValDate certificate_validation_date;
	bool check_crl;
	bool check_new_versions;
	bool send_stats_with_version_checks;
	bool download_on_background;
	int timer_value;
	bool download_at_start;
	DateFmt date_format;
	QString language;
	SelectType after_start_select;

	/*!
	 * @brief Load data from supplied settings.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Return path to configuration directory.
	 */
	QString confDir(void) const;

	/*!
	 * @brief Returns whole configuration file path.
	 */
	inline
	QString loadConfPath(void) const
	{
		return confDir() + "/" + loadFromConf;
	}

	/*!
	 * @brief Returns whole configuration file path.
	 */
	inline
	QString saveConfPath(void) const
	{
		return confDir() + "/" + saveToConf;
	}

	/*!
	 * @brief Returns whole account db path.
	 */
	inline
	QString accountDbPath(void) const
	{
		return confDir() + "/" + accountDbFile;
	}
};


class GlobProxySettings {

public:
	GlobProxySettings(void);
	~GlobProxySettings(void);

	QString https_proxy;
	QString http_proxy;
	/* TODO -- Additional settings according to ProxyManager on Datovka. */

	/*!
	 * @brief Load data from supplied settings.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 */
	void saveToSettings(QSettings &settings) const;
};


/* Global preferences structure. */
extern GlobPreferences globPref;
extern GlobProxySettings globProxSet;

/*!
 * @brief Date/time format used in the application.
 */
extern
const QString dateTimeDisplayFormat;
extern
const QString dateDisplayFormat;


/*!
 * @brief Translates message type to text.
 */
const QString dmTypeToText(const QString &dmType);


/*!
 * @brief Translates author type to text.
 */
const QString authorTypeToText(const QString &authorType);


/*!
 * @brief Returns message status description.
 */
const QString msgStatusToText(int status);


/*!
 * @brief Changes all occurrences of '\' to '/' in given file.
 */
void fixBackSlashesInFile(const QString &fileName);


/*!
 * @brief Fix account password format = compatability with old datovka.
 */
void removeQuoteFromAccountPassword(const QString &fileName);


/*!
 * @brief Return dec index from hex.
 */
int convertHexToDecIndex(int value);

/*!
 * @brief Return hash algorithm as string
 */
QString convertHashAlg(int value);

 /*!
 * @brief Return hash algorithm as int
 */
int convertHashAlg2(QString value);

/*!
 * @brief Return attachment type as string
 */
QString convertAttachmentType(int value);

/*!
 * @brief Convert type of author to string
 */
QString convertSenderTypeToString(int value);

/*!
 * @brief Convert type of databox to string
 */
QString convertDbTypeToString(int value);

/*!
 * @brief Convert type of databox to int
 */
int convertDbTypeToInt(QString value);

/*!
 * @brief Convert event type to string
 */
QString convertEventTypeToString(int value);

/*!
 * @brief Convert state box to text
 */
QString getdbStateText(int value);

/*!
 * @brief Converts base64 encoded string into plain text.
 */
QString fromBase64(const QString &base64);

/*!
 * @brief Converts string into base64.
 */
QString toBase64(const QString &plain);


/*!
 * @brief Text files supplied with the application.
 */
enum text_file {
	TEXT_FILE_CREDITS = 1,
	TEXT_FILE_LICENCE
};


/*!
 * @brief Returns the content of the supplied text file.
 */
QString suppliedTextFileContent(enum text_file textFile);


/*!
 * @brief Returns the path to directory where supplied localisation resides.
 */
QString suppliedLocalisationDir(void);

#endif /* _COMMON_H_ */
