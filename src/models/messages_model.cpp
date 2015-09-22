/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#include <QString>
#include <QVector>

#include "messages_model.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/message_db.h"

DbMsgsTblModel::DbMsgsTblModel(enum Type type, QObject *parent)
    : QSqlQueryModel(parent),
    m_overriddenRL(),
    m_overriddenAD(),
    m_overriddenPS(),
    m_type(type)
{
	switch (m_type) {
	case DUMMY_RECEIVED:
		setRcvdHeader();
		break;
	case DUMMY_SENT:
		setSntHeader();
		break;
	default:
		break;
	}
}

void DbMsgsTblModel::setType(enum Type type)
{
	m_type = type;

	switch (m_type) {
	case DUMMY_RECEIVED:
		setRcvdHeader();
		break;
	case DUMMY_SENT:
		setSntHeader();
		break;
	default:
		break;
	}
}

int DbMsgsTblModel::columnCount(const QModelIndex &parent) const
{
	switch (m_type) {
	case DUMMY_RECEIVED:
		return rcvdItemIds().size();
		break;
	case DUMMY_SENT:
		return sntItemIds().size();
		break;
	default:
		return QSqlQueryModel::columnCount(parent);
		break;
	}
}

int DbMsgsTblModel::rowCount(const QModelIndex &parent) const
{
	switch (m_type) {
	case DUMMY_RECEIVED:
		return 0;
		break;
	case DUMMY_SENT:
		return 0;
		break;
	default:
		return QSqlQueryModel::rowCount(parent);
		break;
	}
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	int dataType;

	switch (role) {
	case Qt::DisplayRole:
		dataType = QSqlQueryModel::headerData(index.column(),
		    Qt::Horizontal, ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_DATETIME:
			/* Convert date on display. */
			return dateTimeStrFromDbFormat(
			    QSqlQueryModel::data(index, role).toString(),
			    dateTimeDisplayFormat);
			break;
		case DB_BOOL_READ_LOCALLY: /* 'read locally' */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded' */
		case DB_INT_PROCESSING_STATE: /* 'process status' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return QSqlQueryModel::data(index, role);
			break;
		}
		break;

	case Qt::DecorationRole:
		dataType = QSqlQueryModel::headerData(index.column(),
		    Qt::Horizontal, ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY:
			{
				/* Show icon for 'read locally'. */
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				if (m_overriddenRL.value(dmId,
				        QSqlQueryModel::data(
				            index).toBool())) {
					return QIcon(
					    ICON_16x16_PATH "grey.png");
				} else {
					return QIcon(
					    ICON_16x16_PATH "green.png");
				}
			}
			break;
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			{
				/* Show icon for 'is downloaded'. */
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				if (m_overriddenAD.value(dmId, false) ||
				    QSqlQueryModel::data(index).toBool()) {
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
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				switch (m_overriddenPS.value(dmId,
				            QSqlQueryModel::data(
				                index).toInt())) {
				case UNSETTLED:
					return QIcon(
					    ICON_16x16_PATH "red.png");
					break;
				case IN_PROGRESS:
					return QIcon(
					    ICON_16x16_PATH "yellow.png");
					break;
				case SETTLED:
					return QIcon(
					    ICON_16x16_PATH "grey.png");
					break;
				default:
					Q_ASSERT(0);
					break;
				}
				return QVariant();
			}
			break;

		default:
			return QSqlQueryModel::data(index, role);
			break;
		}
		break;

	case Qt::FontRole:
		if (DB_BOOL_READ_LOCALLY == QSqlQueryModel::headerData(
		        READLOC_COL, Qt::Horizontal,
		        ROLE_MSGS_DB_ENTRY_TYPE).toInt()) {
			/* In read messages. */
			qint64 dmId = QSqlQueryModel::data(
			    index.sibling(index.row(), 0),
			    Qt::DisplayRole).toLongLong();
			if (!m_overriddenRL.value(dmId,
			        QSqlQueryModel::data(index.sibling(index.row(),
			            READLOC_COL)).toBool())) {
				/* Unread messages are shown bold. */
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}

		return QSqlQueryModel::data(index, role);
		break;

	case ROLE_PLAIN_DISPLAY:
		return QSqlQueryModel::data(index, Qt::DisplayRole);
		break;

	case ROLE_MSGS_DB_PROXYSORT:
		dataType = QSqlQueryModel::headerData(index.column(),
		    Qt::Horizontal, ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOLEAN:
			{
				qint64 id;
				id = QSqlQueryModel::data(index,
				    Qt::DisplayRole).toBool() ? 1 : 0;
				id = id << 48;
				id += QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				return id;
			}
			break;
		case DB_BOOL_READ_LOCALLY:
			{
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
				id = m_overriddenRL.value(dmId,
				    QSqlQueryModel::data(index,
				        Qt::DisplayRole).toBool()) ? 1 : 0;
				id = id << 48;
				id += dmId;
				return id;
			}
		case DB_BOOL_ATTACHMENT_DOWNLOADED:
			{
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
				id = (m_overriddenAD.value(dmId, false) ||
				    QSqlQueryModel::data(index,
				        Qt::DisplayRole).toBool()) ? 1 : 0;
				id = id << 48;
				id += dmId;
				return id;
			}
			break;
		case DB_INT_PROCESSING_STATE:
			{
				qint64 dmId = QSqlQueryModel::data(
				    index.sibling(index.row(), 0),
				    Qt::DisplayRole).toLongLong();
				qint64 id;
				id = m_overriddenPS.value(dmId,
				    QSqlQueryModel::data(index,
				        Qt::DisplayRole).toInt());
				id = id << 48;
				id += dmId;
				return id;
			}
			break;
		case DB_TEXT: /* Ignore case for sorting. */
			return QSqlQueryModel::data(index,
			    Qt::DisplayRole).toString().toLower();
			break;
		default:
			return QSqlQueryModel::data(index, Qt::DisplayRole);
			break;
		}
		break;

	default:
		return QSqlQueryModel::data(index, role);
		break;
	}
}

QVariant DbMsgsTblModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	int dataType;

	switch (role) {
	case Qt::DisplayRole:
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
			return QSqlQueryModel::headerData(section, orientation,
			    role);
		}

		dataType = QSqlQueryModel::headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY: /* 'read locally' */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded' */
		case DB_INT_PROCESSING_STATE: /* 'process status' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return QSqlQueryModel::headerData(section, orientation,
			    role);
			break;
		}
		break;

	case Qt::DecorationRole:
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
			return QSqlQueryModel::headerData(section, orientation,
			    role);
		}

		dataType = QSqlQueryModel::headerData(section, Qt::Horizontal,
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
			return QSqlQueryModel::headerData(section, orientation,
			    role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		if ((section < READLOC_COL) || (section > PROCSNG_COL)) {
			return QVariant();
		}

		dataType = QSqlQueryModel::headerData(section, Qt::Horizontal,
		    ROLE_MSGS_DB_ENTRY_TYPE).toInt();
		switch (dataType) {
		case DB_BOOL_READ_LOCALLY: /* 'read locally'. */
		case DB_BOOL_ATTACHMENT_DOWNLOADED: /* 'is downloaded'. */
		case DB_INT_PROCESSING_STATE: /* 'process status'. */
			/* Tool tip. */
			return QSqlQueryModel::headerData(section, orientation,
			    Qt::EditRole);
			break;
		default:
			return QVariant();
			break;
		}
		break;

	default:
		return QSqlQueryModel::headerData(section, orientation, role);
		break;
	}
}

bool DbMsgsTblModel::overrideRead(qint64 dmId, bool forceRead)
{
	m_overriddenRL[dmId] = forceRead;

	/*
	 * The model should be forced to emit dataChanged(). However, finding
	 * the proper model index here is painful. Therefore ensure that the
	 * signal is emitted after calling this function.
	 */

	return true;
}

bool DbMsgsTblModel::overrideDownloaded(qint64 dmId, bool forceDownloaded)
{
	m_overriddenAD[dmId] = forceDownloaded;

	/*
	 * The model should be forced to emit dataChanged(). However, finding
	 * the proper model index here is painful. Therefore ensure that the
	 * signal is emitted after calling this function.
	 */

	return true;
}

bool DbMsgsTblModel::overrideProcessing(qint64 dmId,
    enum MessageProcessState forceState)
{
	m_overriddenPS[dmId] = forceState;

	/*
	 * The model should be forced to emit dataChanged(). However, finding
	 * the proper model index here is painful. Therefore ensure that the
	 * signal is emitted after calling this function.
	 */

	return true;
}

void DbMsgsTblModel::clearOverridingData(void)
{
	m_overriddenRL.clear();
	m_overriddenAD.clear();
	m_overriddenPS.clear();
}

bool DbMsgsTblModel::setRcvdHeader(void)
{
	for (int i = 0; i < rcvdItemIds().size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(rcvdItemIds()[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			this->setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(rcvdItemIds()[i]).desc);
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(rcvdItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(rcvdItemIds()[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			this->setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(rcvdItemIds()[i]).desc);
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(rcvdItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == rcvdItemIds()[i]) {
			/* Description. */
			this->setHeaderData(i, Qt::Horizontal,
			    QObject::tr("Attachments downloaded"));
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
			    DB_BOOL_ATTACHMENT_DOWNLOADED,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("process_status" == rcvdItemIds()[i]) {
			/* Description. */
			this->setHeaderData(i, Qt::Horizontal,
			    QObject::tr("Processing state"));
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
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
			this->setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(sntItemIds()[i]).desc);
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(sntItemIds()[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if ("is_downloaded" == sntItemIds()[i]) {
			/* Description. */
			this->setHeaderData(i, Qt::Horizontal,
			    QObject::tr("Attachments downloaded"));
			/* Data type. */
			this->setHeaderData(i, Qt::Horizontal,
			    DB_BOOL_ATTACHMENT_DOWNLOADED,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			return false;
		}
	}

	return true;
}

/*
 * Joined tables messages, supplementary_message_data and raw_message_data.
 * Entry 'is_downloaded' is generated by the database rather than directly read
 * from a table.
 */
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

DbMsgsTblModel &DbMsgsTblModel::dummyModel(void)
{
	static DbMsgsTblModel dummy(DbMsgsTblModel::DUMMY_RECEIVED);
	return dummy;
}
