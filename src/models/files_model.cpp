/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QMimeData>
#include <QMimeDatabase>
#include <QSqlRecord>

#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/models/files_model.h"

#define LOCAL_DATABASE_STR QLatin1String("local database")

DbFlsTblModel::DbFlsTblModel(QObject *parent)
    : TblModel(parent)
{
	/* Fixed column count. */
	m_columnCount = MAX_COL;
}

QVariant DbFlsTblModel::data(const QModelIndex &index, int role) const
{
	if ((Qt::DisplayRole == role) && (FSIZE_COL == index.column())) {
		/* Compute attachment size from base64 length. */
		QByteArray b64(_data(index.sibling(index.row(), CONTENT_COL),
		    role).toByteArray());
		return base64RealSize(b64);
	} else if ((Qt::DisplayRole == role) && (FPATH_COL == index.column())) {
		const QString path(_data(index).toString());
		return (path == LOCAL_DATABASE_STR) ? tr("local database") : path;
	} else {
		return _data(index, role);
	}
}

Qt::ItemFlags DbFlsTblModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = TblModel::flags(index);

	if (index.isValid()) {
		defaultFlags |= Qt::ItemIsDragEnabled;
	}

	return defaultFlags;
}

void DbFlsTblModel::setHeader(void)
{
	int i;

	for (i = 0; i < MessageDb::fileItemIds.size(); ++i) {
		/* Description. */
		setHeaderData(i, Qt::Horizontal,
		    flsTbl.attrProps.value(MessageDb::fileItemIds[i]).desc,
		    Qt::DisplayRole);
		/* Data type. */
		setHeaderData(i, Qt::Horizontal,
		    flsTbl.attrProps.value(MessageDb::fileItemIds[i]).type,
		    ROLE_MSGS_DB_ENTRY_TYPE);
	}

	/* Columns with missing names. */
	setHeaderData(FSIZE_COL, Qt::Horizontal,
	    QObject::tr("File size"), Qt::DisplayRole);
	setHeaderData(FPATH_COL, Qt::Horizontal,
	    QObject::tr("File path"), Qt::DisplayRole);
}

bool DbFlsTblModel::setMessage(const struct isds_message *message)
{
	if (NULL == message) {
		Q_ASSERT(0);
		return false;
	}

	m_data.clear();
	m_rowsAllocated = 0;
	m_rowCount = 0;

	/* m_columnCount = MAX_COL; */

	return addMessageData(message);
}

void DbFlsTblModel::setQuery(QSqlQuery &query)
{
	beginResetModel();
	m_data.clear();
	m_rowsAllocated = 0;
	m_rowCount = 0;
	endResetModel();

	/* Looks like empty results have column count set. */
	/* m_columnCount = MAX_COL; */
	if ((query.record().count() + 1) != m_columnCount) {
		Q_ASSERT(0);
		return;
	}

	appendQueryData(query);
}

bool DbFlsTblModel::appendQueryData(QSqlQuery &query)
{
	if ((query.record().count() + 1) != m_columnCount) {
		return false;
	}

	beginResetModel();

	query.first();
	while (query.isActive() && query.isValid()) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		queryToVector(row, query);
		row[FPATH_COL] = LOCAL_DATABASE_STR;

		m_data[m_rowCount++] = row;

		query.next();
	}

	endResetModel();

	return true;
}

/*!
 * @brief Read file content and encode it into base64.
 *
 * @param[in] filePath Path to file.
 * @return Base64-encoded file content.
 */
static
QByteArray getFileBase64(const QString &filePath)
 {
	QFile file(filePath);
	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			logErrorNL("Could not open file '%s'.",
			    filePath.toUtf8().constData());
			goto fail;
		}
		return file.readAll().toBase64();
	}
fail:
	return QByteArray();
}

int DbFlsTblModel::addAttachmentFile(const QString &filePath)
{
	QFile attFile(filePath);

	int fileSize = attFile.size();
	if (0 == fileSize) {
		logWarning("Ignoring file '%s' with zero size.\n",
		    filePath.toUtf8().constData());
		return 0;
	}

	QString fileName(QFileInfo(attFile.fileName()).fileName());
	QMimeType mimeType(QMimeDatabase().mimeTypeForFile(attFile));

	int rows = rowCount();

	for (int i = 0; i < rows; ++i) {
		if (_data(i, FPATH_COL).toString() == filePath) {
			/* Already in table. */
			logWarning("File '%s' already in table.\n",
			    filePath.toUtf8().constData());
			return -1;
		}
	}

	beginInsertRows(QModelIndex(), rows, rows);

	reserveSpace();

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
	rowVect[CONTENT_COL] = getFileBase64(filePath);
	rowVect[FNAME_COL] = fileName;
	rowVect[MIME_COL] = mimeType.name();
	rowVect[FSIZE_COL] = fileSize; // QString::number(fileSize)
	rowVect[FPATH_COL] = filePath;

	m_data[m_rowCount++] = rowVect;

	endInsertRows();

	return fileSize;
}

void DbFlsTblModel::addAttachmentEntry(const QByteArray &base64content,
    const QString &fName)
{
	if (base64content.isEmpty() || fName.isEmpty()) {
		return;
	}

	int rows = rowCount();
	beginInsertRows(QModelIndex(), rows, rows);

	reserveSpace();

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
	rowVect[CONTENT_COL] = base64content;
	rowVect[FNAME_COL] = fName;
	rowVect[MIME_COL] = tr("unknown");
	rowVect[FSIZE_COL] = base64RealSize(base64content); // QString::number()
	rowVect[FPATH_COL] = LOCAL_DATABASE_STR;

	m_data[m_rowCount++] = rowVect;

	endInsertRows();
}

bool DbFlsTblModel::addMessageData(const struct isds_message *message)
{
	const struct isds_list *docListItem;
	const struct isds_document *doc;

	if (NULL == message) {
		Q_ASSERT(0);
		return false;
	}

	beginResetModel();

	docListItem = message->documents;
	if (NULL == docListItem) {
		logWarning("%s\n", "Message has no documents.");
	}
	while (NULL != docListItem) {
		doc = (struct isds_document *) docListItem->data;
		if (NULL == doc) {
			Q_ASSERT(0);
			endResetModel();
			return false;
		}

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CONTENT_COL] = QVariant(
		    QByteArray::fromRawData((char *) doc->data,
		        (int) doc->data_length).toBase64());
		row[FNAME_COL] = QVariant(QString(doc->dmFileDescr));
		row[FSIZE_COL] = QVariant(0);

		m_data[m_rowCount++] = row;

		docListItem = docListItem->next;
	}

	endResetModel();

	return true;
}
