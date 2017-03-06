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

#ifndef _FILES_MODEL_H_
#define _FILES_MODEL_H_

#include <QByteArray>
#include <QObject>
#include <QVariant>

#include "src/models/table_model.h"

/*!
 * @brief Custom file model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 * It is also used for attachment content caching.
 */
class DbFlsTblModel : public TblModel {
    Q_OBJECT

public:
	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		ATTACHID_COL = 0, /* Attachment identifier. */
		MSGID_COL = 1, /* Message identifier. */
		CONTENT_COL = 2, /* Base64-encoded attachment content. */
		FNAME_COL = 3, /* Attachment file name. */
		MIME_COL = 4, /* Mime type description. */
		FSIZE_COL = 5, /* Attachment file size (base64-decoded). */
		FPATH_COL = 6, /* Path to origin. */
		MAX_COL = 7 /* Number of columns. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit DbFlsTblModel(QObject *parent = Q_NULLPTR);

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
	 * @return Pointer to newly allocated mime data object, 0 on error.
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
	 * @brief Sets the content of the model according to the supplied
	 *     message.
	 *
	 * @param[in] message Message structure.
	 * @return True on success.
	 */
	bool setMessage(const struct isds_message *message);

	/*!
	 * @brief Sets the content of the model according to the supplied query.
	 *
	 * @param[in,out] qyery SQL query result.
	 */
	virtual
	void setQuery(QSqlQuery &query) Q_DECL_OVERRIDE;

	/*!
	 * @brief Appends data from the supplied query to the model.
	 *
	 * @param[in,out] query SQL query result.
	 * @return True on success.
	 */
	virtual
	bool appendQueryData(QSqlQuery &query) Q_DECL_OVERRIDE;

	/*!
	 * @brief Adds attachment file.
	 *
	 * @param[in] filePath Path to attachment file.
	 * @param[in] row Row to insert the data into.
	 * @return Positive size of added file, 0 or -1 on error.
	 */
	int insertAttachmentFile(const QString &filePath, int row);

	/*!
	 * @brief Append attachment data line.
	 *
	 * @param[in] base64content Base64-encoded attachment content.
	 * @param[in] fName Attachment name.
	 * @param True when attachment data successfully added.
	 */
	bool appendAttachmentEntry(const QByteArray &base64content,
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
	 * @brief Appends data from the supplied message.
	 *
	 * @param[in] message Message structure.
	 * @return True when message data successfully added.
	 */
	bool appendMessageData(const struct isds_message *message);

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
	 * @param[in] base64content Base64-encoded attachment content.
	 * @param[in] fName Attachment name.
	 * @param[in] fPath File path.
	 * @return True if content with name exists in model.
	 */
	bool nameAndContentPresent(const QVariant &base64content,
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
};

#endif /* _FILES_MODEL_H_ */
