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

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h> /* struct timeval */

#include "src/isds/internal_conversion.h"

QByteArray Isds::fromCData(const void *cData, size_t cSize)
{
	if ((cData == NULL) || (cSize == 0)) {
		return QByteArray();
	}
	return QByteArray((const char *)cData, cSize);
}

bool Isds::toCDataCopy(void **cDataPtr, size_t *cSize, const QByteArray &data)
{
	if (Q_UNLIKELY((cDataPtr == Q_NULLPTR) || (cSize == NULL))) {
		Q_ASSERT(0);
		return false;
	}
	if (*cDataPtr != NULL) {
		/* Delete already created. */
		std::free(*cDataPtr); *cDataPtr = NULL;
		*cSize = 0;
	}
	if (data.isNull() || (data.size() == 0)) {
		return true;
	}
	/* Copy data content.*/
	*cSize = data.size();
	*cDataPtr = std::malloc(*cSize);
	if (Q_UNLIKELY(*cDataPtr == NULL)) {
		Q_ASSERT(0);
		*cSize = 0;
		return false;
	}
	std::memcpy(*cDataPtr, data.constData(), *cSize);
	return true;
}

QString Isds::fromCStr(const char *cStr)
{
	if (cStr == NULL) {
		return QString();
	}
	return QString(cStr);
}

bool Isds::toCStrCopy(char **cStrPtr, const QString &str)
{
	if (Q_UNLIKELY(cStrPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (*cStrPtr != NULL) {
		/* Delete already allocated. */
		std::free(*cStrPtr); *cStrPtr = NULL;
	}
	if (str.isNull()) {
		return true;
	}
	/* Copy string content. */
	QByteArray strBytes(str.toUtf8()); /* Must not be deleted before memcpy. */
	const char *utfStr = strBytes.constData();
	size_t utfStrLen = std::strlen(utfStr);
	if (utfStr == Q_NULLPTR) {
		return true;
	}
	*cStrPtr = (char *)std::malloc(utfStrLen + 1);
	if (Q_UNLIKELY(*cStrPtr == NULL)) {
		Q_ASSERT(0);
		return false;
	}
	if (utfStrLen > 0) {
		std::memcpy(*cStrPtr, utfStr, utfStrLen);
	}
	(*cStrPtr)[utfStrLen] = '\0';
	return true;
}

QDate Isds::dateFromStructTM(struct tm *cDate)
{
	if (cDate == NULL) {
		return QDate();
	}

	int year = cDate->tm_year + 1900;
	int month = cDate->tm_mon + 1;
	int day = cDate->tm_mday;
	return QDate(year, month, day);
}

void Isds::toCDateCopy(struct tm **cDatePtr, const QDate &date)
{
	if (Q_UNLIKELY(cDatePtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (date.isNull()) {
		if (*cDatePtr != NULL) {
			/* Delete allocated. */
			std::free(*cDatePtr); *cDatePtr = NULL;
		}
		return;
	}
	if (*cDatePtr == NULL) {
		*cDatePtr = (struct tm *)std::malloc(sizeof(**cDatePtr));
		if (Q_UNLIKELY(*cDatePtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	std::memset(*cDatePtr, 0, sizeof(**cDatePtr));

	(*cDatePtr)->tm_year = date.year() - 1900;
	(*cDatePtr)->tm_mon = date.month() - 1;
	(*cDatePtr)->tm_mday = date.day();
}

QDateTime Isds::dateTimeFromStructTimeval(struct timeval *cDateTime)
{
	QDateTime timeStamp;

	if (cDateTime != NULL) {
		/* TODO -- handle microseconds. */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
		timeStamp.setSecsSinceEpoch(cDateTime->tv_sec);
#else /* < Qt-5.8 */
		timeStamp.setTime_t(cDateTime->tv_sec);
#endif /* >= Qt-5.8 */
	}

	return timeStamp;
}

bool Isds::toCDateTimeCopy(struct timeval **cDateTimePtr,
    const QDateTime &dateTime)
{
	if (Q_UNLIKELY(cDateTimePtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (dateTime.isNull()) {
		if (*cDateTimePtr != NULL) {
			/* Delete allocated. */
			std::free(*cDateTimePtr); *cDateTimePtr = NULL;
		}
		return true;
	}
	if (*cDateTimePtr == NULL) {
		*cDateTimePtr =
		    (struct timeval *)std::malloc(sizeof(**cDateTimePtr));
		if (Q_UNLIKELY(*cDateTimePtr == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	std::memset(*cDateTimePtr, 0, sizeof(**cDateTimePtr));

	/* TODO -- handle microseconds. */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	(*cDateTimePtr)->tv_sec = dateTime.toSecsSinceEpoch();
#else /* < Qt-5.8 */
	(*cDateTimePtr)->tv_sec = dateTime.toTime_t();
#endif /* >= Qt-5.8 */
	return true;
}

qint64 Isds::fromLongInt(const long int *cLongPtr)
{
	if (cLongPtr == NULL) {
		return -1;
	}
	return *cLongPtr;
}

bool Isds::toLongInt(long int **cLongPtr, qint64 i)
{
	if (Q_UNLIKELY(cLongPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (i < 0) {
		if (*cLongPtr != NULL) {
			std::free(*cLongPtr); *cLongPtr = NULL;
		}
		return true;
	}
	if (*cLongPtr == NULL) {
		*cLongPtr = (long int*)std::malloc(sizeof(**cLongPtr));
		if (Q_UNLIKELY(*cLongPtr == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}

	**cLongPtr = i;
	return true;
}

enum Isds::Type::NilBool Isds::fromBool(const _Bool *cBoolPtr)
{
	if (cBoolPtr == NULL) {
		return Type::BOOL_NULL;
	} else if (*cBoolPtr == false) {
		return Type::BOOL_FALSE;
	} else {
		return Type::BOOL_TRUE;
	}
}

bool Isds::toBool(_Bool **cBoolPtr, enum Type::NilBool nilBool)
{
	if (Q_UNLIKELY(cBoolPtr == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	if (nilBool == Type::BOOL_NULL) {
		if (*cBoolPtr != NULL) {
			std::free(*cBoolPtr); *cBoolPtr = NULL;
		}
		return true;
	}
	if (*cBoolPtr == NULL) {
		*cBoolPtr = (_Bool *)std::malloc(sizeof(**cBoolPtr));
		if (Q_UNLIKELY(*cBoolPtr == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}

	**cBoolPtr = (nilBool == Type::BOOL_TRUE);
	return true;
}

qint64 Isds::string2NonNegativeLong(const QString &str, bool *ok)
{
	if (str.isEmpty()) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return -1;
	}

	bool iOk = false;
	qint64 num = str.toLongLong(&iOk);
	if ((!iOk) || (num < 0)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return -1;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return num;
}

QString Isds::nonNegativeLong2String(qint64 num)
{
	return (num >= 0) ? QString::number(num) : QString();
}
