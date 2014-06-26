

#ifndef _COMMON_H_
#define _COMMON_H_


#include <QSettings>
#include <QString>
#include <QDebug>


#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"
#define VERSION "0.1"
#define TIMEOUT_MS 1000
#define LICENCE_PATH "COPYING";
#define CREDITS_PATH "AUTHORS";

typedef enum {
	USER_NAME = 0,
	CERTIFICATE = 1,
	USER_CERTIFICATE = 2,
	HOTP = 3,
	TOTP = 4
} LoginMethodsIndex;

/* retrun values of qDatovka functions */
typedef enum {
	Q_SUCCESS = 0,   // all operations success
	Q_CANCEL,	 // operation was canceled or file dialog was canceled
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
 * Defined roles accross the application. 
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
	static
	QString confDir(void);

	/*!
	 * @brief Returns whole configuration file path.
	 */
	inline
	QString loadConfPath(void)
	{
		return confDir() + "/" + loadFromConf;
	}

	/*!
	 * @brief Returns whole configuration file path.
	 */
	inline
	QString saveConfPath(void)
	{
		return confDir() + "/" + saveToConf;
	}

	/*!
	 * @brief Returns whole account db path.
	 */
	inline
	QString accountDbPath(void)
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
QString dateTimeDisplayFormat;


/*!
 * @brief Translates message type to text.
 */
const QString dmTypeToText(const QString &dmType);


/*!
 * @brief Translatest author type to text.
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
 * @brief Return dec index from hex.
 */
int convertHexToDecIndex(int value);

/*!
 * @brief Return hash algotirhm as string
 */
QString convertHashAlg(int value);

 /*!
 * @brief Return hash algotirhm as int
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
 * @brief Convert event type to string
 */
QString convertEventTypeToString(int value);

#endif /* _COMMON_H_ */
