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
#include <QIcon>
#include <QSqlRecord>

#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/models/messages_model.h"

const int DbMsgsTblModel::m_rowAllocationIncrement(128);

DbMsgsTblModel::DbMsgsTblModel(enum DbMsgsTblModel::Type type,
    QObject *parent)
    : QAbstractTableModel(parent),
    m_headerData(),
    m_data(),
    m_rowCount(0),
    m_columnCount(0),
    m_type(type)
{
}

int DbMsgsTblModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_rowCount;
	}
}

int DbMsgsTblModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_columnCount;
	}
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	int dataType;

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
			{
				/* Show icon for 'read locally'. */
//				qint64 dmId = _data(
//				    index.sibling(index.row(), 0),
//				    Qt::DisplayRole).toLongLong();
//				if (m_overriddenRL.value(dmId,
//				        _data(index).toBool())) {
				if (_data(index).toBool()) {
					return QIcon(
					    ICON_14x14_PATH "grey.png");
				} else {
					return QIcon(
					    ICON_14x14_PATH "green.png");
				}
			}
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			{
				/* Show icon for 'is downloaded'. */
//				qint64 dmId = _data(
//				    index.sibling(index.row(), 0),
//				    Qt::DisplayRole).toLongLong();
//				if (m_overriddenAD.value(dmId, false) ||
//				    _data(index).toBool()) {
				if (_data(index).toBool()) {
					return QIcon(
					    ICON_14x14_PATH "attachment.png");
				} else {
					return QVariant(); /* No icon. */
				}
			}
			break;
		case DB_INT_PROCESSING_STATE:
			{
				/* Show icon for 'process status'. */
//				qint64 dmId = _data(
//				    index.sibling(index.row(), 0),
//				    Qt::DisplayRole).toLongLong();
//				switch (m_overriddenPS.value(dmId,
//				            _data(index).toInt())) {
				switch (_data(index).toInt()) {
				case UNSETTLED:
					return QIcon(
					    ICON_14x14_PATH "red.png");
					break;
				case IN_PROGRESS:
					return QIcon(
					    ICON_14x14_PATH "yellow.png");
					break;
				case SETTLED:
					return QIcon(
					    ICON_14x14_PATH "grey.png");
					break;
				default:
					Q_ASSERT(0);
					break;
				}
				return QVariant();
			}
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
//			qint64 dmId = _data(
//			    index.sibling(index.row(), 0),
//			    Qt::DisplayRole).toLongLong();
//			if (!m_overriddenRL.value(dmId,
//			        _data(index.sibling(index.row(),
//			            READLOC_COL)).toBool())) {
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
			{
				qint64 id;
				id = _data(index, Qt::DisplayRole).toBool() ?
				    1 : 0;
				id = id << 48;
				id += _data(index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				return id;
			}
			break;
		case DB_BOOL_READ_LOCALLY:
			{
				qint64 dmId = _data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
//				id = m_overriddenRL.value(dmId,
//				    _data(index,
//				        Qt::DisplayRole).toBool()) ? 1 : 0;
				id = _data(index, Qt::DisplayRole).toBool() ?
				    1 : 0;
				id = id << 48;
				id += dmId;
				return id;
			}
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			{
				qint64 dmId = _data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
//				id = (m_overriddenAD.value(dmId, false) ||
//				    _data(index,
//				        Qt::DisplayRole).toBool()) ? 1 : 0;
				id = _data(index, Qt::DisplayRole).toBool() ?
				    1 : 0;
				id = id << 48;
				id += dmId;
				return id;
			}
			break;
		case DB_INT_PROCESSING_STATE:
			{
				qint64 dmId = _data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
//				id = m_overriddenPS.value(dmId,
//				    _data(index,
//				        Qt::DisplayRole).toInt());
				id = _data(index, Qt::DisplayRole).toInt();
				id = id << 48;
				id += dmId;
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

bool DbMsgsTblModel::setHeaderData(int section, Qt::Orientation orientation,
    const QVariant &value, int role)
{
	/* Orientation is ignored. */

	m_headerData[section][role] = value;

	emit headerDataChanged(orientation, section, section);

	return true;
}

QVariant DbMsgsTblModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	int dataType;

	switch (role) {
	case Qt::DisplayRole:
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
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
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
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
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
			return QVariant();
		}

		dataType = _headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY: /* 'read locally'. */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded'. */
		case DB_INT_PROCESSING_STATE: /* 'process status'. */
			/* Tool tip. */
			return _headerData(section, orientation, Qt::EditRole);
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

bool DbMsgsTblModel::setType(enum DbMsgsTblModel::Type type)
{
	m_type = type;

	switch (m_type) {
	case DUMMY_RECEIVED:
		return setRcvdHeader();
		break;
	case DUMMY_SENT:
		return setSntHeader();
		break;
	default:
		return true;
		break;
	}
}

void DbMsgsTblModel::setQuery(QSqlQuery &query)
{
	beginResetModel();

	m_data.clear();
	m_rowsAllocated = 0;
	m_rowCount = 0;
	m_columnCount = query.record().count();

	query.first();
	while (query.isActive() && query.isValid()) {

		if (m_rowCount == m_rowsAllocated) {
			m_rowsAllocated += m_rowAllocationIncrement;
			m_data.resize(m_rowsAllocated);
		}

		QVector<QVariant> row(m_columnCount);

		for (int i = 0; i < m_columnCount; ++i) {
			row[i] = query.value(i);
		}

		m_data[m_rowCount++] = row;

		query.next();
	}

	endResetModel();
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

bool DbMsgsTblModel::setRcvdHeader(void)
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

	return true;
}

bool DbMsgsTblModel::setSntHeader(void)
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

	return true;
}

bool DbMsgsTblModel::overrideRead(qint64 dmId, bool forceRead)
{
	if (columnCount() != 8) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][READLOC_COL] = QVariant(forceRead);

			emit dataChanged(QAbstractTableModel::index(row, 0),
			    QAbstractTableModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

bool DbMsgsTblModel::overrideDownloaded(qint64 dmId, bool forceDownloaded)
{
	if (columnCount() != 8) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][ATTDOWN_COL] = QVariant(forceDownloaded);

			emit dataChanged(QAbstractTableModel::index(row, 0),
			    QAbstractTableModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

bool DbMsgsTblModel::overrideProcessing(qint64 dmId,
    enum MessageProcessState forceState)
{
	if (columnCount() != 8) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][PROCSNG_COL] = QVariant(forceState);

			emit dataChanged(QAbstractTableModel::index(row, 0),
			    QAbstractTableModel::index(row, columnCount() - 1));
			return true;
		}
	}

	return false;
}

DbMsgsTblModel &DbMsgsTblModel::dummyModel(enum DbMsgsTblModel::Type type)
{
	static DbMsgsTblModel dummy(DUMMY_RECEIVED);
	dummy.setType(type);
	return dummy;
}

QVariant DbMsgsTblModel::_data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if ((index.row() < m_rowCount) && (index.column() < m_columnCount)) {
		return m_data.at(index.row()).at(index.column());
	} else {
		return QVariant();
	}
}

QVariant DbMsgsTblModel::_data(int row, int col, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if ((row < m_rowCount) && (col < m_columnCount)) {
		return m_data.at(row).at(col);
	} else {
		return QVariant();
	}
}

QVariant DbMsgsTblModel::_headerData(int section,
    Qt::Orientation orientation, int role) const
{
	/* Unused. */
	(void) orientation;

	return m_headerData[section][role];
}
