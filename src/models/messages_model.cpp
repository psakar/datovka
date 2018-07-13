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
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/delegates/tag_item.h"
#include "src/global.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h" /* Direct access to tag database, */
#include "src/isds/type_description.h"
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
	m_columnCount = _COLNUM;
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	/* Draw records management and tag information. */
	/* TODO -- This is only a temporal solution. */
	if (index.column() == (_COLNUM + _RECMGMT_NEG_COL)) {
		return recMgmtData(index, role);
	} else if (index.column() == (_COLNUM + _TAGS_NEG_COL)) {
		return tagsData(index, role);
	}

	switch (role) {
	case Qt::DisplayRole:
		switch (index.column()) {
		case _DELIVERY_COL:
		case _ACCEPT_COL:
			/* Convert date on display. */
			return dateTimeStrFromDbFormat(
			    _data(index, role).toString(),
			    dateTimeDisplayFormat);
			break;
		case _READLOC_COL: /* 'read locally' */
		case _PROCSNG_COL: /* 'process status' */
		case _ATTDOWN_COL: /* 'is downloaded' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return _data(index, role);
			break;
		}
		break;

	case Qt::DecorationRole:
		switch (index.column()) {
		case _READLOC_COL:
			/* Show icon for 'read locally'. */
			if (_data(index).toBool()) {
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			} else {
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			}
			break;
		case _PROCSNG_COL:
			/* Show icon for 'process status'. */
			switch (_data(index).toInt()) {
			case UNSETTLED:
				{
					QIcon ico;
					ico.addFile(QStringLiteral(ICON_16x16_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_24x24_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_32x32_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
					return ico;
				}
				break;
			case IN_PROGRESS:
				{
					QIcon ico;
					ico.addFile(QStringLiteral(ICON_16x16_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_24x24_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_32x32_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
					return ico;
				}
				break;
			case SETTLED:
				{
					QIcon ico;
					ico.addFile(QStringLiteral(ICON_16x16_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_24x24_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
					ico.addFile(QStringLiteral(ICON_32x32_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
					return ico;
				}
				break;
			default:
				Q_ASSERT(0);
				break;
			}
			return QVariant();
			break;
		case _ATTDOWN_COL:
			/* Show icon for 'is downloaded'. */
			if (_data(index).toBool()) {
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			} else {
				return QVariant(); /* No icon. */
			}
			break;
		default:
			return _data(index, role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		if (index.column() == _MSGSTAT_COL) {
			/* Message state in sent messages. */
			return Isds::Description::descrDmState(
			    Isds::variant2DmState(_data(index, Qt::DisplayRole)));
		}
		return QVariant();
		break;

	case Qt::FontRole:
		if (m_type == WORKING_RCVD) {
			/* In received messages. */
			if (!_data(index.sibling(index.row(),
			        _READLOC_COL)).toBool()) {
				/* Unread messages are shown bold. */
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}
		return _data(index, role);
		break;

	case Qt::AccessibleTextRole:
		switch (index.column()) {
		case _DMID_COL:
			return tr("message identifier") + QLatin1String(" ") +
			    _data(index, Qt::DisplayRole).toString();
			break;
		case _READLOC_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QStringLiteral(" ") +
			    _data(index, Qt::DisplayRole).toString() +
			    QStringLiteral(" ") +
			    Isds::Description::descrDmState(
			        Isds::variant2DmState(_data(index, Qt::DisplayRole)));
			break;
		default:
			/* Continue with code. */
			break;
		}
		switch (index.column()) {
		case _DELIVERY_COL:
		case _ACCEPT_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    dateTimeStrFromDbFormat(
			        _data(index, Qt::DisplayRole).toString(),
			        dateTimeDisplayFormat);
			break;
		case _READLOC_COL: /* 'read locally' */
			if (_data(index).toBool()) {
				return tr("marked as read");
			} else {
				return tr("marked as unread");
			}
			break;
		case _ATTDOWN_COL: /* 'is downloaded' */
			if (_data(index).toBool()) {
				return tr("attachments downloaded");
			} else {
				return tr("attachments not downloaded");
			}
			break;
		case _PROCSNG_COL: /* 'process status' */
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
		switch (index.column()) {
		case _READLOC_COL:
		case _ATTDOWN_COL:
			return sortRank(
			    _data(index, Qt::DisplayRole).toBool() ? 1 : 0,
			    index);
			break;
		case _PROCSNG_COL:
			return sortRank(_data(index, Qt::DisplayRole).toInt(),
			    index);
			break;
		case _ANNOT_COL: /* Ignore case for sorting. */
		case _SENDER_COL:
		case _RECIP_COL:
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
	/* Draw additional. */
	if (section >= (_COLNUM + _RECMGMT_NEG_COL)) {
		return _headerData(section, orientation, role);
	}

	switch (role) {
	case Qt::DisplayRole:
		switch (section) {
		case _READLOC_COL: /* 'read locally' */
		case _ATTDOWN_COL: /* 'is downloaded' */
		case _PROCSNG_COL: /* 'process status' */
			/* Hide text. */
			return QVariant();
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::DecorationRole:
		switch (section) {
		case _READLOC_COL: /* 'read locally' */
			{
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			}
			break;
		case _ATTDOWN_COL: /* 'is downloaded' */
			{
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			}
			break;
		case _PROCSNG_COL: /* 'process status' */
			{
				QIcon ico;
				ico.addFile(QStringLiteral(ICON_16x16_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
			}
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		switch (section) {
		case _READLOC_COL: /* 'read locally' */
		case _ATTDOWN_COL: /* 'is downloaded' */
		case _PROCSNG_COL: /* 'process status' */
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
	if (Q_UNLIKELY(m_columnCount != _COLNUM)) {
		Q_ASSERT(0);
		return;
	}
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = WORKING_RCVD;
//		m_columnCount = _COLNUM;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != WORKING_RCVD)) {
			Q_ASSERT(0);
			return;
		}
//		if (Q_UNLIKELY(m_columnCount != _COLNUM)) {
//			Q_ASSERT(0);
//			return;
//		}
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

		row[_DMID_COL] = entry.dmId;
		row[_ANNOT_COL] = entry.dmAnnotation;
		row[_SENDER_COL] = entry.dmSender;
		row[_DELIVERY_COL] = entry.dmDeliveryTime;
		row[_ACCEPT_COL] = entry.dmAcceptanceTime;
		row[_READLOC_COL] = entry.readLocally;
		row[_ATTDOWN_COL] = entry.isDownloaded;
		row[_PROCSNG_COL] = entry.processStatus;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void DbMsgsTblModel::appendData(const QList<MessageDb::SntEntry> &entryList,
    int appendedColsNum)
{
	if (Q_UNLIKELY(m_columnCount != _COLNUM)) {
		Q_ASSERT(0);
		return;
	}
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = WORKING_SNT;
//		m_columnCount = _COLNUM;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != WORKING_SNT)) {
			Q_ASSERT(0);
			return;
		}
//		if (Q_UNLIKELY(m_columnCount != _COLNUM)) {
//			Q_ASSERT(0);
//			return;
//		}
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

		row[_DMID_COL] = entry.dmId;
		row[_ANNOT_COL] = entry.dmAnnotation;
		row[_RECIP_COL] = entry.dmRecipient;
		row[_DELIVERY_COL] = entry.dmDeliveryTime;
		row[_ACCEPT_COL] = entry.dmAcceptanceTime;
		row[_MSGSTAT_COL] = entry.dmMessageStatus;
		row[_ATTDOWN_COL] = entry.isDownloaded;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

bool DbMsgsTblModel::setType(enum Type type)
{
	m_type = type;

	return true;
}

enum DbMsgsTblModel::Type DbMsgsTblModel::type(void) const
{
	return m_type;
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

bool DbMsgsTblModel::setHeader(const QList<AppendedCol> &appendedCols)
{
	/* Sets description and data type. */

	setHeaderData(_DMID_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[0]).desc,
	    Qt::DisplayRole);
	setHeaderData(_DMID_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[0]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_ANNOT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[1]).desc,
	    Qt::DisplayRole);
	setHeaderData(_ANNOT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[1]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_SENDER_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[2]).desc,
	    Qt::DisplayRole);
	setHeaderData(_SENDER_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[2]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_RECIP_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[2]).desc,
	    Qt::DisplayRole);
	setHeaderData(_RECIP_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[2]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_DELIVERY_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[3]).desc,
	    Qt::DisplayRole);
	setHeaderData(_DELIVERY_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[3]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_ACCEPT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[4]).desc,
	    Qt::DisplayRole);
	setHeaderData(_ACCEPT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[4]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_READLOC_COL, Qt::Horizontal,
	    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[5]).desc,
	    Qt::DisplayRole);
	setHeaderData(_READLOC_COL, Qt::Horizontal,
	    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[5]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_MSGSTAT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[5]).desc,
	    Qt::DisplayRole);
	setHeaderData(_MSGSTAT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[5]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_ATTDOWN_COL, Qt::Horizontal,
	    tr("Attachments downloaded"), Qt::DisplayRole);
	setHeaderData(_ATTDOWN_COL, Qt::Horizontal,
	    DB_BOOL_ATTACHMENT_DOWNLOADED, ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(_PROCSNG_COL, Qt::Horizontal, tr("Processing state"),
	    Qt::DisplayRole);
	setHeaderData(_PROCSNG_COL, Qt::Horizontal,
	    DB_INT_PROCESSING_STATE, ROLE_MSGS_DB_ENTRY_TYPE);

	appendHeaderColumns(this, _BASIC_COLNUM, appendedCols);

	return true;
}

bool DbMsgsTblModel::overrideRead(qint64 dmId, bool forceRead)
{
	if (WORKING_RCVD != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, _DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][_READLOC_COL] = QVariant(forceRead);

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
		if (_data(row, _DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][_ATTDOWN_COL] = QVariant(forceDownloaded);

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
		if (_data(row, _DMID_COL, Qt::DisplayRole).toLongLong() ==
		    dmId) {
			m_data[row][_PROCSNG_COL] = QVariant(forceState);

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
	id += _data(index.sibling(index.row(), _DMID_COL),
	    Qt::DisplayRole).toLongLong(); /* dmId */
	return id;
}
