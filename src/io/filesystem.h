/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <QDateTime>
#include <QString>

/* Default file name formats of saved/exported files. */
#define DEFAULT_TMP_FORMAT "%Y-%M-%D_%i_tmp"

/*!
 * @brief Create file name according to the format string.
 *
 * @note The format string knows these attributes:
 *     "%Y", "%M", "%D", "%h", "%m", "%i", "%s", "%S", "%d", "%u", "%f"
 *
 * @param[in] format           Format string, DEFAULT_TMP_FORMAT is used when empty.
 * @param[in] dmId             Message id.
 * @param[in] dbId             Data box identifier.
 * @param[in] userName         User identifier (login).
 * @param[in] attachName       Attachment file name.
 * @param[in] dmDeliveryTime   Message delivery time.
 * @param[in] dmAcceptanceTime Message acceptance time, current time when invalid.
 * @param[in] dmAnnotation     Message annotation.
 * @param[in] dmSender         Message sender.
 * @return File name.
 */
QString fileNameFromFormat(QString format, qint64 dmId, const QString &dbId,
    const QString &userName, const QString &attachName,
    const QDateTime &dmDeliveryTime, QDateTime dmAcceptanceTime,
    QString dmAnnotation, QString dmSender);

/*!
 * @brief Check if file with given path exists.
 *
 * @param[in] filePath Full file path.
 * @return Full path with non-conflicting file if file already exists,
 *     null string on error.
 */
QString nonconflictingFileName(QString filePath);

#endif /* _FILESYSTEM_H_ */
