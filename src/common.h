

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


typedef enum {
	USER_NAME = 0,
	CERTIFICATE = 1,
	USER_CERTIFICATE = 2,
	HOTP = 3,
	TOTP = 4
} LoginMethodsIndex;


/*
 * Defined roles accross the application. 
 */
#define ROLE_CONF_SETINGS (Qt::UserRole + 1) /*
                                              * Used to access configuration
                                              * data.
                                              */
//#define ROLE_DB (Qt::UserRole + 2)
#define ROLE_DB_ENTRY_TYPE (Qt::UserRole + 3) /*
                                               * Used to determine the db data
                                               * type of the column.
                                               */


#define strongAccountInfoLine(title, value) \
	(QString("<div><strong>") + (title) + ": </strong>" + (value) + \
	"</div>")
#define accountInfoLine(title, value) \
	(QString("<div>") + (title) + ": " + (value) + "</div>")
#define indentDivStart "<div style=\"margin-left: 12px;\">"
#define divEnd "</div>"

class AccountStructInfo {
public:
	QString login_method;
	QString userName;
	QString password;
	bool testAccount;
	QString certPath;
};

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


#endif /* _COMMON_H_ */
