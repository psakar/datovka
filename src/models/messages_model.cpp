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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/datovka_shared/graphics/graphics.h"
#include "src/datovka_shared/io/records_management_db.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/datovka_shared/log/log.h"
#include "src/delegates/tag_item.h"
#include "src/global.h"
#include "src/gui/icon_container.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h" /* Direct access to tag database, */
#include "src/isds/type_description.h"
#include "src/models/messages_model.h"

/*
 * Specifies number of bits to be used for message id when combining with other
 * data.
 */
#define MSG_ID_WIDTH 48

static IconContainer inconContainer; /* Local icon container. */

/*!
 * @brief Construct the default records management icon.
 *
 * @return Icon object.
 */
static inline
QIcon defaultRMIcon(void)
{
	return IconContainer::construcIcon(IconContainer::ICON_UP);
}

DbMsgsTblModel::DbMsgsTblModel(enum DbMsgsTblModel::Type type, QObject *parent)
    : TblModel(parent),
    m_type(type),
    m_rmIco(defaultRMIcon())
{
	/* Fixed column size. */
	m_columnCount = MAX_COLNUM;
}

QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
{
	/* Draw records management and tag information. */
	/* TODO -- This is only a temporal solution. */
	if (index.column() == (MAX_COLNUM + RECMGMT_NEG_COL)) {
		return recMgmtData(index, role);
	} else if (index.column() == (MAX_COLNUM + TAGS_NEG_COL)) {
		return tagsData(index, role);
	}

	switch (role) {
	case Qt::DisplayRole:
		switch (index.column()) {
		case DELIVERY_COL:
		case ACCEPT_COL:
			/* Convert date on display. */
			return dateTimeStrFromDbFormat(
			    _data(index, role).toString(),
			    dateTimeDisplayFormat);
			break;
		case PERSDELIV_COL: /* 'personal delivery' */
		case READLOC_COL: /* 'read locally' */
		case PROCSNG_COL: /* 'process status' */
		case ATTDOWN_COL: /* 'is downloaded' */
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
		case PERSDELIV_COL:
			/* Show icon for 'personal delivery'. */
			if (_data(index).toBool()) {
				return inconContainer.icon(IconContainer::ICON_HAND);
			} else {
				return QVariant(); /* No icon. */
			}
			break;
		case READLOC_COL:
			/* Show icon for 'read locally'. */
			if (_data(index).toBool()) {
				return inconContainer.icon(IconContainer::ICON_GREY_BALL);
			} else {
				return inconContainer.icon(IconContainer::ICON_GREEN_BALL);
			}
			break;
		case PROCSNG_COL:
			/* Show icon for 'process status'. */
			switch (_data(index).toInt()) {
			case UNSETTLED:
				return inconContainer.icon(IconContainer::ICON_RED_BALL);
				break;
			case IN_PROGRESS:
				return inconContainer.icon(IconContainer::ICON_YELLOW_BALL);
				break;
			case SETTLED:
				return inconContainer.icon(IconContainer::ICON_GREY_BALL);
				break;
			default:
				Q_ASSERT(0);
				break;
			}
			return QVariant();
			break;
		case ATTDOWN_COL:
			/* Show icon for 'is downloaded'. */
			if (_data(index).toBool()) {
				return inconContainer.icon(IconContainer::ICON_ATTACHMENT);
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
		if (index.column() == MSGSTAT_COL) {
			/* Message state in sent messages. */
			return Isds::Description::descrDmState(
			    Isds::variant2DmState(_data(index, Qt::DisplayRole)));
		}
		return QVariant();
		break;

	case Qt::FontRole:
		if (m_type == RCVD_MODEL) {
			/* In received messages. */
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

	case Qt::AccessibleTextRole:
		switch (index.column()) {
		case DMID_COL:
			return tr("message identifier") + QLatin1String(" ") +
			    _data(index, Qt::DisplayRole).toString();
			break;
		case READLOC_COL:
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
		case PERSDELIV_COL: /* 'personal delivery' */
			if (_data(index).toBool()) {
				return tr("personal delivery");
			} else {
				return tr("not a personal delivery");
			}
			break;
		case DELIVERY_COL:
		case ACCEPT_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    dateTimeStrFromDbFormat(
			        _data(index, Qt::DisplayRole).toString(),
			        dateTimeDisplayFormat);
			break;
		case READLOC_COL: /* 'read locally' */
			if (_data(index).toBool()) {
				return tr("marked as read");
			} else {
				return tr("marked as unread");
			}
			break;
		case ATTDOWN_COL: /* 'is downloaded' */
			if (_data(index).toBool()) {
				return tr("attachments downloaded");
			} else {
				return tr("attachments not downloaded");
			}
			break;
		case PROCSNG_COL: /* 'process status' */
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
		case PERSDELIV_COL:
		case READLOC_COL:
		case ATTDOWN_COL:
			return sortRank(
			    _data(index, Qt::DisplayRole).toBool() ? 1 : 0,
			    index);
			break;
		case PROCSNG_COL:
			return sortRank(_data(index, Qt::DisplayRole).toInt(),
			    index);
			break;
		case ANNOT_COL: /* Ignore case for sorting. */
		case SENDER_COL:
		case RECIP_COL:
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
	if (section >= BASIC_COLNUM) {
		return _headerData(section, orientation, role);
	}

	switch (role) {
	case Qt::DisplayRole:
		switch (section) {
		case PERSDELIV_COL: /* 'personal delivery' */
		case READLOC_COL: /* 'read locally' */
		case ATTDOWN_COL: /* 'is downloaded' */
		case PROCSNG_COL: /* 'process status' */
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
		case PERSDELIV_COL: /* 'personal delivery' */
			return inconContainer.icon(IconContainer::ICON_HAND_GREY);
			break;
		case READLOC_COL: /* 'read locally' */
			return inconContainer.icon(IconContainer::ICON_READCOL);
			break;
		case ATTDOWN_COL: /* 'is downloaded' */
			return inconContainer.icon(IconContainer::ICON_ATTACHMENT);
			break;
		case PROCSNG_COL: /* 'process status' */
			return inconContainer.icon(IconContainer::ICON_FLAG);
			break;
		default:
			return _headerData(section, orientation, role);
			break;
		}
		break;

	case Qt::ToolTipRole:
		switch (section) {
		case PERSDELIV_COL: /* 'personal delivery' */
		case READLOC_COL: /* 'read locally' */
		case ATTDOWN_COL: /* 'is downloaded' */
		case PROCSNG_COL: /* 'process status' */
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

void DbMsgsTblModel::assignData(const QList<MessageDb::RcvdEntry> &entryList,
    int appendedColsNum)
{
	if (Q_UNLIKELY(m_columnCount != MAX_COLNUM)) {
		Q_ASSERT(0);
		return;
	}
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	removeRows(0, rowCount());

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = RCVD_MODEL;
		//m_columnCount = MAX_COLNUM;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != RCVD_MODEL)) {
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
		row[PERSDELIV_COL] = entry.dmPersonalDelivery;
		row[ANNOT_COL] = entry.dmAnnotation;
		row[SENDER_COL] = entry.dmSender;
		row[DELIVERY_COL] = entry.dmDeliveryTime;
		row[ACCEPT_COL] = entry.dmAcceptanceTime;
		row[READLOC_COL] = entry.readLocally;
		row[ATTDOWN_COL] = entry.isDownloaded;
		row[PROCSNG_COL] = entry.processStatus;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void DbMsgsTblModel::assignData(const QList<MessageDb::SntEntry> &entryList,
    int appendedColsNum)
{
	if (Q_UNLIKELY(m_columnCount != MAX_COLNUM)) {
		Q_ASSERT(0);
		return;
	}
	if (Q_UNLIKELY(appendedColsNum < 0)) {
		Q_ASSERT(0);
		return;
	}

	removeRows(0, rowCount());

	/* Set column count if the model is empty. */
	if (rowCount() == 0) {
		beginResetModel();
		m_type = SNT_MODEL;
		//m_columnCount = MAX_COLNUM;
		endResetModel();
	} else {
		if (Q_UNLIKELY(m_type != SNT_MODEL)) {
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
		row[RECIP_COL] = entry.dmRecipient;
		row[DELIVERY_COL] = entry.dmDeliveryTime;
		row[ACCEPT_COL] = entry.dmAcceptanceTime;
		row[MSGSTAT_COL] = entry.dmMessageStatus;
		row[ATTDOWN_COL] = entry.isDownloaded;

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

	setHeaderData(DMID_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[0]).desc,
	    Qt::DisplayRole);
	setHeaderData(DMID_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[0]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(PERSDELIV_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[1]).desc,
	    Qt::DisplayRole);
	setHeaderData(PERSDELIV_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[1]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(ANNOT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[2]).desc,
	    Qt::DisplayRole);
	setHeaderData(ANNOT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[2]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(SENDER_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[3]).desc,
	    Qt::DisplayRole);
	setHeaderData(SENDER_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[3]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(RECIP_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[2]).desc,
	    Qt::DisplayRole);
	setHeaderData(RECIP_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[2]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(DELIVERY_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[4]).desc,
	    Qt::DisplayRole);
	setHeaderData(DELIVERY_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[4]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(ACCEPT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[5]).desc,
	    Qt::DisplayRole);
	setHeaderData(ACCEPT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::rcvdItemIds[5]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(READLOC_COL, Qt::Horizontal,
	    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[6]).desc,
	    Qt::DisplayRole);
	setHeaderData(READLOC_COL, Qt::Horizontal,
	    smsgdtTbl.attrProps.value(MessageDb::rcvdItemIds[6]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(MSGSTAT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[5]).desc,
	    Qt::DisplayRole);
	setHeaderData(MSGSTAT_COL, Qt::Horizontal,
	    msgsTbl.attrProps.value(MessageDb::sntItemIds[5]).type,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(ATTDOWN_COL, Qt::Horizontal,
	    tr("Attachments downloaded"), Qt::DisplayRole);
	setHeaderData(ATTDOWN_COL, Qt::Horizontal, DB_BOOLEAN,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	setHeaderData(PROCSNG_COL, Qt::Horizontal, tr("Processing state"),
	    Qt::DisplayRole);
	setHeaderData(PROCSNG_COL, Qt::Horizontal, DB_INTEGER,
	    ROLE_MSGS_DB_ENTRY_TYPE);

	appendHeaderColumns(this, BASIC_COLNUM, appendedCols);

	return true;
}

bool DbMsgsTblModel::overrideRead(qint64 dmId, bool forceRead)
{
	if (RCVD_MODEL != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() == dmId) {
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
	if (RCVD_MODEL != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() == dmId) {
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
	if (RCVD_MODEL != m_type) {
		/* Not a read messages model. */
		return false;
	}

	for (int row = 0; row < rowCount(); ++row) {
		if (_data(row, DMID_COL, Qt::DisplayRole).toLongLong() == dmId) {
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
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		m_rmIco = defaultRMIcon();
		return false;
	}

	RecordsManagementDb::ServiceInfoEntry entry(
	    GlobInstcs::recMgmtDbPtr->serviceInfo());
	if (!entry.isValid() || entry.logoSvg.isEmpty()) {
		m_rmIco = defaultRMIcon();
		return false;
	}
	QPixmap pixmap16(Graphics::pixmapFromSvg(entry.logoSvg, 16));
	QPixmap pixmap32(Graphics::pixmapFromSvg(entry.logoSvg, 32));
	if (pixmap16.isNull() || pixmap32.isNull()) {
		m_rmIco = defaultRMIcon();
		return false;
	}

	QIcon ico;
	ico.addPixmap(pixmap16, QIcon::Normal, QIcon::Off);
	ico.addPixmap(pixmap32, QIcon::Normal, QIcon::Off);
	m_rmIco = ico;
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
		return locations.isEmpty() ? QVariant() : m_rmIco;
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
