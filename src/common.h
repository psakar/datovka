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

#include <QDir>
#include <QSettings>
#include <QString>

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
#define DATOVKA_HOMEPAGE_URL "https://www.datovka.cz/"
#define DATOVKA_CHECK_NEW_VERSION_URL "https://secure.nic.cz/files/datove_schranky/Version"
#define DATOVKA_DOWNLOAD_URL "https://www.datovka.cz/"
#define PWD_EXPIRATION_NOTIFICATION_DAYS 7 // show expiration date dialog before xx days

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

/* Expecting arguments of type QString. */
#define strongAccountInfoLineNoEscape(title, value) \
	(QLatin1String("<div><strong>") + (title) + \
	QLatin1String(": </strong>") + (value) + \
	QLatin1String("</div>"))
#define strongAccountInfoLine(title, value) \
	(QLatin1String("<div><strong>") + (title).toHtmlEscaped() + \
	QLatin1String(": </strong>") + (value).toHtmlEscaped() + \
	QLatin1String("</div>"))
#define accountInfoLine(title, value) \
	(QLatin1String("<div>") + (title).toHtmlEscaped() + \
	QLatin1String(": ") + (value).toHtmlEscaped() + QLatin1String("</div>"))
#define indentDivStart QLatin1String("<div style=\"margin-left: 12px;\">")
#define divEnd QLatin1String("</div>")

#define strongMessagePdf(title) \
	(QLatin1String("<strong>") + (title).toHtmlEscaped() + \
	QLatin1String("</strong>"))

#define messageTableSectionPdf(title) \
	(QLatin1String("<table width=\"100%\" style=\"background-color: #FFFF00; padding: 40px 20px 40px 20px; font-size: 18px;\"><tr><td>") + \
	(title).toHtmlEscaped() + QLatin1String("</td></tr></table>"))

#define messageTableInfoStartPdf() \
	(QLatin1String("<table style=\"margin-left: 10px; margin-top: 10px; margin-bottom: 30px; font-size: 16px;\">"))

#define messageTableInfoPdf(title, value) \
	(QLatin1String("<tr><td>") + (title).toHtmlEscaped() + \
	QLatin1String(": ") + QLatin1String("</td><td>") + \
	(value).toHtmlEscaped() + QLatin1String("</td></tr>"))

#define messageTableInfoEndPdf() (QLatin1String("</table>"))

/*!
 * @brief Date/time format used in the application.
 */
extern
const QString dateTimeDisplayFormat;
extern
const QString dateDisplayFormat;

/*!
 * @brief Adds attachment into email.
 *
 * @param[in,out] message Message body.
 * @param[in] attachName  Attachment file name.
 * @param[in] base64      Attagment content in Base64.
 * @param[in] boundary    Boundary string.
 */
void addAttachmentToEmailMessage(QString &message, const QString &attachName,
    const QByteArray &base64, const QString &boundary);

/*!
 * @brief Computes the size of real (decoded from base64) data.
 *
 * @param[in] b64 Data in Base64.
 * @return Size of real data.
 */
int base64RealSize(const QByteArray &b64);

/*!
 * @brief Creates email header and message body.
 *
 * @param[in,out] message Message body.
 * @param[in] subj        Subject string.
 * @param[in] boundary    Boundary string.
 */
void createEmailMessage(QString &message, const QString &subj,
    const QString &boundary);

/*!
 * @brief Adds last line into email.
 *
 * @param[in,out] message Message body.
 * @param[in] boundary    Boundary string.
 */
void finishEmailMessage(QString &message, const QString &boundary);

/*!
 * @brief Converts base64 encoded string into plain text.
 *
 * @param[in] base64 String in Base64.
 * @return Decoded string.
 */
QString fromBase64(const QString &base64);

/*!
 * @brief Check valid database filename.
 *
 * @param[in] inDbFileName   Input database file name.
 * @param[out] dbUserName    Username entry.
 * @paran[out] dbYear        Year entry if exists or NULL.
 * @paran[out] dbTestingFlag True if account is testing or false
 * @paran[out] errMsg        Error message to user
 * @return true if database filename is correct
 */
bool isValidDatabaseFileName(QString inDbFileName, QString &dbUserName,
    QString &dbYear, bool &dbTestingFlag, QString &errMsg);

/*!
 * @brief Converts string to base64.
 *
 * @param[in] plain Plain input string.
 * @return text string in Base64.
 */
QString toBase64(const QString &plain);

#endif /* _COMMON_H_ */
