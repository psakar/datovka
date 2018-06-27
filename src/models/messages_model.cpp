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

#include <QFont>

#include "src/common.h"
#include "src/datovka_shared/graphics/graphics.h"
#include "src/datovka_shared/io/records_management_db.h"
#include "src/delegates/tag_item.h"
#include "src/global.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h" /* Direct access to tag database, */
#include "src/log/log.h"
#include "src/models/messages_model.h"

/*
 * Specifies number of bits to be used for message id when combining with other
 * data.
 */
#define MSG_ID_WIDTH 48

DbMsgsTblModel::DbMsgsTblModel(enum DbMsgsTblModel::Type type, QObject *parent)
    : TblModel(parent),
    m_type(type),
    m_dsIco(ICON_3PARTY_PATH "up_16.png")
{
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	int dataType;

#define TAGS_OFFS 1 /* Tags are located behind upload information. */

	/* Draw records management and tag information. */
	/* TODO -- This is only a temporal solution. */
	switch (m_type) {
	case WORKING_RCVD:
		if (index.column() == (PROCSNG_COL + TAGS_OFFS)) {
			return recMgmtData(index, role);
		} else if (index.column() > (PROCSNG_COL + TAGS_OFFS)) {
			return tagsData(index, role);
		}
		break;
	case WORKING_SNT:
		if (index.column() == (ATTDOWN_COL + TAGS_OFFS)) {
			return recMgmtData(index, role);
		} else if (index.column() > (ATTDOWN_COL + TAGS_OFFS)) {
			return tagsData(index, role);
		}
		break;
	default:
		Q_ASSERT(0);
		return QVariant();
		break;
	}

#undef TAGS_OFFS

	switch (role) {
	case Qt::DisplayRole:
		dataType = _headerData(index.column(), Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_DATETIME:
			/* Convert date on display. */
			return dateTimeStrFromDbFormat(
			    _data(index, role).toString(),
			    dateTimeDisplayFormat);
			break;
		case DB_BOOL_READ_LOCALLY: /* 'read locally' */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded' */
		case DB_INT_PROCESSING_STATE: /* 'process status' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return _data(index, role);
			break;
		}
		break;

	case Qt::DecorationRole:
		dataType = _headerData(index.column(), Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY:
			/* Show icon for 'read locally'. */
			if (_data(index).toBool()) {
				return QIcon(ICON_14x14_PATH "grey.png");
			} else {
				return QIcon(ICON_14x14_PATH "green.png");
			}
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			/* Show icon for 'is downloaded'. */
			if (_data(index).toBool()) {
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			} else {
				return QVariant(); /* No icon. */
			}
			break;
		case DB_INT_PROCESSING_STATE:
			/* Show icon for 'process status'. */
			switch (_data(index).toInt()) {
			case UNSETTLED:
				return QIcon(ICON_14x14_PATH "red.png");
				break;
			case IN_PROGRESS:
				return QIcon(ICON_14x14_PATH "yellow.png");
				break;
			case SETTLED:
				return QIcon(ICON_14x14_PATH "grey.png");
				break;
			default:
				Q_ASSERT(0);
				break;
			}
			return QVariant();
			break;

		default:
			return _data(index, role);
			break;
		}
		break;

	case Qt::FontRole:
		if (DB_BOOL_READ_LOCALLY == _headerData(READLOC_STATUS_COL,
		        Qt::Horizontal, ROLE_MSGS_DB_ENTRY_TYPE).toInt()) {
			/* In read messages. */
			if (!_data(index.sibling(index.row(),
			        READLOC_STATUS_COL)).toBool()) {
				/* Unread messages are shown bold. */
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}

		return _data(index, role);
		break;

	case Qt::AccessibleTextRole:
		if (index.column() == DMID_COL) {
			return tr("message identifier") + QLatin1String(" ") +
			    _data(index, Qt::DisplayRole).toString();
		}
		dataType = _headerData(index.column(), Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_DATETIME:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    dateTimeStrFromDbFormat(
			        _data(index, Qt::DisplayRole).toString(),
			        dateTimeDisplayFormat);
			break;
		case DB_BOOL_READ_LOCALLY: /* 'read locally' */
			if (_data(index).toBool()) {
				return tr("marked as read");
			} else {
				return tr("marked as unread");
			}
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded' */
			if (_data(index).toBool()) {
				return tr("attachments downloaded");
			} else {
				return tr("attachments not downloaded");
			}
			break;
		case DB_INT_PROCESSING_STATE: /* 'process status' */
			{
				QString headerPref(headerData(index.column(),
				     Qt::Horizontal, Qt::ToolTipRole).toString());
				headerPref += QLatin1String(" ");

				switch (_data(index).toInt()) {
				case UNSETTLED:
					return headerPref + tr("unsettled");
					break;
				case IN_PROGRESS:
					return headerPref + tr("in progress");
					break;
				case SETTLED:
					return headerPref + tr("settled");
					break;
				default:
					Q_ASSERT(0);
					break;
				}
			}
			return QVariant();
			break;
		default:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    _data(index, Qt::DisplayRole).toString();
			break;
		}
		break;

	case ROLE_PLAIN_DISPLAY:
		return _data(index, Qt::DisplayRole);
		break;

	case ROLE_MSGS_DB_PROXYSORT:
		dataType = _headerData(index.column(), Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOLEAN:
		case DB_BOOL_READ_LOCALLY:
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			return sortRank(
			    _data(index, Qt::DisplayRole).toBool() ? 1 : 0,
			    index);
			break;
		case DB_INT_PROCESSING_STATE:
			return sortRank(_data(index, Qt::DisplayRole).toInt(),
			    index);
			break;
		case DB_TEXT: /* Ignore case for sorting. */
			return _data(index,
			    Qt::DisplayRole).toString().toLower();
			break;
		default:
			return _data(index, Qt::DisplayRole);
			break;
		}
		break;

	default:
		return _data(index, role);
		break;
	}
}

QVariant DbMsgsTblModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	int dataType;
	QIcon ico;

	/* Draw additional. */
	switch (m_type) {
	case WORKING_RCVD:
		if (section > PROCSNG_COL) {
			return _headerData(section, orientation, role);
		}
		break;
	case WORKING_SNT:
		if (section > ATTDOWN_COL) {
			return _headerData(section, orientation, role);
		}
		break;
	default:
		Q_ASSERT(0);
		return QVariant();
		break;
	}

	switch (role) {
	case Qt::DisplayRole:
		if (section < READLOC_STATUS_COL) {
			return _headerData(section, orientation, role);
		}

		dataType = _headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY: /* 'read locally' */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded' */
		case DB_INT_PROCESSING_STATE: /* 'process status' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::DecorationRole:
		if (section < READLOC_STATUS_COL) {
			return _headerData(section, orientation, role);
		}
		dataType = _headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY:
			/* Show icon for 'read locally'. */
			ico.addFile(QStringLiteral(ICON_16x16_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
			ico.addFile(QStringLiteral(ICON_32x32_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
			return ico;
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			/* Show icon for 'is downloaded'. */
			ico.addFile(QStringLiteral(ICON_16x16_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
			ico.addFile(QStringLiteral(ICON_32x32_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
			return ico;
			break;
		case DB_INT_PROCESSING_STATE:
			ico.addFile(QStringLiteral(ICON_16x16_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
			ico.addFile(QStringLiteral(ICON_32x32_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
			return ico;
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		if (section < READLOC_STATUS_COL) {
			return QVariant();
		}

		dataType = _headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY: /* 'read locally'. */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded'. */
		case DB_INT_PROCESSING_STATE: /* 'process status'. */
			/* Tool tip. */
			return _headerData(section, orientation, Qt::DisplayRole);
			break;
		default:
			return QVariant();
			break;
		}
		break;

	default:
		return _headerData(section, orientation, role);
		break;
	}
}

void DbMsgsTblModel::appendData(const QList<MessageDb::RcvdEntry> &entryList,
    int appendedColsNum)
{
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = WORKING_RCVD;
		m_columnCount = MessageDb::rcvdItemIds.size() + appendedColsNum;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != WORKING_RCVD)) {
			Q_ASSERT(0);
			return;
		}
		if (Q_UNLIKELY(m_columnCount !=
		        (MessageDb::rcvdItemIds.size() + appendedColsNum))) {
			Q_ASSERT(0);
			return;
		}
	}

	if (entryList.isEmpty()) {
		/* Don't do anything. */
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const MessageDb::RcvdEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[DMID_COL] = entry.dmId;
		row[ANNOT_COL] = entry.dmAnnotation;
		row[SENDER_RECIP_COL] = entry.dmSender;
		row[DELIVERY_COL] = entry.dmDeliveryTime;
		row[ACCEPT_COL] = entry.dmAcceptanceTime;
		row[READLOC_STATUS_COL] = entry.readLocally;
		row[ATTDOWN_COL] = entry.isDownloaded;
		row[PROCSNG_COL] = entry.processStatus;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void DbMsgsTblModel::appendData(const QList<MessageDb::SntEntry> &entryList,
    int appendedColsNum)
{
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = WORKING_SNT;
		m_columnCount = MessageDb::sntItemIds.size() + appendedColsNum;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != WORKING_SNT)) {
			Q_ASSERT(0);
			return;
		}
		if (Q_UNLIKELY(m_columnCount !=
		        (MessageDb::sntItemIds.size() + appendedColsNum))) {
			Q_ASSERT(0);
			return;
		}
	}

	if (entryList.isEmpty()) {
		/* Don't do anything. */
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const MessageDb::SntEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[DMID_COL] = entry.dmId;
		row[ANNOT_COL] = entry.dmAnnotation;
		row[SENDER_RECIP_COL] = entry.dmRecipient;
		row[DELIVERY_COL] = entry.dmDeliveryTime;
		row[ACCEPT_COL] = entry.dmAcceptanceTime;
		row[READLOC_STATUS_COL] = entry.dmMessageStatus;
		row[ATTDOWN_COL] = entry.isDownloaded;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

bool DbMsgsTblModel::setType(enum DbMsgsTblModel::Type type)
{
	m_type = type;

	return true;
}

static
void appendHeaderColumns(DbMsgsTblModel *model, int dfltHdrSize,
    const QList<DbMsgsTblModel::AppendedCol> &appendedCols)
{
	if ((model == Q_NULLPTR) || (dfltHdrSize < 0)) {
		Q_ASSERT(0);
		return;
	}

	for (int i = 0; i < appendedCols.size(); ++i) {
		/* Description. */
		model->setHeaderData(dfltHdrSize + i, Qt::Horizontal,
		    appendedCols.at(i).display, Qt::DisplayRole);
		model->setHeaderData(dfltHdrSize + i, Qt::Horizontal,
		    appendedCols.at(i).decoration, Qt::DecorationRole);
		model->setHeaderData(dfltHdrSize + i, Qt::Horizontal,
		    appendedCols.at(i).toolTip, Qt::ToolTipRole);
		/* Data type. */
		model->setHeaderData(dfltHdrSize + i, Qt::Horizontal,
		    DB_APPENDED_VARIANT, ROLE_MSGS_DB_ENTRY_TYPE);
	}
}

bool DbMsgsTblModel::setRcvdHeader(const QList<AppendedCol> &appendedCols)
{
	for (int i = 0; i < MessageDb::rcvdItemIds.size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(MessageDb::rcvdItemIds[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(MessageDb::rcvdItemIds[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == MessageDb::rcvdItemIds[i]) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    tr("Attachments downloaded"), Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    DB_BOOL_ATTACHMENT_DOWNLOADED,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("process_status" == MessageDb::rcvdItemIds[i]) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    tr("Processing state"), Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    DB_INT_PROCESSING_STATE, ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			return false;
		}
	}

	appendHeaderColumns(this, MessageDb::rcvdItemIds.size(), appendedCols);

	return true;
}

bool DbMsgsTblModel::setSntHeader(const QList<AppendedCol> &appendedCols)
{
	for (int i = 0; i < MessageDb::sntItemIds.size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(MessageDb::sntItemIds[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(MessageDb::sntItemIds[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(MessageDb::sntItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == MessageDb::sntItemIds[i]) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    tr("Attachments downloaded"), Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    DB_BOOL_ATTACHMENT_DOWNLOADED,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			return false;
		}
	}

	appendHeaderColumns(this, MessageDb::sntItemIds.size(), appendedCols);

	return true;
}

bool DbMsgsTblModel::overrideRead(qint64 dmId, bool forceRead)
{
	if (WORKING_RCVD != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][READLOC_STATUS_COL] = QVariant(forceRead);

			emit dataChanged(TblModel::index(row, 0),
			    TblModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

bool DbMsgsTblModel::overrideDownloaded(qint64 dmId, bool forceDownloaded)
{
	if (WORKING_RCVD != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][ATTDOWN_COL] = QVariant(forceDownloaded);

			emit dataChanged(TblModel::index(row, 0),
			    TblModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

bool DbMsgsTblModel::overrideProcessing(qint64 dmId,
    enum MessageProcessState forceState)
{
	if (WORKING_RCVD != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][PROCSNG_COL] = QVariant(forceState);

			emit dataChanged(TblModel::index(row, 0),
			    TblModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

bool DbMsgsTblModel::fillTagsColumn(const QString &userName, int col)
{
	if (Q_NULLPTR == GlobInstcs::tagDbPtr) {
		return false;
	}

	if (userName.isEmpty()) {
		return false;
	}

	/* Check indexes into column. */
	if (col >= 0) {
		if (col > columnCount()) {
			return false;
		}
	} else {
		col += columnCount();
		if (col < 0) {
			return false;
		}
	}

	for (int row = 0; row < rowCount(); ++row) {
		qint64 dmId = TblModel::index(row, 0).data().toLongLong();
		TagItemList tagList(
		    GlobInstcs::tagDbPtr->getMessageTags(userName, dmId));
		tagList.sortNames();
		m_data[row][col] = QVariant::fromValue(tagList);
	}

	emit dataChanged(TblModel::index(0, col),
	    TblModel::index(rowCount() - 1, col));

	return true;
}

bool DbMsgsTblModel::refillTagsColumn(const QString &userName,
    const QList<qint64> &dmIds, int col)
{
	if (Q_NULLPTR == GlobInstcs::tagDbPtr) {
		return false;
	}

	if (userName.isEmpty()) {
		return false;
	}

	/* Check indexes into column. */
	if (col >= 0) {
		if (col > columnCount()) {
			return false;
		}
	} else {
		col += columnCount();
		if (col < 0) {
			return false;
		}
	}

	for (int row = 0; row < rowCount(); ++row) {
		qint64 dmId = TblModel::index(row, 0).data().toLongLong();
		if (dmIds.contains(dmId)) {
			TagItemList tagList(
			    GlobInstcs::tagDbPtr->getMessageTags(userName, dmId));
			tagList.sortNames();
			m_data[row][col] = QVariant::fromValue(tagList);
			emit dataChanged(TblModel::index(row, col),
			    TblModel::index(row, col));
		}
	}

	return true;
}

bool DbMsgsTblModel::setRecordsManagementIcon(void)
{
	QIcon ico;
	ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_16.png"), QSize(), QIcon::Normal, QIcon::Off);
	ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_32.png"), QSize(), QIcon::Normal, QIcon::Off);

	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		m_dsIco = ico;
		return false;
	}

	RecordsManagementDb::ServiceInfoEntry entry(
	    GlobInstcs::recMgmtDbPtr->serviceInfo());
	if (!entry.isValid() || entry.logoSvg.isEmpty()) {
		m_dsIco = ico;
		return false;
	}
	QPixmap pixmap(Graphics::pixmapFromSvg(entry.logoSvg, 16));
	if (pixmap.isNull()) {
		m_dsIco = ico;
		return false;
	}

	m_dsIco = QIcon(pixmap);
	return true;
}

bool DbMsgsTblModel::fillRecordsManagementColumn(int col)
{
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		return false;
	}

	/* Check indexes into column. */
	if (col >= 0) {
		if (col > columnCount()) {
			return false;
		}
	} else {
		col += columnCount();
		if (col < 0) {
			return false;
		}
	}

	for (int row = 0; row < rowCount(); ++row) {
		qint64 dmId = TblModel::index(row, 0).data().toLongLong();
		m_data[row][col] =
		    GlobInstcs::recMgmtDbPtr->storedMsgLocations(dmId);
	}

	emit dataChanged(TblModel::index(0, col),
	    TblModel::index(rowCount() - 1, col));

	return true;
}

bool DbMsgsTblModel::refillRecordsManagementColumn(const QList<qint64> &dmIds,
    int col)
{
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		return false;
	}

	/* Check indexes into column. */
	if (col >= 0) {
		if (col > columnCount()) {
			return false;
		}
	} else {
		col += columnCount();
		if (col < 0) {
			return false;
		}
	}

	for (int row = 0; row < rowCount(); ++row) {
		qint64 dmId = TblModel::index(row, 0).data().toLongLong();
		if (dmIds.contains(dmId)) {
			m_data[row][col] =
			    GlobInstcs::recMgmtDbPtr->storedMsgLocations(dmId);
			emit dataChanged(TblModel::index(row, col),
			    TblModel::index(row, col));
		}
	}

	return true;
}

QVariant DbMsgsTblModel::recMgmtData(const QModelIndex &index, int role) const
{
	QStringList locations;
	{
		QVariant rawData(_data(index, Qt::DisplayRole));
		if (Q_UNLIKELY(!rawData.canConvert<QStringList>())) {
			/*
			 * Cannot call Q_ASSERT(0) here as this method
			 * may get called on invalid data occasionally.
			 */
			logWarningNL("%s",
			    "Did not get records management data.");
			return QVariant();
		}
		locations = rawData.toStringList();
	}

	switch (role) {
	case Qt::DecorationRole:
		return locations.isEmpty() ? QVariant() : m_dsIco;
		break;
	case Qt::ToolTipRole:
		return locations.join(QLatin1String("\n"));
		break;
	case Qt::AccessibleTextRole:
		if (!locations.isEmpty()) {
			return headerData(index.column(), Qt::Horizontal, Qt::ToolTipRole).toString() +
			    QLatin1String(": ") +
			    locations.join(QLatin1String(", "));
		}
		return QVariant();
		break;
	case ROLE_MSGS_DB_PROXYSORT:
		return sortRank(locations.isEmpty() ? 0 : 1, index);
		break;
	default:
		return QVariant();
		break;
	}
}

QVariant DbMsgsTblModel::tagsData(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::AccessibleTextRole:
		{
			QVariant val(_data(index, Qt::DisplayRole));
			if (Q_UNLIKELY(!val.canConvert<TagItemList>())) {
				/*
				 * Cannot call Q_ASSERT(0) here as this method
				 * may get called on invalid data occasionally.
				 */
				logWarningNL("%s", "Did not get tag data.");
				return QVariant();
			}

			const TagItemList tagList(
			    qvariant_cast<TagItemList>(val));
			if (!tagList.isEmpty()) {
				QString descr(
				    headerData(index.column(), Qt::Horizontal).toString() +
				    QLatin1String(": "));
				for (int i = 0; i < tagList.size(); ++i) {
					if (i > 0) {
						descr += QLatin1String(", ");
					}
					descr += tagList.at(i).name;
				}
				return descr;
			}
		}
		return QVariant();
		break;
	case ROLE_MSGS_DB_PROXYSORT:
		/* Sort proxy model is able to handle tags. */
		return _data(index, Qt::DisplayRole);
		break;
	default:
		/* Leave additional tags to delegates. */
		return _data(index, role);
		break;
	}
}

qint64 DbMsgsTblModel::sortRank(qint16 num, const QModelIndex &index) const
{
	qint64 id = num;
	id = id << MSG_ID_WIDTH;
	id += _data(index.sibling(index.row(), DMID_COL),
	    Qt::DisplayRole).toLongLong(); /* dmId */
	return id;
}
