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


#ifndef _COMMON_H_
#define _COMMON_H_


#include <QLocale>
#include <QSettings>
#include <QString>
#include <QDebug>
#include <QDir>

#define TMP_ATTACHMENT_PREFIX "qdatovka_XXXXXX_"

#define ID_ISDS_SYS_DATABOX "aaaaaaa"
#define ICON_14x14_PATH ":/icons/14x14/"
#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"
#define ISDS_PING_TIMEOUT_MS 10000
#define ISDS_CONNECT_TIMEOUT_MS 10000 /* libisds connection time-out. */
#define ISDS_DOWNLOAD_TIMEOUT_MS 300000
#define RUN_FIRST_ACTION_MS 3000 // 3 sec run action after datovka start
#define TIMER_DEFAULT_TIMEOUT_MS 600000 // 10 min timer period
#define DLG_ISDS_KEEPALIVE_MS 180000 // 3 min dialog isds ping timer period
#define MAX_ATTACHMENT_SIZE 10000000 // 10MB
#define TIMER_STATUS_TIMEOUT_MS 5000 // 5s will message in status bar shown
#define TIMER_MARK_MSG_READ_MS 5000 /* Mark message as read after 5 seconds. */

#define CZ_NIC_URL "https://www.nic.cz"
#define DATOVKA_ONLINE_HELP_URL "https://labs.nic.cz/cs/datovka.html"
#define DATOVKA_OFFLINE_HELP_URL "file:///help/index.html"
#define DATOVKA_CHECK_NEW_VERSION_URL "https://secure.nic.cz/files/datove_schranky/Version"
#define DATOVKA_DOWNLOAD_URL "https://labs.nic.cz/cs/datovka.html"
#define PWD_EXPIRATION_NOTIFICATION_DAYS 7 // show expiration date dialog before xx days

/* return values of Datovka login methods */
typedef enum {
	USER_NAME = 0,
	CERTIFICATE = 1,
	USER_CERTIFICATE = 2,
	HOTP = 3,
	TOTP = 4
} LoginMethodsIndex;

/* return values of Datovka message state */
enum MessageProcessState {
	UNSETTLED = 0,
	IN_PROGRESS = 1,
	SETTLED = 2
};

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

/* Message direction. */
enum MessageDirection {

	/* Use only for advanced search. */
	MSG_ALL = 0,

	/* Received message type in table supplementary_message_data is set
	 * to 1; message_type = 1.
	 * Must always be set to 1 because of old Datovka compatibility.
	 */
	MSG_RECEIVED = 1,

	/* Sent message type in table supplementary_message_data is set
	 * to 2; message_type = 2.
	 * Must always be set to 2 because of old Datovka compatibility.
	 */
	MSG_SENT = 2
};


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
#define ROLE_MSGS_DB_PROXYSORT (Qt::UserRole + 4) /*
                                                   * Used for sorting according
                                                   * to boolean values which
                                                   * are displayed as icons.
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

	enum SelectType {
		SELECT_NEWEST = 1,
		SELECT_LAST_VISITED = 2,
		SELECT_NOTHING = 3
	};


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
	enum SelectType after_start_select;
	const int message_mark_as_read_timeout;
	bool use_global_paths;
	QString save_attachments_path;
	QString add_file_to_attachments_path;
	bool all_attachments_save_zfo_delinfo;
	bool all_attachments_save_zfo_msg;
	bool all_attachments_save_pdf_msgenvel;
	bool all_attachments_save_pdf_delinfo;
	int isds_download_timeout_ms;

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
		return confDir() + QDir::separator() + loadFromConf;
	}

	/*!
	 * @brief Returns whole configuration file path.
	 */
	inline
	QString saveConfPath(void) const
	{
		return confDir() + QDir::separator() + saveToConf;
	}

	/*!
	 * @brief Returns whole account db path.
	 */
	inline
	QString accountDbPath(void) const
	{
		return confDir() + QDir::separator() + accountDbFile;
	}
};


class ProxiesSettings {
public:
	static
	const QString noProxyStr;
	static
	const QString autoProxyStr;

	class ProxySettings {
	public:
		ProxySettings(void);

		QString hostName;
		int port;
		QString userName;
		QString password;
	};

	ProxiesSettings(void);

	ProxySettings https;
	ProxySettings http;

	/*!
	 * @brief Load data from supplied settings.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Detect HTTP proxy. Return host and port number.
	 */
	static
	ProxiesSettings::ProxySettings detectHttpProxy(void);
};


/* Global preferences structure. */
extern GlobPreferences globPref;
extern ProxiesSettings globProxSet;

/*!
 * @brief Date/time format used in the application.
 */
extern
const QString dateTimeDisplayFormat;
extern
const QString dateDisplayFormat;

/* Global locale instance. */
extern
QLocale programLocale;

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
 * @brief Fix account password format = compatibility with old Datovka.
 */
void removeDoubleQuotesFromAccountPassword(const QString &fileName);


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
 * Convert type of user to string
 */
const QString & convertUserTypeToString(int value);

/*!
 * @brief Convert type of databox to int
 */
int convertDbTypeToInt(QString value);

/*!
 * @brief Convert event type to string
 */
QString convertEventTypeToString(int value);

/*!
 * @brief Return privilegs as html string from number representation.
 */
QString convertUserPrivilsToString(int userPrivils);

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
QString appLocalisationDir(void);


/*!
 * @brief Returns the path to directory where supplied localisation resides.
 */
QString qtLocalisationDir(void);


enum WriteFileState {
	WF_SUCCESS = 0, /*!< File was successfully created and written. */
	WF_CANNOT_CREATE, /*!< File could not be created. */
	WF_CANNOT_WRITE_WHOLE, /*!< File could not be entirely written. */
	WF_ERROR /*!< Different error. */
};


/*!
 * @brief Create and write data to file.
 *
 * @param[in] fileName      File name.
 * @param[in] data          Data to be written into file.
 * @param[in] deleteOnError Delete created file when cannot be entirely
 *                          written.
 */
enum WriteFileState writeFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError = false);


/*!
 * @brief Create and write data to temporary file.
 *
 * @param[in] fileName      File name.
 * @param[in] data          Data to be written into file.
 * @param[in] deleteOnError Delete created file when cannot be entirely
 *                          written.
 * @return Full path to written file on success, empty string on failure.
 */
QString writeTemporaryFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError = false);


#endif /* _COMMON_H_ */
