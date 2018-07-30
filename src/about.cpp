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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
#include <QVersionNumber>
#else /* < Qt-5.6 */
#include <QVector>
#endif /* >= Qt-5.6 */

#include "src/about.h"
#include "src/datovka_shared/log/log.h"

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

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
#warning "Compiling against version < Qt-5.6 which does not have QVersionNumber."

/*!
 * @brief Replacement for QVersionNumber which is not presnet in Qt before 5.6.
 */
class QVersionNumber {
public:
	QVersionNumber(void) : m_major(-1), m_micro(-1), m_minor(-1)
	{
	}

	bool isNull(void) const
	{
		return (m_major < 0) || (m_micro < 0) || (m_minor < 0);
	}

	static
	int compare(const QVersionNumber &v1, const QVersionNumber &v2)
	{
		if (v1.m_major < v2.m_major) {
			return -1;
		} else if (v1.m_major > v2.m_major) {
			return 1;
		} else if (v1.m_micro < v2.m_micro) {
			return -1;
		} else if (v1.m_micro > v2.m_micro) {
			return 1;
		} else if (v1.m_minor < v2.m_minor) {
			return -1;
		} else if (v1.m_minor > v2.m_minor) {
			return 1;
		} else {
			return 0;
		}
	}

	static
	QVersionNumber fromString(const QString &str)
	{
		QVersionNumber verNum;

		const int elemNum = 3;
		QStringList elemList(str.split(QChar('.')));
		if (elemList.size() != elemNum) {
			return verNum;
		}

		QVector<int> elemVect(3, -1);
		for (int i = 0; i < elemNum; ++i) {
			bool ok = false;
			elemVect[i] = elemList[i].toInt(&ok);
			if (!ok) {
				elemVect[i] = -1;
			}
		}

		verNum.m_major = elemVect[0];
		verNum.m_micro = elemVect[1];
		verNum.m_minor = elemVect[2];
		return verNum;
	}

private:
	int m_major;
	int m_micro;
	int m_minor;
};
#endif

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
}
