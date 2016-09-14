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
#define POS_UNAME 2
#define POS_PWD 4
#define POS_LIST_MIN 4

void LoginCredentials::clearAll(void)
{
	boxName.clear();
	userName.clear();
	pwd.clear();
}

bool LoginCredentials::loadLoginCredentials(const QString &filePath,
    unsigned lineNum)
{
	/*
	 * File must be organised in this way:
	 *
	 * :user_name_01:password_01:.....
	 *
	 * First character on the line is the separator which is used to tonise
	 * the line. Immediately after the separator starts the string with the
	 * login. Immediately after follows the separator character.
	 * Immediately after follows the password string. Some other entries may
	 * also follow.
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

	if (list.size() < POS_LIST_MIN) {
		return false;
	}

	clearAll();

	boxName = list.at(POS_BNAME);
	userName = list.at(POS_UNAME);
	pwd = list.at(POS_PWD);

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
	QTextStream fileStream(&file);

	unsigned readLine = 0;

	while (!file.atEnd() && readLine < lineNum) {
		++readLine;
		if (readLine == lineNum) {
			line = fileStream.readLine();
		} else {
			fileStream.readLine();
		}
	}

	return line;
}
