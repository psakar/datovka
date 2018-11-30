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

#include <algorithm> /* std::sort */
#include <QColor>
#include <QDir>
#include <QMimeData>
#include <QMimeDatabase>
#include <QSet>
#include <QTemporaryDir>
#include <QSqlRecord>
#include <QUrl>

#include "src/datovka_shared/log/log.h"
#include "src/io/db_tables.h"
#include "src/io/filesystem.h"
#include "src/io/message_db.h"
#include "src/models/attachments_model.h"

#define LOCAL_DATABASE_STR QStringLiteral("local database")

AttachmentTblModel::AttachmentTblModel(bool highlightUnlistedSuff,
    QObject *parent)
    : TblModel(parent),
    m_highlightUnlistedSuff(highlightUnlistedSuff)
{
	/* Fixed column count. */
	m_columnCount = MAX_COL;
}

/*!
 * @brief Check whether file exists.
 *
 * @param[in] filePath Path to file.
 * @return True if file exists.
 */
static inline
bool fileExistent(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}
	return QFileInfo(filePath).exists();
}

/*!
 * @brief Check whether file exists and is readable.
 *
 * @param[in] filePath Path to file.
 * @return True if file is readable.
 */
static
bool fileReadable(const QString &filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}
	QFileInfo fileInfo(filePath);
	return fileInfo.exists() && fileInfo.isReadable();
}

/*!
 * @brief Read file size.
 *
 * @param[in] filePath Path to file.
 * @return File size or negative number if cannot determine.
 */
static
qint64 getFileSize(const QString &filePath)
 {
	QFileInfo fileInfo(filePath);
	if (fileInfo.exists() && fileInfo.isReadable()) {
		return fileInfo.size();
	}
	return -1;
}

/*!
 * @brief Read file content.
 *
 * @param[in] filePath Path to file.
 * @return Raw (non-base64-emcoded) file content.
 */
static
QByteArray getFileContent(const QString &filePath)
 {
	QFile file(filePath);
	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			logErrorNL("Could not open file '%s'.",
			    filePath.toUtf8().constData());
			goto fail;
		}
		return file.readAll();
	}
fail:
	return QByteArray();
}

/*!
 * @brief Return lower-case file suffix.
 */
#define fileSuffix(fileName) \
	QFileInfo(fileName).suffix().toLower()

/*!
 * @brief Check whether file name has allowed suffix.
 *
 * @param[in] fileName File name string.
 * @return True if file has suffix mentioned in Operation Rules of ISDS.
 */
static
bool fileHasAllowedSuffix(const QString &fileName)
{
	if (Q_UNLIKELY(fileName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	const QString suff(fileSuffix(fileName));
	if (suff.isEmpty()) {
		return false;
	}
	return Isds::Document::allowedFileSuffixes().contains(suff);
}

/*!
 * @brief Generate a description why the file does not meet the suffix
 *     restrictions.
 *
 * @param[in] fileName File name string.
 * @return Null string if suffix is allowed.
 */
static
QString suffixNotification(const QString &fileName)
{
	if (!fileHasAllowedSuffix(fileName)) {
		QString message(fileSuffix(fileName).isEmpty() ?
		    AttachmentTblModel::tr(
		        "The file name '%1' contains no suffix.").arg(fileName) :
		    AttachmentTblModel::tr(
		        "The suffix of file '%1' does not match the list of suffixes listed in the Operating Rules of ISDS.").arg(fileName));
		message += QStringLiteral("\n") +
		    AttachmentTblModel::tr(
		        "It may happen that the server will reject sending this message.");
		return message;
	}
	return QString();
}

QVariant AttachmentTblModel::data(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		switch (index.column()) {
		case BINARY_CONTENT_COL:
			{
				const QString fPath(_data(index.row(), FPATH_COL,
				    role).toString());

				if (fPath == LOCAL_DATABASE_STR) {
					/* Data should be in the model. */
					return _data(index, role);
				} else if (fileReadable(fPath)) {
					/* Read file. */
					return getFileContent(fPath);
				} else {
					/*
					 * This fallback is used when filling model
					 * with zfo content.
					 */
					return _data(index, role);
				}
			}
			break;
		case BINARY_SIZE_COL:
			{
				/* Read size from model if it is set. */
				qint64 binarySize = _data(index.row(),
				    BINARY_SIZE_COL, role).toLongLong();
				if (binarySize > 0) {
					return binarySize;
				}
			}
			{
				const QString fPath(_data(index.row(),
				    FPATH_COL, role).toString());
				const QByteArray rawData(_data(index.row(),
				    BINARY_CONTENT_COL, role).toByteArray());

				if (fPath == LOCAL_DATABASE_STR) {
					/* Get attachment size. */
					return rawData.size();
				} else if (fileReadable(fPath)) {
					/* File size. */
					return getFileSize(fPath);
				} else {
					/*
					 * This fallback is used when filling model
					 * with zfo content.
					 */
					return rawData.size();
				}
			}
			break;
		case FPATH_COL:
			{
				const QString fPath(_data(index).toString());
				return (fPath == LOCAL_DATABASE_STR) ?
				    tr("local database") : fPath;
			}
			break;
		default:
			return _data(index, role);
			break;
		}
		break;
	case Qt::ToolTipRole:
		if (m_highlightUnlistedSuff) {
			const QString notif(suffixNotification(
			    _data(index.row(), FNAME_COL, Qt::DisplayRole).toString()));
			if (!notif.isEmpty()) {
				return notif;
			}
		}
		return QVariant();
		break;
	case Qt::ForegroundRole:
		if (m_highlightUnlistedSuff &&
		    !fileHasAllowedSuffix(_data(index.row(), FNAME_COL, Qt::DisplayRole).toString())) {
			return QColor(Qt::darkRed);
		} else {
			return QVariant();
		}
		break;
	case Qt::AccessibleTextRole:
		switch (index.column()) {
		case FNAME_COL:
			{
				const QString fileName(data(index).toString());
				QString message(
				    headerData(index.column(), Qt::Horizontal).toString() +
				    QStringLiteral(" ") + fileName);
				if (m_highlightUnlistedSuff) {
					if (m_highlightUnlistedSuff) {
						const QString notif(suffixNotification(
						    _data(index.row(), FNAME_COL, Qt::DisplayRole).toString()));
						if (!notif.isEmpty()) {
							message += QStringLiteral(" ") + notif;
						}
					}
				}
				return message;
			}
			break;
		case MIME_COL:
		case FPATH_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QStringLiteral(" ") + data(index).toString();
			break;
		case BINARY_SIZE_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QStringLiteral(" ") + data(index).toString() +
			    QStringLiteral(" ") + tr("bytes");
			break;
		default:
			return QVariant();
			break;
		}
		break;
	case ROLE_PLAIN_DISPLAY:
		/* Explicitly asking associated file path. */
		if (index.column() == FPATH_COL) {
			const QString fPath(_data(index.row(), FPATH_COL,
			    Qt::DisplayRole).toString());
			const QByteArray binaryData(_data(index.row(),
			    BINARY_CONTENT_COL, Qt::DisplayRole).toByteArray());

			if ((fPath == LOCAL_DATABASE_STR) && !binaryData.isEmpty()) {
				return QVariant(); /* No file present. */
			} else if (fileReadable(fPath)) {
				return fPath; /* File present. */
			} else {
				return QVariant();
			}
		}
		return QVariant();
		break;
	default:
		return _data(index, role);
		break;
	}
}

Qt::DropActions AttachmentTblModel::AttachmentTblModel::supportedDropActions(
    void) const
{
	/* The model must provide removeRows() to be able to use move action. */
	return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags AttachmentTblModel::flags(const QModelIndex &index) const
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

QStringList AttachmentTblModel::mimeTypes(void) const
{
	return QStringList(QStringLiteral("text/uri-list"));
}

/*!
 * @brief Converts list of absolute file names to list of URLs.
 *
 * @param[in] fileNames List of absolute file names.
 * @return List of URLs or empty list on error.
 */
static
QList<QUrl> fileUrls(const QStringList &fileNames)
{
	QList<QUrl> uriList;

	foreach (const QString &fileName, fileNames) {
		uriList.append(QUrl::fromLocalFile(fileName));
	}

	return uriList;
}

QMimeData *AttachmentTblModel::mimeData(const QModelIndexList &indexes) const
{
	QModelIndexList lineIndexes = sortedUniqueLineIndexes(indexes,
	    FNAME_COL);
	if (lineIndexes.isEmpty()) {
		return Q_NULLPTR;
	}

	/* Create temporary directory. Automatic remove is on by default. */
	QTemporaryDir dir(QDir::tempPath() + QDir::separator() + TMP_DIR_NAME);
	if (!dir.isValid()) {
		logErrorNL("%s", "Could not create a temporary directory.");
		return Q_NULLPTR;
	}
	/*
	 * Automatic removal cannot be set because his removes the files
	 * before actual copying of the file.
	 *
	 * TODO -- Represent the copied data in a different way than an URL.
	 */
	dir.setAutoRemove(false);

	QStringList fileNames(accessibleFiles(dir.path(), lineIndexes));
	if (fileNames.isEmpty()) {
		logErrorNL("%s", "Could not write temporary files.");
		return Q_NULLPTR;
	}

	QMimeData *mimeData = new (std::nothrow) QMimeData;
	if (Q_NULLPTR == mimeData) {
		return Q_NULLPTR;
	}
	QList<QUrl> urlList = fileUrls(fileNames);
	mimeData->setUrls(urlList);

	return mimeData;
}

bool AttachmentTblModel::canDropMimeData(const QMimeData *data,
    Qt::DropAction action, int row, int column,
    const QModelIndex &parent) const
{
	Q_UNUSED(action);
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	if (Q_NULLPTR == data) {
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

bool AttachmentTblModel::dropMimeData(const QMimeData *data,
    Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent)) {
		return false;
	}

	/* data->hasUrls() tested in canDropMimeData() */
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

void AttachmentTblModel::setHeader(void)
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
	setHeaderData(BINARY_SIZE_COL, Qt::Horizontal, tr("File size"),
	    Qt::DisplayRole);
	setHeaderData(FPATH_COL, Qt::Horizontal, tr("File path"),
	    Qt::DisplayRole);
}

void AttachmentTblModel::appendData(
    const QList<MessageDb::AttachmentEntry> &entryList)
{
	if (Q_UNLIKELY(MessageDb::fileItemIds.size() != 6)) {
		Q_ASSERT(0);
		return;
	}

	m_columnCount = 6;

	if (entryList.isEmpty()) {
		/* Don't do anything. */
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const MessageDb::AttachmentEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[ATTACHID_COL] = entry.id;
		row[MSGID_COL] = entry.messageId;
		row[BINARY_CONTENT_COL] = entry.binaryContent;
		row[FNAME_COL] = entry.dmFileDescr;
		row[MIME_COL] = entry.dmMimeType;
		row[BINARY_SIZE_COL] = entry.binaryContent.size();

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

bool AttachmentTblModel::setMessage(const Isds::Message &message)
{
	if (Q_UNLIKELY(message.isNull())) {
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

/* File content will be held within model. */
//#define HOLD_FILE_CONTENT

int AttachmentTblModel::insertAttachmentFile(const QString &filePath, int row)
{
	if (!fileExistent(filePath)) {
		logWarningNL("Ignoring non-existent file '%s'.",
		    filePath.toUtf8().constData());
		return FILE_NOT_EXISTENT;
	} else if (!fileReadable(filePath)) {
		logWarningNL("Ignoring file '%s' which cannot be read.",
		    filePath.toUtf8().constData());
		return FILE_NOT_READABLE;
	}

	QFile attFile(filePath);

	int fileSize = attFile.size();
	if (0 == fileSize) {
		logWarningNL("Ignoring file '%s' with zero size.",
		    filePath.toUtf8().constData());
		return FILE_ZERO_SIZE;
	}

	const QString fileName(QFileInfo(attFile.fileName()).fileName());
	const QMimeType mimeType(QMimeDatabase().mimeTypeForFile(attFile));

	for (int i = 0; i < rowCount(); ++i) {
		if (_data(i, FPATH_COL).toString() == filePath) {
			/* Already in table. */
			logWarning("File '%s' already in table.\n",
			    filePath.toUtf8().constData());
			return FILE_ALREADY_PRESENT;
		}
	}

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
#if defined HOLD_FILE_CONTENT
	rowVect[BINARY_CONTENT_COL] = getFileContent(filePath);
#else /* !defined HOLD_FILE_CONTENT */
	rowVect[BINARY_CONTENT_COL] = QByteArray();
#endif /* defined HOLD_FILE_CONTENT */
	rowVect[FNAME_COL] = fileName;
	rowVect[MIME_COL] = mimeType.name();
#if defined HOLD_FILE_CONTENT
	rowVect[BINARY_SIZE_COL] = fileSize;
#else /* !defined HOLD_FILE_CONTENT */
	rowVect[BINARY_SIZE_COL] = 0;
#endif /* defined HOLD_FILE_CONTENT */
	rowVect[FPATH_COL] = filePath;

	/* Check data duplicity. */
	if (insertVector(rowVect, row, true)) {
		return fileSize;
	} else {
		return OTHER_ERROR;
	}
}

bool AttachmentTblModel::appendBinaryAttachment(const QByteArray &binaryContent,
    const QString &fName)
{
	if (binaryContent.isEmpty() || fName.isEmpty()) {
		return false;
	}

	QVector<QVariant> rowVect(m_columnCount);

	//rowVect[ATTACHID_COL] = QVariant();
	//rowVect[MSGID_COL] = QVariant();
	rowVect[BINARY_CONTENT_COL] = binaryContent;
	rowVect[FNAME_COL] = fName;
	rowVect[MIME_COL] = mimeStr(fName, binaryContent);
	rowVect[BINARY_SIZE_COL] = binaryContent.size();
	rowVect[FPATH_COL] = LOCAL_DATABASE_STR;

	/* Check data duplicity. */
	return insertVector(rowVect, rowCount(), true);
}

qint64 AttachmentTblModel::totalAttachmentSize(void) const
{
	qint64 sum = 0;

	for (int row = 0; row < rowCount(); ++row) {
		sum += data(index(row, BINARY_SIZE_COL),
		    Qt::DisplayRole).toLongLong();
	}

	return sum;
}

/*!
 * @brief Used for sorting index lists.
 */
class IndexRowLess {
public:
	bool operator()(const QModelIndex &a, const QModelIndex &b) const
	{
		return a < b;
	}
};

QModelIndexList AttachmentTblModel::sortedUniqueLineIndexes(
    const QModelIndexList &indexes, int dfltCoumn)
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

	::std::sort(uniqueLines.begin(), uniqueLines.end(), IndexRowLess());

	return uniqueLines;
}

QString AttachmentTblModel::mimeStr(const QString &fileName,
    const QByteArray &data)
{
	if (!fileName.isEmpty() && !data.isEmpty()) {
		const QMimeType mimeType(
		    QMimeDatabase().mimeTypeForFileNameAndData(fileName, data));
		if (mimeType.isValid()) {
			return mimeType.name();
		}
	}

	return tr("unknown");
}

bool AttachmentTblModel::appendMessageData(const Isds::Message &message)
{
	if (Q_UNLIKELY(message.isNull())) {
		Q_ASSERT(0);
		return false;
	}

	const QList<Isds::Document> &docList(message.documents());
	if (docList.isEmpty()) {
		logWarning("%s\n", "Message has no documents.");
	}
	foreach (const Isds::Document &doc, docList) {
		if (Q_UNLIKELY(doc.isNull())) {
			Q_ASSERT(0);
			return false;
		}

		QVector<QVariant> row(m_columnCount);

		row[BINARY_CONTENT_COL] = doc.binaryContent();
		row[FNAME_COL] = doc.fileDescr();
		row[MIME_COL] = mimeStr(doc.fileDescr(), doc.binaryContent());
		row[BINARY_SIZE_COL] = doc.binaryContent().size();

		/* Don't check data duplicity! */
		insertVector(row, rowCount(), false);
	}

	return true;
}

bool AttachmentTblModel::insertVector(const QVector<QVariant> &rowVect,
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

	if (insertUnique && nameAndContentPresent(rowVect.at(BINARY_CONTENT_COL),
	        rowVect.at(FNAME_COL), rowVect.at(FPATH_COL))) {
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

	endInsertRows();

	return true;
}

bool AttachmentTblModel::nameAndContentPresent(const QVariant &binaryContent,
    const QVariant &fName, const QVariant &fPath) const
{
	/* Cannot use foreach (), because contains pre-allocated empty lines. */
	for (int row = 0; row < rowCount(); ++row) {
		const QVector<QVariant> &rowEntry = m_data.at(row);
		if ((rowEntry.at(FNAME_COL) == fName) &&
		    (rowEntry.at(FPATH_COL) == fPath) &&
		    (rowEntry.at(BINARY_CONTENT_COL) == binaryContent)) {
			return true;
		}
	}

	return false;
}

/*!
 * @brief Creates temporary files related to selected view items.
 *
 * @param[in] tmpDirPath Temporary directory path.
 * @param[in] index Index identifying line.
 * @param[in] fileNumber Number identifying the file.
 * @return Absolute file name or empty string on error.
 */
static
QString temporaryFile(const QString &tmpDirPath, const QModelIndex &index,
    int fileNumber)
{
	if (tmpDirPath.isEmpty()) {
		Q_ASSERT(0);
		return QString();
	}

	QString subirPath(tmpDirPath + QDir::separator() +
	    QString::number(fileNumber++));
	{
		/*
		 * Create a separate subdirectory because the files
		 * may have equal names.
		 */
		QDir dir(tmpDirPath);
		if (!dir.mkpath(subirPath)) {
			logError("Could not create directory '%s'.",
			    subirPath.toUtf8().constData());
			return QString();
		}
	}
	QString attachAbsPath(subirPath + QDir::separator());
	{
		/* Determine full file path. */
		QModelIndex fileNameIndex = index.sibling(index.row(),
		    AttachmentTblModel::FNAME_COL);
		if(!fileNameIndex.isValid()) {
			Q_ASSERT(0);
			return QString();
		}
		QString attachFileName(fileNameIndex.data().toString());
		if (attachFileName.isEmpty()) {
			Q_ASSERT(0);
			return QString();
		}
		attachAbsPath += attachFileName;
	}
	QByteArray attachData;
	{
		/* Obtain data. */
		QModelIndex dataIndex = index.sibling(index.row(),
		    AttachmentTblModel::BINARY_CONTENT_COL);
		if (!dataIndex.isValid()) {
			Q_ASSERT(0);
			return QString();
		}
		attachData = dataIndex.data().toByteArray();
		if (attachData.isEmpty()) {
			Q_ASSERT(0);
			return QString();
		}
	}
	if (WF_SUCCESS != writeFile(attachAbsPath, attachData, false)) {
		return QString();
	}

	return attachAbsPath;
}

QStringList AttachmentTblModel::accessibleFiles(const QString &tmpDirPath,
    const QModelIndexList &indexes) const
{
	QStringList accessibleFileList;
	int fileNumber = 0;

	foreach (const QModelIndex &idx, indexes) {
		fileNumber++;
		const QString fPath(_data(idx.row(), FPATH_COL,
		    Qt::DisplayRole).toString());

		QString fileName;

		if (fPath == LOCAL_DATABASE_STR) {
			fileName = temporaryFile(tmpDirPath, idx, fileNumber);
		} else if (fileReadable(fPath)) {
			fileName = fPath;
		} else {
			/* This fallback should not be used. */
			fileName = temporaryFile(tmpDirPath, idx, fileNumber);
		}

		if (fileName.isEmpty()) {
			return QStringList();
		}

		accessibleFileList.append(fileName);
	}

	return accessibleFileList;
}
