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

#ifndef _ABOUT_H_
#define _ABOUT_H_

#include <QStringList>

/*!
 * @brief Obtain list of strings containing libraries which the application
 *     depends on.
 *
 * @return List of strings containing library descriptions.
 */
QStringList libraryDependencies(void);

/*!
 * @brief Compare newest available version and application version.
 *
 * @param[in] vStr1 Version string.
 * @param[in] vStr2 Version string.
 * @retval -2 if vStr1 does not contain a suitable version string
 * @retval -1 if vStr1 is less than vStr2
 * @retval  0 if vStr1 is equal to vStr2
 * @retval  1 if vStr1 is greater than vStr2
 * @retval  2 if vStr2 does not contain a suitable version string
 */
int compareVersionStrings(const QString &vStr1, const QString &vStr2);

#endif /* _ABOUT_H_ */
