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

#include <QMimeDatabase>
#include <QMimeType>

#include "src/common.h"
#include "src/log/log.h"

const QString dateTimeDisplayFormat("dd.MM.yyyy HH:mm:ss");
const QString dateDisplayFormat("dd.MM.yyyy");

void addAttachmentToEmailMessage(QString &message, const QString &attachName,
    const QByteArray &base64, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */

	QMimeDatabase mimeDb;

	QMimeType mimeType(
	    mimeDb.mimeTypeForData(QByteArray::fromBase64(base64)));

	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: " + mimeType.name() + "; charset=UTF-8;" + newLine +
	    " name=\"" + attachName +  "\"" + newLine;
	message += "Content-Transfer-Encoding: base64" + newLine;
	message += "Content-Disposition: attachment;" + newLine +
	    " filename=\"" + attachName + "\"" + newLine;

	for (int i = 0; i < base64.size(); ++i) {
		if ((i % 60) == 0) {
			message += newLine;
		}
		message += base64.at(i);
	}
	message += newLine;
}

int base64RealSize(const QByteArray &b64)
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

void createEmailMessage(QString &message, const QString &subj,
    const QString &boundary)
{
	message.clear();

	const QString newLine("\n"); /* "\r\n" ? */

	/* Rudimentary header. */
	message += "Subject: " + subj + newLine;
	message += "MIME-Version: 1.0" + newLine;
	message += "Content-Type: multipart/mixed;" + newLine +
	    " boundary=\"" + boundary + "\"" + newLine;

	/* Body. */
	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: text/plain; charset=UTF-8" + newLine;
	message += "Content-Transfer-Encoding: 8bit" + newLine;

	message += newLine;
	message += "-- " + newLine; /* Must contain the space. */
	message += " " + QObject::tr("Created using Datovka") + " " VERSION "." + newLine;
	message += " <URL: " DATOVKA_HOMEPAGE_URL ">" + newLine;
}

void finishEmailMessage(QString &message, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */
	message += newLine + "--" + boundary + "--" + newLine;
}

QString fromBase64(const QString &base64)
{
	return QString::fromUtf8(QByteArray::fromBase64(base64.toUtf8()));
}

QString getWebDatovkaUsername(const QString &userId, const QString &accountId)
{
	return DB_MOJEID_NAME_PREFIX + userId + "-" + accountId;
}

int getWebDatovkaAccountId(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return -1;
	}

	const QString aID = userName.split("-").at(2);
	return aID.toInt();
}

int getWebDatovkaUserId(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return -1;
	}
	const QString uID = userName.split("-").at(1);
	return uID.toInt();
}

QString getWebDatovkaTagDbPrefix(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return QString();
	}

	return DB_MOJEID_NAME_PREFIX + userName.split("-").at(1);
}

bool isWebDatovkaAccount(const QString &userName)
{
	return userName.contains(DB_MOJEID_NAME_PREFIX);
}

bool isValidDatabaseFileName(QString inDbFileName,
    QString &dbUserName, QString &dbYear, bool &dbTestingFlag, QString &errMsg)
{
	QStringList fileNameParts;
	bool ret = false;
	errMsg = "";
	dbUserName = "";
	dbYear = "";

	if (inDbFileName.contains("___")) {
		// get username from filename
		fileNameParts = inDbFileName.split("_");
		if (fileNameParts.isEmpty() || fileNameParts.count() <= 1) {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "database filename.").arg(inDbFileName);
			return ret;
		}
		if (fileNameParts[0].isEmpty() ||
		    fileNameParts[0].length() != 6) {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "username in the database filename.").arg(inDbFileName);
			return ret;
		}
		dbUserName = fileNameParts[0];

		// get year from filename
		if (fileNameParts[1].isEmpty()) {
			dbYear = "";
		} else if (fileNameParts[1] == "inv") {
			dbYear = fileNameParts[1];
		} else if (fileNameParts[1].length() == 4) {
			dbYear = fileNameParts[1];
		} else {
			errMsg = QObject::tr("File '%1' does not contain valid "
			    "year in the database filename.").arg(inDbFileName);
			dbYear = "";
			return ret;
		}

		// get testing flag from filename
		fileNameParts = inDbFileName.split(".");
		if (fileNameParts.isEmpty()) {
			errMsg = QObject::tr("File '%1' does not contain valid "
			    "database filename.").arg(inDbFileName);
			return ret;
		}
		fileNameParts = fileNameParts[0].split("___");
		if (fileNameParts.isEmpty()) {
			errMsg = QObject::tr("File '%1' does not contain "
			    "valid database filename.").arg(inDbFileName);
			return ret;
		}

		if (fileNameParts[1] == "1") {
			dbTestingFlag = true;
		} else if (fileNameParts[1] == "0") {
			dbTestingFlag = false;
		} else {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "account type flag or filename has wrong format.").arg(inDbFileName);
			dbTestingFlag = false;
			return ret;
		}
	} else {
		errMsg = QObject::tr("File '%1' does not contain a valid message "
		    "database or filename has wrong format.").arg(inDbFileName);
		return ret;
	}

	return true;
}

QString toBase64(const QString &plain)
{
	return QString::fromUtf8(plain.toUtf8().toBase64());
}
