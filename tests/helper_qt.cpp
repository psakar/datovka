/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "src/log/log.h"
#include "tests/helper_qt.h"

/* First position is an empty string. */
#define POS_BNAME 1
#define POS_TYPE 2
#define POS_UNAME 3
#define POS_PWD 4

#define POS_CRT 5
#define POS_KEY 6
#define POS_HOTP 7

#define POS_LIST_MIN 5

LoginCredentials::LoginCredentials(void)
    : boxName(),
    loginType(AcntSettings::LIM_UNKNOWN),
    userName(),
    pwd(),
    crtPath(),
    passphrase(),
    hotp(),
    totp()
{
}

void LoginCredentials::clearAll(void)
{
	boxName.clear();
	loginType = AcntSettings::LIM_UNKNOWN;
	userName.clear();
	pwd.clear();
	crtPath.clear();
	passphrase.clear();
	hotp.clear();
	totp.clear();
}

bool LoginCredentials::loadLoginCredentials(const QString &filePath,
    unsigned lineNum)
{
	/*
	 * File must be organised in this way:
	 *
	 * :box_name_01:login_type_01:user_name_01:password_01:cert_path_01:cert_key_01:hotp_01:totp_01
	 *
	 * First character on the line is the separator which is used to split
	 * the line. Immediately after the separator starts the string with the
	 * box name. Immediately after follows the separator character.
	 * Immediately after follows the login and password string. Some other
	 * entries may also follow.
	 */

	if (lineNum == 0) {
		/* Line numbering starts at 1. */
		return false;
	}

	if (!isReadableFile(filePath)) {
		return false;
	}

	QString line(readLine(filePath, lineNum));

	if (line.isEmpty()) {
		return false;
	}

	QChar separator = line.at(0);
	QStringList list(line.split(separator));

	int minSize = POS_LIST_MIN;
	if (list.size() < minSize) {
		return false;
	}

	clearAll();

	switch (typeFromStr(list.at(POS_TYPE))) {
	case AcntSettings::LIM_UNAME_PWD:
		break;
	case AcntSettings::LIM_UNAME_CRT:
	case AcntSettings::LIM_UNAME_PWD_CRT:
		minSize += 2;
		break;
	case AcntSettings::LIM_UNAME_PWD_HOTP:
		minSize += 3;
		break;
	case AcntSettings::LIM_UNKNOWN:
	default:
		return false;
		break;
	}

	boxName = list.at(POS_BNAME);
	loginType = typeFromStr(list.at(POS_TYPE));
	Q_ASSERT(loginType != AcntSettings::LIM_UNKNOWN);
	userName = list.at(POS_UNAME);
	if (loginType != AcntSettings::LIM_UNAME_CRT) {
		pwd = list.at(POS_PWD);
	}
	if (loginType == AcntSettings::LIM_UNAME_CRT ||
	    loginType == AcntSettings::LIM_UNAME_PWD_CRT) {
		crtPath = list.at(POS_CRT);
		passphrase = list.at(POS_KEY);
	} else 	if (loginType == AcntSettings::LIM_UNAME_PWD_HOTP) {
		hotp = list.at(POS_HOTP);
	}

	return true;
}

bool LoginCredentials::isReadableFile(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}

	QFileInfo fInfo(filePath);

	return fInfo.exists() && fInfo.isFile() && fInfo.isReadable();
}

QString LoginCredentials::readLine(const QString &filePath, unsigned lineNum)
{
	Q_ASSERT(!filePath.isEmpty());
	Q_ASSERT(lineNum > 0);

	QString line;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		return line;
	}

	{
		QTextStream fileStream(&file);

		unsigned readLine = 0;

		while (!fileStream.atEnd() && readLine < lineNum) {
			++readLine;
			if (readLine == lineNum) {
				line = fileStream.readLine();
			} else {
				fileStream.readLine();
			}
		}
	}

	file.close();

	return line;
}

enum AcntSettings::LogInMethod LoginCredentials::typeFromStr(
    const QString &typeStr)
{
	/* Supported values are:
	 * "USER_PWD"
	 * "USER_CRT"
	 * "PWD_CERT"
	 * "PWD_HOTP"
	 */

	if (typeStr == QLatin1String("USER_PWD")) {
		return AcntSettings::LIM_UNAME_PWD;
	} else if (typeStr == QLatin1String("USER_CRT")) {
		return AcntSettings::LIM_UNAME_CRT;
	} else if (typeStr == QLatin1String("PWD_CERT")) {
		return AcntSettings::LIM_UNAME_PWD_CRT;
	} else if (typeStr == QLatin1String("PWD_HOTP")) {
		return AcntSettings::LIM_UNAME_PWD_HOTP;
	} else { /* TOTP is not supported. */
		return AcntSettings::LIM_UNKNOWN;
	}
}
