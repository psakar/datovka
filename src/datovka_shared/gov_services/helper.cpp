/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QRegularExpression>
#include <QtGlobal>

#include "src/datovka_shared/gov_services/helper.h"

QString Gov::Helper::withoutWhiteSpaces(const QString &str)
{
	const static QRegularExpression wsRe("\\s+");
	QString retStr(str);

	return retStr.replace(wsRe, QString());
}

#define MAX_IC_LEN 8 /* IC has 8 digits. */

/*!
 * @brief Check number whether it contains a valid IC. Also checks the checksum
 *     digit value.
 *
 * @param[in] num Number to be checked.
 * @return True if number a is a valid IC.
 */
static
bool isValidIcNum(qint64 num)
{
	if ((num < 0) || (num > Q_INT64_C(99999999))) {
		return false;
	}

	int d8 = (num / 10000000) % 10;
	int d7 = (num / 1000000) % 10;
	int d6 = (num / 100000) % 10;
	int d5 = (num / 10000) % 10;
	int d4 = (num / 1000) % 10;
	int d3 = (num / 100) % 10;
	int d2 = (num / 10) % 10;
	int d1 = num % 10; /* Lowest digit is a checksum digit. */

	int x = (11 - ((8 * d8 + 7 * d7 + 6 * d6 + 5 * d5 + 4 * d4 + 3 * d3 + 2 * d2) % 11)) % 10;
	return d1 == x;
}

bool Gov::Helper::isValidIcStr(const QString &str)
{
	const static QRegularExpression numRe("\\d+");
	QRegularExpressionMatchIterator it(numRe.globalMatch(str));
	if (it.hasNext()) {
		const QRegularExpressionMatch match(it.next());
		if (str != match.captured()) {
			return false;
		}
	} else {
		return false;
	}

	if (str.length() > MAX_IC_LEN) {
		return false;
	}

	bool iOk = false;
	qint64 ic = str.toLongLong(&iOk);
	if (!iOk) {
		return false;
	}

	return isValidIcNum(ic);
}
