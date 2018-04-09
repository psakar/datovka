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

#include "src/isds/internal_conversion.h"

QString Isds::fromCStr(const char *cStr)
{
	if (cStr == NULL) {
		return QString();
	}
	return QString(cStr);
}

void Isds::toCStrCopy(char **cStrPtr, const QString &str)
{
	if (Q_UNLIKELY(cStrPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*cStrPtr != NULL) {
		/* Delete already allocated. */
		std::free(*cStrPtr); *cStrPtr = NULL;
	}
	if (str.isNull()) {
		return;
	}
	/* Copy string content. */
	const char *utfStr = str.toUtf8().constData();
	std::size_t utfStrLen = std::strlen(utfStr);
	if (utfStr == Q_NULLPTR) {
		return;
	}
	*cStrPtr = (char *)std::malloc(utfStrLen + 1);
	if (Q_UNLIKELY(*cStrPtr == NULL)) {
		Q_ASSERT(0);
		return;
	}
	std::memcpy(*cStrPtr, utfStr, utfStrLen + 1);
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

void Isds::toLongInt(long int **cLongPtr, qint64 i)
{
	if (Q_UNLIKELY(cLongPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*cLongPtr == NULL) {
		*cLongPtr = (long int*)std::malloc(sizeof(**cLongPtr));
		if (Q_UNLIKELY(*cLongPtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}

	**cLongPtr = i;
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

void Isds::toBool(_Bool **cBoolPtr, enum Type::NilBool nilBool)
{
	if (Q_UNLIKELY(cBoolPtr == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (nilBool == Type::BOOL_NULL) {
		if (*cBoolPtr != NULL) {
			std::free(*cBoolPtr); *cBoolPtr = NULL;
		}
		return;
	}
	if (*cBoolPtr == NULL) {
		*cBoolPtr = (_Bool *)std::malloc(sizeof(**cBoolPtr));
		if (Q_UNLIKELY(*cBoolPtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}

	**cBoolPtr = (nilBool == Type::BOOL_TRUE);
}
