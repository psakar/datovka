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

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>

#define TMP_ATTACHMENT_PREFIX "qdatovka_XXXXXX_"
#define TMP_DIR_NAME "qdatovka_dir_XXXXXX"

/* Default file name formats of saved/exported files. */
#define DEFAULT_TMP_FORMAT "%Y-%M-%D_%i_tmp"
#define DEFAULT_MESSAGE_FILENAME_FORMAT "DZ-%i"
#define DEFAULT_DELIVERY_FILENAME_FORMAT "DD-%i"
#define DEFAULT_ATTACHMENT_FILENAME_FORMAT "%f"
#define DEFAULT_DELIVERY_ATTACH_FORMAT "DD-%i-%f"

/*!
 * @brief Create partial file path according to the format string.
 *
 * @note The format string knows these attributes:
 *     "%Y", "%M", "%D", "%h", "%m", "%n", "%i", "%s", "%S", "%d", "%u", "%f"
 * @note Directory structure must be created explicitly.
 *
 * @param[in] format           Format string, DEFAULT_TMP_FORMAT is used when
 *                             empty. Directory separators may be used here.
 * @param[in] prohibitDirSep   True if directory separators should be prohibited.
 * @param[in] dmId             Message id.
 * @param[in] dbId             Data box identifier.
 * @param[in] userName         User identifier (login).
 * @param[in] accountName      Account name.
 * @param[in] attachName       Attachment file name.
 * @param[in] dmDeliveryTime   Message delivery time.
 * @param[in] dmAcceptanceTime Message acceptance time, current time when
 *                             invalid.
 * @param[in] dmAnnotation     Message annotation.
 * @param[in] dmSender         Message sender.
 * @return File name.
 */
QString fileSubpathFromFormat(QString format, bool prohibitDirSep, qint64 dmId,
    const QString &dbId, const QString &userName, QString accountName,
    const QString &attachName, const QDateTime &dmDeliveryTime,
    QDateTime dmAcceptanceTime, QString dmAnnotation, QString dmSender);

/*!
 * @brief Creates directory structure to store file into.
 *
 * @param[in] filePath File path. File need not be existent.
 * @return True if directory structure already exists or was successfully
 *     created, false on error.
 */
bool createDirStructureRecursive(const QString &filePath);

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
	TEXT_FILE_LICENCE = 1 /*!< License file. */
};

/*!
 * @brief Return default installation location of text file.
 *
 * @param[in] textFile Text file identifier.
 */
QString expectedTextFilePath(enum TextFile textFile);

/*!
 * @brief Returns the content of the supplied text file.
 *
 * @param[in] textFile Text file identifier.
 */
QString suppliedTextFileContent(enum TextFile textFile);
