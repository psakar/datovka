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

#include <QByteArray>
#include <QObject>
#include <QVariant>

#include "src/datovka_shared/isds/message_interface.h"
#include "src/io/message_db.h"
#include "src/models/table_model.h"

/*!
 * @brief Custom attachment table model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 * It is also used for attachment content caching.
 */
class AttachmentTblModel : public TblModel {
	Q_OBJECT

public:
	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		ATTACHID_COL = 0, /* Attachment identifier (as used in local database). Not used for any purpose. */
		MSGID_COL = 1, /* Message identifier. */
		BINARY_CONTENT_COL = 2, /* Raw (non-base64-encoded) attachment content. */
		FNAME_COL = 3, /* Attachment file name. */
		MIME_COL = 4, /* Mime type description. */
		BINARY_SIZE_COL = 5, /* Attachment binary size. */
		FPATH_COL = 6, /* Path to origin. */
		MAX_COL = 7 /* Number of columns. */
	};

	/*!
	 * @brief Error values when inserting file into model.
	 */
	enum InsertError {
		OTHER_ERROR = -4, /*!< Generic error. */
		FILE_NOT_EXISTENT = -3, /*!< File does nto exist. */
		FILE_NOT_READABLE = -2, /*!< File cannot be read. */
		FILE_ALREADY_PRESENT = -1, /*!< File is already held in the model. */
		FILE_ZERO_SIZE = 0 /*!< File has zero size (i.e. is empty). */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] highlightUnlistedSuff If true then files with unsupported
	 *                                  suffixes will be highlighted.
	 * @param[in] parent Parent object.
	 */
	explicit AttachmentTblModel(bool highlightUnlistedSuff = false,
	    QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Returns the data stored under the given role.
	 *
	 * @param[in] index Position.
	 * @param[in] role  Role if the position.
	 * @return Data or invalid QVariant if no matching data found.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the drop actions supported by this model.
	 *
	 * @return Supported drop actions.
	 */
	virtual
	Qt::DropActions supportedDropActions(void) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Used to set items draggable.
	 *
	 * @param[in] index Index which to obtain flags for.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns the list of allowed MIME types.
	 *
	 * @return List of MIME types.
	 */
	virtual
	QStringList mimeTypes(void) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns object containing serialised attachment data.
	 *
	 * @param[in] indexes List of indexes.
	 * @return Pointer to newly allocated mime data object, Q_NULLPTR on error.
	 */
	virtual
	QMimeData *mimeData(
	    const QModelIndexList &indexes) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns whether the model accepts drops of given mime data.
	 *
	 * @param[in] data Data to be dropped.
	 * @param[in] action Type of drop action.
	 * @param[in] row Target row.
	 * @param[in] column Target column.
	 * @param[in] parent Parent index.
	 * @return True if drop is accepted.
	 */
	virtual
	bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
	    int row, int column,
	    const QModelIndex &parent) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Handles data supplied by drop operation.
	 *
	 * @param[in] data Data to be dropped.
	 * @param[in] action Type of drop action.
	 * @param[in] row Target row.
	 * @param[in] column Target column.
	 * @param[in] parent Parent index.
	 * @return True if data are handled by the model.
	 */
	virtual
	bool dropMimeData(const QMimeData *data, Qt::DropAction action,
	    int row, int column,
	    const QModelIndex &parent) Q_DECL_OVERRIDE;

	/*!
	 * @brief Sets default header.
	 */
	void setHeader(void);

	/*!
	 * @brief Appends data into the model.
	 *
	 * @brief entryList List of entries to append into the model.
	 */
	void appendData(const QList<MessageDb::AttachmentEntry> &entryList);

	/*!
	 * @brief Sets the content of the model according to the supplied
	 *     message.
	 *
	 * @param[in] message Message.
	 * @return True on success.
	 */
	bool setMessage(const Isds::Message &message);

	/*!
	 * @brief Adds attachment file.
	 *
	 * @param[in] filePath Path to attachment file.
	 * @param[in] row Row to insert the data into.
	 * @return Positive size of added file, error code otherwise (enum InsertError).
	 */
	int insertAttachmentFile(const QString &filePath, int row);

	/*!
	 * @brief Append attachment data line.
	 * @param[in] binaryContent Raw (non-base64-encoded) attachment content.
	 * @param[in] fName Attachment name.
	 * @param True when attachment data successfully added.
	 */
	bool appendBinaryAttachment(const QByteArray &binaryContent,
	    const QString &fName);

	/*!
	 * @brief Computes size of files held by the model.
	 *
	 * @returns Total attachment data size.
	 */
	qint64 totalAttachmentSize(void) const;

	/*!
	 * @brief Generate sorted list containing only one index per line each.
	 *
	 * @param[in] indexes Indexes identifying lines.
	 * @return List of indexes with unique row numbers.
	 */
	static
	QModelIndexList sortedUniqueLineIndexes(const QModelIndexList &indexes,
	    int dfltCoumn);

private:
	/*!
	 * @brief Determine MIME string.
	 *
	 * @param[in] fileName File name.
	 * @param[in] data Binary content.
	 * @return MIME name or description that it is unknown.
	 */
	static
	QString mimeStr(const QString &fileName, const QByteArray &data);

	/*!
	 * @brief Appends data from the supplied message.
	 *
	 * @param[in] message Message.
	 * @return True when message data successfully added.
	 */
	bool appendMessageData(const Isds::Message &message);

	/*!
	 * @brief Insert supplied vector.
	 *
	 * @param[in] rowVect Vector containing a model row.
	 * @param[in] row Row to insert the data into.
	 * @param[in] insertUnique true if only unique file should be added.
	 * @return True when attachment data successfully added.
	 */
	bool insertVector(const QVector<QVariant> &rowVect, int row,
	    bool insertUnique);

	/*!
	 * @brief Check whether file name and content combination already
	 *     exists.
	 *
	 * @param[in] binaryContent Raw (non-base64-encoded) attachment content.
	 * @param[in] fName Attachment name.
	 * @param[in] fPath File path.
	 * @return True if content with name exists in model.
	 */
	bool nameAndContentPresent(const QVariant &binaryContent,
	    const QVariant &fName, const QVariant &fPath) const;

	/*!
	 * @brief Returns list of file paths for lines given in indexes.
	 *
	 * @note FIles are created if no files are held within the model.
	 *
	 * @param[in] tmpDirPath Temporary directory path.
	 * @param[in] indexes Indexes identifying the attachment entries.
	 * @return List of file paths, empty path list on error.
	 */
	QStringList accessibleFiles(const QString &tmpDirPath,
	    const QModelIndexList &indexes) const;

	const bool m_highlightUnlistedSuff; /*!< Highlight entries with unsupported file suffix. */
};
