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

#include <QDate>
#include <QString>

namespace Isds {

	/*!
	 * @brief Converts a C-string into QString.
	 *
	 * @param[in] cStr C string.
	 * @return String object, null string if NULL pointer was supplied.
	 */
	QString fromCStr(char *cStr);

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
	 * @brief Creates a struct tm copy if the supplied QDate.
	 *
	 * @param[in,out] cDatePtr Address of pointer to struct tm.
	 * @param[in] date Date object.
	 */
	void toCDateCopy(struct tm **cDatePtr, const QDate &date);
}
