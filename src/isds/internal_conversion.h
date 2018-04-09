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

#pragma once

#include <cstdbool>
#include <QDate>
#include <QString>

#include "src/isds/types.h"

namespace Isds {

	/*!
	 * @brief Converts a C-string into QString.
	 *
	 * @param[in] cStr C string.
	 * @return String object, null string if NULL pointer was supplied.
	 */
	QString fromCStr(const char *cStr);

	/*!
	 * @brief Creates a C-string copy to the supplied QString.
	 *
	 * @param[in,out] cStrPtr Pointer to C string.
	 * @param[in] str String object.
	 */
	void toCStrCopy(char **cStrPtr, const QString &str);

	/*!
	 * @brief Converts date from struct tm.
	 *
	 * @param[in] cDate Struct tm containing date.
	 * @return Date object, null date if NULL pointer was supplied.
	 */
	QDate dateFromStructTM(struct tm *cDate);

	/*!
	 * @brief Creates a struct tm copy of the supplied QDate.
	 *
	 * @param[in,out] cDatePtr Address of pointer to struct tm.
	 * @param[in] date Date object.
	 */
	void toCDateCopy(struct tm **cDatePtr, const QDate &date);

	/*!
	 * @brief Creates a long int from supplied number.
	 *
	 * @param[in,out] cLongPtr Pointer to long int.
	 * @param[in] i Integerer.
	 */
	void toLongInt(long int **cLongPtr, qint64 i);

	/*!
	 * @brief Converts internal pointer to bool.
	 *
	 * @param[in] cBoolPtr Pointer to bool.
	 * @return Nullable bool enumeration type.
	 */
	enum Type::NilBool fromBool(const _Bool *cBoolPtr);

	/*!
	 * @brief Sets bool pointer according to supplied value.
	 *
	 * @param[in,out] cBoolPtr Bool pointer to be set.
	 * @param[in] nilBool Nullable bool value.
	 */
	void toBool(_Bool **cBoolPtr, enum Type::NilBool nilBool);
}
