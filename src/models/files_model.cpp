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

#include <QDir>
#include <QMimeData>
#include <QMimeDatabase>
#include <QSet>
#include <QTemporaryDir>
#include <QSqlRecord>
#include <QUrl>

#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/filesystem.h"
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

Qt::DropActions DbFlsTblModel::DbFlsTblModel::supportedDropActions(void) const
{
	/* The model must provide removeRows() to be able to use move action. */
	return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags DbFlsTblModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = TblModel::flags(index);

	if (index.isValid()) {
		/* Don't allow drops on items. */
		defaultFlags |= Qt::ItemIsDragEnabled; // | Qt::ItemIsDropEnabled;
	} else {
		defaultFlags |= Qt::ItemIsDropEnabled;
	}

	return defaultFlags;
}

QStringList DbFlsTblModel::mimeTypes(void) const
{
	return QStringList(QStringLiteral("text/uri-list"));
}

/*!
 * @brief Generate list containing only one index per line each.
 *
 * @param[in] indexes Indexes identifying lines.
 * @return List of indexes with unique row numbers.
 */
static
const QModelIndexList uniqueLineIndexes(const QModelIndexList &indexes,
    int dfltCoumn)
{
	QSet<int> lines;
	QModelIndexList uniqueLines;

	foreach (const QModelIndex &index, indexes) {
		if (lines.contains(index.row())) {
			continue;
		}
		uniqueLines.append(index.sibling(index.row(), dfltCoumn));
		lines.insert(index.row());
	}

	return uniqueLines;
}

/*!
 * @brief Creates temporary files related to selected view items.
 *
 * @param[in] tmpDir Temporary directory object.
 * @param[in] indexes Indexes identifying lines.
 * @return List of absolute file names or empty list on error.
 */
static
QStringList temporaryFiles(const QTemporaryDir &tmpDir,
    const QModelIndexList &indexes)
{
	QStringList tmpFileList;
	int fileNumber = 0;
	const QString tmpPath(tmpDir.path());

	if (tmpPath.isEmpty()) {
		Q_ASSERT(0);
		return QList<QString>();
	}

	foreach (const QModelIndex &idx, indexes) {
		QString subirPath(tmpPath + QDir::separator() +
		    QString::number(fileNumber++));
		{
			/*
			 * Create a separate subdirectory because the files
			 * may have equal names.
			 */
			QDir dir(tmpPath);
			if (!dir.mkpath(subirPath)) {
				logError("Could not create directory '%s'.",
				    subirPath.toUtf8().constData());
				return QList<QString>();
			}
		}
		QString attachAbsPath(subirPath + QDir::separator());
		{
			/* Determine full file path. */
			QModelIndex fileNameIndex = idx.sibling(idx.row(),
			    DbFlsTblModel::FNAME_COL);
			if(!fileNameIndex.isValid()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			QString attachFileName(fileNameIndex.data().toString());
			if (attachFileName.isEmpty()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			attachAbsPath += attachFileName;
		}
		QByteArray attachData;
		{
			/* Obtain data. */
			QModelIndex dataIndex = idx.sibling(idx.row(),
			    DbFlsTblModel::CONTENT_COL);
			if (!dataIndex.isValid()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
			attachData = QByteArray::fromBase64(
			    dataIndex.data().toByteArray());
			if (attachData.isEmpty()) {
				Q_ASSERT(0);
				return QList<QString>();
			}
		}
		if (WF_SUCCESS != writeFile(attachAbsPath, attachData, false)) {
			return QList<QString>();
		}

		tmpFileList.append(attachAbsPath);
	}

	return tmpFileList;
}

/*!
 * @brief Converts list of absolute file names to list of URLs.
 *
 * @param[in] tmpFileNames List of absolute file names.
 * @return List of URLs or empty list on error.
 */
static
QList<QUrl> temporaryFileUrls(const QStringList &tmpFileNames)
{
	QList<QUrl> uriList;

	foreach (const QString &fileName, tmpFileNames) {
		uriList.append(QUrl::fromLocalFile(fileName));
	}

	return uriList;
}

QMimeData *DbFlsTblModel::mimeData(const QModelIndexList &indexes) const
{
	QModelIndexList lineIndexes = uniqueLineIndexes(indexes, FNAME_COL);
	if (lineIndexes.isEmpty()) {
		return 0;
	}

	/* Create temporary directory. Automatic remove is on by default. */
	QTemporaryDir dir(QDir::tempPath() + QDir::separator() + TMP_DIR_NAME);
	if (!dir.isValid()) {
		logErrorNL("%s", "Could not create a temporary directory.");
		return 0;
	}
	/*
	 * Automatic removal cannot be set because his removes the files
	 * before actual copying of the file.
	 *
	 * TODO -- Represent the copied data in a different way than an URL.
	 */
	dir.setAutoRemove(false);

	QStringList tmpFileNames(temporaryFiles(dir, lineIndexes));
	if (tmpFileNames.isEmpty()) {
		logErrorNL("%s", "Could not write temporary files.");
		return 0;
	}

	QMimeData *mimeData = new (std::nothrow) QMimeData;
	if (0 == mimeData) {
		return 0;
	}
	QList<QUrl> urlList = temporaryFileUrls(tmpFileNames);
	mimeData->setUrls(urlList);

	return mimeData;
}

bool DbFlsTblModel::canDropMimeData(const QMimeData *data,
    Qt::DropAction action, int row, int column,
    const QModelIndex &parent) const
{
	Q_UNUSED(action);
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	if (0 == data) {
		return false;
	}

	return data->hasUrls();
}

/*!
 * @brief Convert a list of URLs to a list of absolute file paths.
 *
 * @param[in] uriList List of file URLs.
 * @return List of absolute file paths or empty list on error.
 */
static
QStringList filePaths(const QList<QUrl> &uriList)
{
	QStringList filePaths;

	foreach (const QUrl &uri, uriList) {
		if (!uri.isValid()) {
			logErrorNL("Dropped invalid URL '%s'.",
			    uri.toString().toUtf8().constData());
			return QList<QString>();
		}

		if (!uri.isLocalFile()) {
			logErrorNL("Dropped URL '%s' is not a local file.",
			    uri.toString().toUtf8().constData());
			return QList<QString>();
		}

		filePaths.append(uri.toLocalFile());
	}

	return filePaths;
}

bool DbFlsTblModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
    int row, int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent)) {
		return false;
	}

	/* data->hasUrls() already tested */

	QStringList paths(filePaths(data->urls()));

	if (parent.isValid()) {
		/* Non-root node; append data. */
		row = rowCount();
	}

	foreach (const QString &filePath, paths) {
		insertAttachmentFile(filePath, row);
		row++; /* Move to next row. */
	}

	return true;
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

	beginResetModel();
	m_data.clear();
	m_rowsAllocated = 0;
	m_rowCount = 0;
	endResetModel();

	/* m_columnCount = MAX_COL; */

	return appendMessageData(message);
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

	query.first();
	while (query.isActive() && query.isValid()) {

		QVector<QVariant> row(m_columnCount);

		queryToVector(row, query);
		row[FPATH_COL] = LOCAL_DATABASE_STR;

		/* Don't check data duplicity! */
		insertVector(row, rowCount(), false);

		query.next();
	}

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

int DbFlsTblModel::insertAttachmentFile(const QString &filePath, int row)
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

	for (int i = 0; i < rowCount(); ++i) {
		if (_data(i, FPATH_COL).toString() == filePath) {
			/* Already in table. */
			logWarning("File '%s' already in table.\n",
			    filePath.toUtf8().constData());
			return -1;
		}
	}

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
	rowVect[CONTENT_COL] = getFileBase64(filePath);
	rowVect[FNAME_COL] = fileName;
	rowVect[MIME_COL] = mimeType.name();
	rowVect[FSIZE_COL] = fileSize; // QString::number(fileSize)
	rowVect[FPATH_COL] = filePath;

	/* Check data duplicity. */
	if (insertVector(rowVect, row, true)) {
		return fileSize;
	} else {
		return -1;
	}
}

bool DbFlsTblModel::appendAttachmentEntry(const QByteArray &base64content,
    const QString &fName)
{
	if (base64content.isEmpty() || fName.isEmpty()) {
		return false;
	}

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
	rowVect[CONTENT_COL] = base64content;
	rowVect[FNAME_COL] = fName;
	rowVect[MIME_COL] = tr("unknown");
	rowVect[FSIZE_COL] = base64RealSize(base64content); // QString::number()
	rowVect[FPATH_COL] = LOCAL_DATABASE_STR;

	/* Check data duplicity. */
	return insertVector(rowVect, rowCount(), true);
}

bool DbFlsTblModel::appendMessageData(const struct isds_message *message)
{
	const struct isds_list *docListItem;
	const struct isds_document *doc;

	if (NULL == message) {
		Q_ASSERT(0);
		return false;
	}

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

		QVector<QVariant> row(m_columnCount);

		row[CONTENT_COL] = QVariant(
		    QByteArray::fromRawData((char *) doc->data,
		        (int) doc->data_length).toBase64());
		row[FNAME_COL] = QVariant(QString(doc->dmFileDescr));
		row[FSIZE_COL] = QVariant(0);

		/* Don't check data duplicity! */
		insertVector(row, rowCount(), false);

		docListItem = docListItem->next;
	}

	return true;
}

bool DbFlsTblModel::insertVector(const QVector<QVariant> &rowVect,
    int row, bool insertUnique)
{
	if (rowVect.size() != m_columnCount) {
		return false;
	}

	if ((row < 0) || (row > rowCount())) {
		/*
		 * -1 is passed when dropping directly on index which may be
		 *  invalid for root node.
		 */
		row = rowCount();
	}

	if (insertUnique && nameAndContentPresent(rowVect.at(CONTENT_COL),
	        rowVect.at(FNAME_COL))) {
		/* Fail if data present. */
		logWarningNL("Same data '%s' already attached.",
		    rowVect.at(FNAME_COL).toString().toUtf8().constData());
		return false;
	}

	Q_ASSERT((0 <= row) && (row <= rowCount()));

	beginInsertRows(QModelIndex(), row, row);

	reserveSpace();

	if (row == rowCount()) {
		m_data[m_rowCount++] = rowVect;
	} else {
		m_data.insert(row, rowVect);
		m_rowCount++;
	}

	endResetModel();

	return true;
}

bool DbFlsTblModel::nameAndContentPresent(const QVariant &base64content,
    const QVariant &fName) const
{
	/* Cannot use foreach (), because contains pre-allocated empty lines. */
	for (int row = 0; row < rowCount(); ++row) {
		const QVector<QVariant> &rowEntry = m_data.at(row);
		if ((rowEntry.at(FNAME_COL) == fName) &&
		    (rowEntry.at(CONTENT_COL) == base64content)) {
			return true;
		}
	}

	return false;
}
