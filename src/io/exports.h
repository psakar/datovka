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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"

/*!
 * @brief Provides exports from local database to disk.
 */
class Exports {
	Q_DECLARE_TR_FUNCTIONS(Exports)

public:
	/*!
	 * @brief Defines type of exported file.
	 */
	enum ExportFileType {
		ZFO_MESSAGE = 0,
		ZFO_DELIVERY,
		ZFO_DELIV_ATTACH,
		PDF_ENVELOPE,
		PDF_DELIVERY,
		PDF_DELIV_ATTACH
	};

	/*!
	 * @brief Export result codes.
	 */
	enum ExportError {
		EXP_SUCCESS = 0,
		EXP_CANCELED,
		EXP_DB_ERROR,
		EXP_NOT_MSG_DATA,
		EXP_WRITE_FILE_ERROR,
		EXP_ERROR
	};

	/*!
	 * @brief Generates file path containing attachment name according
	 *     to set format.
	 *
	 * @param[in] dbSet Account database set.
	 * @param[in] targetPath Path to target folder for export.
	 * @param[in] attachName Attachment file name.
	 * @param[in] userName Account username.
	 * @param[in] accountName Account name.
	 * @param[in] dbId Data box ID for export.
	 * @param[in] msgId Message ID for export.
	 * @param[in] prohibitDirSep True if directory separators should be
	 *                           prohibited when generating file name.
	 * @return New target path with file name for saving.
	 */
	static
	QString attachmentSavePathWithFileName(const MessageDbSet &dbSet,
	    const QString &targetPath, const QString &attachName,
	    const QString &dbId, const QString &userName,
	    const QString &accountName, const MessageDb::MsgId &msgId,
	    bool prohibitDirSep);

	/*!
	 * @brief Export message data as ZFO/PDF file.
	 *
	 * @param[in,out] parent Parent widget to call object from.
	 * @param[in]     dbSet Account database set.
	 * @param[in]     fileType Type of export file.
	 * @param[in]     targetPath Path to target folder for export.
	 * @param[in]     attachFileName Attachment file name.
	 * @param[in]     userName Account username.
	 * @param[in]     accountName Account name.
	 * @param[in]     dbId Data-box ID for export.
	 * @param[in]     msgId Message ID for export.
	 * @param[in]     askLocation Ask to new location for export.
	 * @param[out]    lastPath Last export path.
	 * @param[out]    errTxt Error text.
	 * @return Result operation code.
	 */
	static
	enum ExportError exportAs(QWidget *parent, const MessageDbSet &dbSet,
	    enum ExportFileType fileType, const QString &targetPath,
	    const QString &attachFileName, const QString &userName,
	    const QString &accountName, const QString &dbId,
	    const MessageDb::MsgId &msgId, bool askLocation,
	    QString &lastPath, QString &errStr);

	/*!
	 * @brief Export message envelope together with attachments.
	 *
	 * @param[in]  dbSet Account database set.
	 * @param[in]  targetPath Path to target folder for export.
	 * @param[in]  userName Account username.
	 * @param[in]  accountName Account name.
	 * @param[in]  dbId Data-box ID for export.
	 * @param[in]  msgId Message ID for export.
	 * @param[out] errTxt Error text.
	 * @return Result operation code.
	 */
	static
	enum ExportError exportEnvAndAttachments(const MessageDbSet &dbSet,
	    const QString &targetPath, const QString &userName,
	    const QString &accountName, const QString &dbId,
	    const MessageDb::MsgId &msgId, QString &errStr);

	/*!
	 * @brief Save message attachments and export ZFO/PDF files as well.
	 *
	 * @param[in]  dbSet Account database set.
	 * @param[in]  targetPath Path to target folder for export.
	 * @param[in]  userName Account username.
	 * @param[in]  accountName Account name.
	 * @param[in]  dbId Data-box ID for export.
	 * @param[in]  msgId Message ID for export.
	 * @param[out] errTxt Error text.
	 * @return Result operation code.
	 */
	static
	enum ExportError saveAttachmentsWithExports(const MessageDbSet &dbSet,
	    const QString &targetPath, const QString &userName,
	    const QString &accountName, const QString &dbId,
	    const MessageDb::MsgId &msgId, QString &errStr);

private:
	/*!
	 * @brief Private constructor.
	 *
	 * @note Just prevent any instances of this class.
	 */
	Exports(void);

	/*!
	 * @brief Print and write PDF file to disk.
	 *
	 * @param[in] fileName - Path and name of target file.
	 * @param[in] data     - File data.
	 * @return True on success, false else.
	 */
	static
	bool printPDF(const QString &fileName, const QString &data);

	/*!
	 * @brief Write ZFO file to disk.
	 *
	 * @param[in] fileName - Path and name of target file.
	 * @param[in] data     - File data.
	 * @return True on success, false else.
	 */
	static
	bool writeZFO(const QString &fileName, const QByteArray &data);
};
