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


#ifndef _COMMON_H_
#define _COMMON_H_


#include <QLocale>
#include <QSettings>
#include <QString>
#include <QDebug>
#include <QDir>

#define ICON_14x14_PATH ":/icons/14x14/"
#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"

#define ID_ISDS_SYS_DATABOX "aaaaaaa"

#define ISDS_PING_TIMEOUT_MS 10000
#define ISDS_CONNECT_TIMEOUT_MS 10000 /* libisds connection time-out. */
#define ISDS_DOWNLOAD_TIMEOUT_MS 300000
#define TIMESTAMP_EXPIR_BEFORE_DAYS 15 /* Show timestamp expiration before days*/
#define RUN_FIRST_ACTION_MS 3000 // 3 sec run action after datovka start
#define TIMER_DEFAULT_TIMEOUT_MS 600000 // 10 min timer period
#define DLG_ISDS_KEEPALIVE_MS 180000 // 3 min dialog isds ping timer period
#define MAX_ATTACHMENT_SIZE_MB 20
#define MAX_ATTACHMENT_SIZE_BYTES 20000000 // 20MB
#define MAX_ATTACHMENT_FILES 900 // Number of attachment files is throttled at 900
#define TIMER_STATUS_TIMEOUT_MS 5000 // 5s will message in status bar shown
#define TIMER_MARK_MSG_READ_MS 5000 /* Mark message as read after 5 seconds. */

#define SUPPORT_MAIL "datove-schranky@labs.nic.cz"
#define CZ_NIC_URL "https://www.nic.cz"
#define DATOVKA_ONLINE_HELP_URL "https://secure.nic.cz/files/datove_schranky/redirect/prirucka.html"
#define DATOVKA_FAQ_URL "https://secure.nic.cz/files/datove_schranky/redirect/faq.html"
#define DATOVKA_HOMEPAGE_URL "https://labs.nic.cz/cs/datovka.html"
#define DATOVKA_CHECK_NEW_VERSION_URL "https://secure.nic.cz/files/datove_schranky/Version"
#define DATOVKA_DOWNLOAD_URL "https://labs.nic.cz/cs/datovka.html"
#define PWD_EXPIRATION_NOTIFICATION_DAYS 7 // show expiration date dialog before xx days

#define DB_MOJEID_NAME_PREFIX "mojeid-"

/* return values of Datovka login methods */
typedef enum {
	USER_NAME = 0,
	CERTIFICATE = 1,
	USER_CERTIFICATE = 2,
	HOTP = 3,
	TOTP = 4,
	MOJEID = 5
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
	Q_GLOBAL_ERROR,  // any qdatovka error
	Q_ISDS_ERROR,
	Q_SQL_ERROR
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
#define ROLE_PLAIN_DISPLAY (Qt::UserRole + 1) /* Low level data. */
#define ROLE_MSGS_DB_ENTRY_TYPE (Qt::UserRole + 2) /*
                                                    * Used to determine the db
                                                    * data type of the column.
                                                    */
#define ROLE_MSGS_DB_PROXYSORT (Qt::UserRole + 3) /*
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
 * @brief Return sender databox type as string from number representation.
 */
QString convertSenderDbTypesToString(int value);

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
 * @brief Computes the size of real (decoded from base64) data.
 */
int base64RealSize(const QByteArray &b64);

/*!
 * @brief Test if selected account is WebDatovka (MojeId) account.
 */
bool isWebDatovkaAccount(const QString &userName);

/*!
 * @brief Get/create WebDatovka (MojeId) username.
 */
QString getWebDatovkaUsername(const QString &userId, const QString &accountId);

/*!
 * Get account id from username of WebDatovka account.
 */
int getWebDatovkaAccountId(const QString &userName);

/*!
 * Get user id from username of WebDatovka account.
 */
int getWebDatovkaUserId(const QString &userName);

/*!
 * Get name prefix of tag database from username (WebDatovka).
 */
QString getWebDatovkaTagDbPrefix(const QString &userName);

/*!
 * @brief Check valid database filename.
 *
 * @param[in] inDbFileName Input database file name.
 * @param[out] dbUserName Username entry.
 * @paran[out] dbYear Year entry if exists or NULL.
 * @paran[out] dbTestingFlag True if account is testing or false
 * @paran[out] errMsg Error message to user
 * @return true if database filename is correct
 */
bool isValidDatabaseFileName(QString inDbFileName, QString &dbUserName,
    QString &dbYear, bool &dbTestingFlag, QString &errMsg);

#endif /* _COMMON_H_ */
