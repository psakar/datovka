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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <cstdlib>
#include <isds.h>
#include <openssl/crypto.h> /* SSLeay_version(3) */
#include <QRegularExpression>
#include <QVersionNumber>

#include "src/about.h"
#include "src/log/log.h"

QStringList libraryDependencies(void)
{
	QStringList libs;

	libs.append(QStringLiteral("Qt ") + qVersion());

	char *isdsVer = isds_version();
	libs.append(QStringLiteral("libisds ") + isdsVer);
	free(isdsVer); isdsVer = NULL;

	libs.append(SSLeay_version(SSLEAY_VERSION));

	return libs;
}

/*!
 * @brief Strip unwanted data from version string.
 *
 * @param[in,out] vStr Version string. Must contain a substring in format
 *                     '[0-9]+.[0-9]+.[0-9]+' .
 * @return True if such substring is found.
 */
static
bool stripVersionString(QString &vStr)
{
	vStr.remove(QRegularExpression(QLatin1String("^[^0-9.]*")));
	vStr.remove(QRegularExpression(QLatin1String("[^0-9.].*$")));
	vStr.remove(QRegularExpression(QLatin1String("^[.]*")));
	vStr.remove(QRegularExpression(QLatin1String("[.]*$")));

	QRegularExpression verExpr(
	    QLatin1String("[0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*"));
	QRegularExpressionMatch match(verExpr.match(vStr));

	return match.hasMatch() && (match.capturedLength() == vStr.length());
}

int compareVersionStrings(const QString &vStr1, const QString &vStr2)
{
	QString vs1(vStr1), vs2(vStr2);

	if (!stripVersionString(vs1)) {
		logErrorNL("Cannot strip version string '%s'.",
		    vs1.toUtf8().constData());
		return -2;
	}
	if (!stripVersionString(vs2)) {
		logErrorNL("Cannot strip version string '%s'.",
		    vs2.toUtf8().constData());
		return 2;
	}

	QVersionNumber v1 = QVersionNumber::fromString(vs1);
	if (v1.isNull()) {
		logErrorNL(
		    "Version string '%s' does not match required format.",
		    vs1.toUtf8().constData());
		return -2;
	}
	QVersionNumber v2 = QVersionNumber::fromString(vs2);
	if (v2.isNull()) {
		logErrorNL(
		    "Version string '%s' does not match required format.",
		    vs2.toUtf8().constData());
		return 2;
	}

	/*
	 * Documentation of QVersionNumber::compare() only mentions negative
	 * or positive values. It does not mention -1 or 1.
	 */
	int cmp = QVersionNumber::compare(v1, v2);
	if (cmp < 0) {
		return -1;
	} else if (cmp == 0) {
		return 0;
	} else {
		return 1;
	}

	return cmp;
}
