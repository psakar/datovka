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

#include <QFont>

#include "src/common.h"
#include "src/delegates/tag_item.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/document_service_db.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h" /* Direct access to tag database, */
#include "src/io/tag_db_container.h"
#include "src/models/messages_model.h"

const int DbMsgsTblModel::rcvdMsgsColCnt(8);
const int DbMsgsTblModel::sntMsgsColCnt(7);

/*
 * Specifies number of bits to be used for message id when combining with other
 * data.
 */
#define MSG_ID_WIDTH 48

DbMsgsTblModel::DbMsgsTblModel(enum DbMsgsTblModel::Type type, QObject *parent)
    : TblModel(parent),
    m_type(type)
{
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	int dataType;

	/* Leave additional to delegates. */
	switch (m_type) {
	case WORKING_RCVD:
	case DUMMY_RCVD:
		if (index.column() > PROCSNG_COL) {
			return _data(index, role);
		}
		break;
	case WORKING_SNT:
	case DUMMY_SNT:
		if (index.column() > ATTDOWN_COL) {
			return _data(index, role);
		}
		break;
	default:
		Q_ASSERT(0);
		return QVariant();
		break;
	}

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
				return QIcon(ICON_14x14_PATH "attachment.png");
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
		if (DB_BOOL_READ_LOCALLY == _headerData(READLOC_COL,
		        Qt::Horizontal, ROLE_MSGS_DB_ENTRY_TYPE).toInt()) {
			/* In read messages. */
			if (!_data(index.sibling(index.row(),
			        READLOC_COL)).toBool()) {
				/* Unread messages are shown bold. */
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}

		return _data(index, role);
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
			{
				qint64 id =
				    _data(index, Qt::DisplayRole).toBool() ?
				    1 : 0;
				id = id << MSG_ID_WIDTH;
				id += _data(index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong(); /* dmId */
				return id;
			}
			break;
		case DB_INT_PROCESSING_STATE:
			{
				qint64 id =
				    _data(index, Qt::DisplayRole).toInt();
				id = id << MSG_ID_WIDTH;
				id += _data(index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong(); /* dmId */
				return id;
			}
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

	/* Draw additional. */
	switch (m_type) {
	case WORKING_RCVD:
	case DUMMY_RCVD:
		if (section > PROCSNG_COL) {
			return _headerData(section, orientation, role);
		}
		break;
	case WORKING_SNT:
	case DUMMY_SNT:
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
		if (section < READLOC_COL) {
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
		if (section < READLOC_COL) {
			return _headerData(section, orientation, role);
		}

		dataType = _headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY:
			/* Show icon for 'read locally'. */
			return QIcon(ICON_16x16_PATH "readcol.png");
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			/* Show icon for 'is downloaded'. */
			return QIcon(ICON_14x14_PATH "attachment.png");
			break;
		case DB_INT_PROCESSING_STATE:
			return QIcon(ICON_16x16_PATH "flag.png");
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		if (section < READLOC_COL) {
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

void DbMsgsTblModel::setQuery(QSqlQuery &query, enum DbMsgsTblModel::Type type)
{
	setType(type);
	TblModel::setQuery(query);
	/* TODO -- Check whether type matches query. */
}

bool DbMsgsTblModel::appendQueryData(QSqlQuery &query,
    enum DbMsgsTblModel::Type type)
{
	if (type != m_type) {
		return false;
	}
	return TblModel::appendQueryData(query);
}

bool DbMsgsTblModel::setType(enum DbMsgsTblModel::Type type)
{
	m_type = type;

	switch (m_type) {
	case DUMMY_RCVD:
		return setRcvdHeader(QList<DbMsgsTblModel::AppendedCol>()); /* FIXME */
		break;
	case DUMMY_SNT:
		return setSntHeader(QList<DbMsgsTblModel::AppendedCol>()); /* FIXME */
		break;
	default:
		return true;
		break;
	}
}

const QVector<QString> &DbMsgsTblModel::rcvdItemIds(void)
{
	static QVector<QString> ids;
	if (ids.size() == 0) {
		ids.append("dmID");
		ids.append("dmAnnotation");
		ids.append("dmSender");
		ids.append("dmDeliveryTime");
		ids.append("dmAcceptanceTime");
		ids.append("read_locally");
		ids.append("is_downloaded");
		ids.append("process_status");
	}
	return ids;
}

const QVector<QString> &DbMsgsTblModel::sntItemIds(void)
{
	static QVector<QString> ids;
	if (ids.size() == 0) {
	    ids.append("dmID");
	    ids.append("dmAnnotation");
	    ids.append("dmRecipient");
	    ids.append("dmDeliveryTime");
	    ids.append("dmAcceptanceTime");
	    ids.append("dmMessageStatus");
	    ids.append("is_downloaded");
	}
	return ids;
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
	for (int i = 0; i < rcvdItemIds().size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(rcvdItemIds()[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(rcvdItemIds()[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(rcvdItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(rcvdItemIds()[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(rcvdItemIds()[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(rcvdItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == rcvdItemIds()[i]) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    tr("Attachments downloaded"), Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    DB_BOOL_ATTACHMENT_DOWNLOADED,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("process_status" == rcvdItemIds()[i]) {
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

	appendHeaderColumns(this, rcvdItemIds().size(), appendedCols);

	return true;
}

bool DbMsgsTblModel::setSntHeader(const QList<AppendedCol> &appendedCols)
{
	for (int i = 0; i < sntItemIds().size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(sntItemIds()[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(sntItemIds()[i]).desc,
			    Qt::DisplayRole);
			/* Data type. */
			setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(sntItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == sntItemIds()[i]) {
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

	appendHeaderColumns(this, sntItemIds().size(), appendedCols);

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
			m_data[row][READLOC_COL] = QVariant(forceRead);

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

DbMsgsTblModel &DbMsgsTblModel::dummyModel(enum DbMsgsTblModel::Type type)
{
	static DbMsgsTblModel dummy(DUMMY_RCVD);
	dummy.setType(type);
	return dummy;
}

bool DbMsgsTblModel::fillTagsColumn(const QString &userName, int col)
{
	TagDb *tagDb = 0;

	if (isWebDatovkaAccount(userName)) {
		tagDb = globWebDatovkaTagDbPtr->
		     accessTagDb(getWebDatovkaTagDbPrefix(userName));
	} else {
		tagDb = globTagDbPtr;
	}

	if (0 == tagDb) {
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
		TagItemList tagList(tagDb->getMessageTags(userName, dmId));
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
	TagDb *tagDb = 0;

	if (isWebDatovkaAccount(userName)) {
		tagDb = globWebDatovkaTagDbPtr->
		    accessTagDb(getWebDatovkaTagDbPrefix(userName));
	} else {
		tagDb = globTagDbPtr;
	}

	if (0 == tagDb) {
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
			    tagDb->getMessageTags(userName, dmId));
			tagList.sortNames();
			m_data[row][col] = QVariant::fromValue(tagList);
			emit dataChanged(TblModel::index(row, col),
			    TblModel::index(row, col));
		}
	}

	return true;
}

bool DbMsgsTblModel::fillDocumentServiceColumn(int col)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
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
		    globDocumentServiceDbPtr->storedMsgLocations(dmId);
	}

	emit dataChanged(TblModel::index(0, col),
	    TblModel::index(rowCount() - 1, col));

	return true;
}

bool DbMsgsTblModel::refillDocumentServiceColumn(const QList<qint64> &dmIds,
    int col)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
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
			    globDocumentServiceDbPtr->storedMsgLocations(dmId);
			emit dataChanged(TblModel::index(row, col),
			    TblModel::index(row, col));
		}
	}

	return true;
}

void DbMsgsTblModel::setQuery(QSqlQuery &query)
{
	TblModel::setQuery(query);
}

bool DbMsgsTblModel::appendQueryData(QSqlQuery &query)
{
	return TblModel::appendQueryData(query);
}
