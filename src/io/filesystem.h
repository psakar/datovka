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

#include <QByteArray>
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
 * @param[in] format           Format string, DEFAULT_TMP_FORMAT is used when
 *                             empty.
 * @param[in] dmId             Message id.
 * @param[in] dbId             Data box identifier.
 * @param[in] userName         User identifier (login).
 * @param[in] attachName       Attachment file name.
 * @param[in] dmDeliveryTime   Message delivery time.
 * @param[in] dmAcceptanceTime Message acceptance time, current time when
 *                             invalid.
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

/*!
 * @brief Identifies the cause why a file could not be written.
 */
enum WriteFileState {
	WF_SUCCESS = 0, /*!< File was successfully created and written. */
	WF_CANNOT_CREATE, /*!< File could not be created. */
	WF_CANNOT_READ, /*!< File could not be read. */
	WF_CANNOT_WRITE_WHOLE, /*!< File could not be entirely written. */
	WF_ERROR /*!< Different error. */
};

/*!
 * @brief Create file and write data to it.
 *
 * @param[in] filePath      File path.
 * @param[in] data          Data to be written into file.
 * @param[in] deleteOnError Delete created file when cannot be entirely
 *                          written.
 * @return Status identifier.
 */
enum WriteFileState writeFile(const QString &filePath, const QByteArray &data,
    bool deleteOnError = false);

/*!
 * @brief Create and write data to temporary file.
 *
 * @param[in] fileName      File name.
 * @param[in] data          Data to be written into file.
 * @param[in] deleteOnError Delete created file when it cannot be entirely
 *                          written.
 * @return Full path to written file on success, empty string on failure.
 */
QString writeTemporaryFile(const QString &fileName, const QByteArray &data,
    bool deleteOnError = false);

/*!
 * @brief Returns path to configuration directory.
 *
 * @param[in] confSubdir Configuration directory name (e.g. ".dsgui").
 * @return Full path to configuration directory.
 */
QString confDirPath(const QString &confSubdir);

/*!
 * @brief Changes all occurrences of '\' to '/' in given configuration file.
 *
 * @param[in] filePath Path to file.
 * @return Status identifier.
 */
enum WriteFileState confFileFixBackSlashes(const QString &filePath);

/*!
 * @brief Fix account password format = compatibility with Datovka 3.
 *
 * @note Removes double quotes ('"') from account password string.
 *
 * @param[in] filePath Path to file.
 * @return Status identifier.
 */
enum WriteFileState confFileRemovePwdQuotes(const QString &filePath);

/*!
 * @brief Returns the path to directory where application localisation resides.
 *
 * @return Full directory path.
 */
QString appLocalisationDir(void);

/*!
 * @brief Returns the path to directory where Qt localisation resides.
 *
 * @return Full directory path.
 */
QString qtLocalisationDir(void);

/*!
 * @brief Text files supplied with the application.
 */
enum TextFile {
	TEXT_FILE_CREDITS = 1, /*!< Credits file. */
	TEXT_FILE_LICENCE /*!< License file. */
};

/*!
 * @brief Returns the content of the supplied text file.
 */
QString suppliedTextFileContent(enum TextFile textFile);

#endif /* _FILESYSTEM_H_ */
