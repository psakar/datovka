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
#include <QIcon>
#include <QMimeData>
#include <QRegularExpression>

#include "src/common.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"

/*
 * For index navigation QModellIndex::internalId() is used. The value encodes
 * the index of the top account node (which is also the index into the array
 * of user names) and the actual node type.
 *
 * The 4 least significant bits hold the node type.
 */
#define TYPE_BITS 4
#define TYPE_MASK 0x0f

/*!
 * @brief Generates model index internal identifier.
 *
 * @param[in] uNameIdx Index into the list of user names.
 * @param[in] nodeType Node type.
 * @return Internal identifier.
 */
#define internalIdCreate(uNameIdx, nodeType) \
	((((quintptr) (uNameIdx)) << TYPE_BITS) | (nodeType))

/*!
 * @brief Change the node type in internal identifier.
 *
 * @param[in] intId    Internal identifier.
 * @param[in] nodeType New type to be set.
 * @return Internal identifier with new type.
 */
#define internalIdChangeType(intId, nodeType) \
	(((intId) & ~((quintptr) TYPE_MASK)) | (nodeType))

/*!
 * @brief Obtain node type from the internal identifier.
 *
 * @param[in] intId Internal identifier.
 * @return Node type.
 */
#define internalIdNodeType(intId) \
	((enum NodeType) ((intId) & TYPE_MASK))

/*!
 * @brief Obtain index into user name list from internal identifier.
 *
 * @param[in] indId Internal identifier.
 * @return Index into user name list.
 */
#define internalIdUserNameIndex(intId) \
	(((unsigned) (intId)) >> TYPE_BITS)

AccountModel::AccountModel(QObject *parent)
    : QAbstractItemModel(parent),
    m_userNames(),
    m_row2UserNameIdx(),
    m_countersMap()
{
	/* Automatically handle signalled changes. */
	connect(GlobInstcs::acntMapPtr, SIGNAL(accountDataChanged(QString)),
	    this, SLOT(handleAccountDataChange(QString)));
}

QModelIndex AccountModel::index(int row, int column,
    const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	enum NodeType type = nodeUnknown;

	/* Parent type. */
	if (!parent.isValid()) {
		type = nodeRoot;
	} else {
		type = nodeType(parent);
	}

	type = childNodeType(type, row); /* Child type. */

	if (nodeUnknown == type) {
		return QModelIndex();
	}

	quintptr internalId = 0;
	if (nodeAccountTop == type) {
		/* Set top node row and type. */
		Q_ASSERT(row < m_row2UserNameIdx.size());
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
		Q_ASSERT(!m_userNames.at(uNameIdx).isEmpty());
		internalId = internalIdCreate(uNameIdx, type);
	} else {
		/* Preserve top node row from parent, change type. */
		internalId = internalIdChangeType(parent.internalId(), type);
	}

	return createIndex(row, column, internalId);
}

QModelIndex AccountModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	quintptr internalId = index.internalId();

	/* Child type. */
	enum NodeType type = internalIdNodeType(internalId);

	int parentRow = -1;
	type = parentNodeType(type, &parentRow); /* Parent type. */

	if ((nodeUnknown == type) || (nodeRoot == type)) {
		return QModelIndex();
	}

	if (parentRow < 0) {
		Q_ASSERT(nodeAccountTop == type);
		/* Determine the row of the account top node. */
		const int uNameIdx = internalIdUserNameIndex(internalId);
		for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
			if (uNameIdx == m_row2UserNameIdx.at(row)) {
				parentRow = row;
				break;
			}
		}
	}

	Q_ASSERT(parentRow >= 0);

	/* Preserve top node row from child. */
	return createIndex(parentRow, 0,
	    internalIdChangeType(internalId, type));
}

int AccountModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		/* Root. */
		return m_row2UserNameIdx.size();
	}

	int rows = 0;

	switch (nodeType(parent)) {
	case nodeAccountTop:
		rows = 3;
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
		rows = 0;
		break;
	case nodeAll:
		rows = 2;
		break;
	case nodeReceived:
		{
			const QString uName(userName(parent));
			Q_ASSERT(!uName.isEmpty());
			rows = m_countersMap[uName].receivedGroups.size();
		}
		break;
	case nodeSent:
		{
			const QString uName(userName(parent));
			Q_ASSERT(!uName.isEmpty());
			rows = m_countersMap[uName].sentGroups.size();
		}
		break;
	default:
		rows = 0;
		break;
	}

	return rows;
}

int AccountModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant AccountModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const QString uName(userName(index));
	if (Q_UNLIKELY(uName.isEmpty())) {
		Q_ASSERT(0);
		return QVariant();
	}
	const AcntSettings &accountInfo((*GlobInstcs::acntMapPtr)[uName]);
	const enum NodeType type = internalIdNodeType(index.internalId());

	switch (role) {
	case Qt::DisplayRole:
		switch (type) {
		case nodeAccountTop:
			return accountInfo.accountName();
			break;
		case nodeRecentReceived:
			{
				QString label(tr("Recent Received"));
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				if (cntrs.unreadRecentReceived > 0) {
					label += QString(" (%1)").arg(
					    cntrs.unreadRecentReceived);
				}
				return label;
			}
			break;
		case nodeRecentSent:
			return tr("Recent Sent");
			break;
		case nodeAll:
			return tr("All");
			break;
		case nodeReceived:
			return tr("Received");
			break;
		case nodeSent:
			return tr("Sent");
			break;
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				QString label(cntrs.receivedGroups[row]);
				Q_ASSERT(!label.isEmpty());
				Q_ASSERT(
				    cntrs.unreadReceivedGroups.find(label) !=
				    cntrs.unreadReceivedGroups.constEnd());
				if (cntrs.unreadReceivedGroups[label].unread > 0) {
					label += QString(" (%1)").arg(
					    cntrs.unreadReceivedGroups[label].unread);
				}
				return label;
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.sentGroups.size());
				return cntrs.sentGroups[row];
			}
			break;
		default:
			Q_ASSERT(0);
			return QVariant();
			break;
		}
		break;

	case Qt::DecorationRole:
		{
			QIcon ico;
			switch (type) {
			case nodeAccountTop:
				ico.addFile(QStringLiteral(ICON_3PARTY_PATH "letter_16.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_3PARTY_PATH "letter_32.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
				break;
			case nodeRecentReceived:
			case nodeReceived:
			case nodeReceivedYear:
				ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
				break;
			case nodeRecentSent:
			case nodeSent:
			case nodeSentYear:
				ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
				ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
				return ico;
				break;
			default:
				return QVariant();
				break;
			}
		}
		break;

	case Qt::FontRole:
		switch (type) {
		case nodeAccountTop:
			{
				QFont retFont;
				retFont.setBold(true);
				return retFont;
			}
			break;
		case nodeRecentReceived:
			{
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				if (cntrs.unreadRecentReceived > 0) {
					QFont retFont;
					retFont.setBold(true);
					return retFont;
				}
			}
			break;
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				const QString &label(cntrs.receivedGroups[row]);
				Q_ASSERT(!label.isEmpty());
				Q_ASSERT(
				    cntrs.unreadReceivedGroups.find(label) !=
				    cntrs.unreadReceivedGroups.constEnd());
				if (cntrs.unreadReceivedGroups[label].unread > 0) {
					QFont retFont;
					retFont.setBold(true);
					return retFont;
				}
			}
			break;
		default:
			return QVariant();
			break;
		}
		break;

	case Qt::ForegroundRole:
		switch (type) {
		case nodeReceivedYear:
			{
				int row = index.row();
				const AccountCounters &cntrs(m_countersMap[uName]);
				const QString &label(cntrs.receivedGroups[row]);
				if (!cntrs.unreadReceivedGroups[label].dbOpened) {
					return QColor(Qt::darkGray);
				} else {
					return QVariant();
				}
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				const AccountCounters &cntrs(m_countersMap[uName]);
				const QString &label(cntrs.sentGroups[row]);
				if (!cntrs.unreadSentGroups[label].dbOpened) {
					return QColor(Qt::darkGray);
				} else {
					return QVariant();
				}
			}
		default:
			return QVariant();
			break;
		}
		break;

	case Qt::AccessibleTextRole:
		switch (type) {
		case nodeAccountTop:
			return tr("account") + QLatin1String(" ") +
			    accountInfo.accountName();
			break;
		case nodeRecentReceived:
			{
				QString label(tr("recently received messages"));
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				if (cntrs.unreadRecentReceived > 0) {
					label += QLatin1String(" - ") +
					    tr("contains %1 unread")
					        .arg(cntrs.unreadRecentReceived);
				}
				return label;
			}
			break;
		case nodeRecentSent:
			return tr("recently sent messages");
			break;
		case nodeAll:
			return tr("all messages");
			break;
		case nodeReceived:
			return tr("all received messages");
			break;
		case nodeSent:
			return tr("all sent messages");
			break;
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				const QString &year(cntrs.receivedGroups[row]);
				if (year == MessageDb::invalidYearName) {
					return tr("invalid received messages");
				}
				QString label(
				    tr("messages received in year %1")
				        .arg(year));
				Q_ASSERT(!label.isEmpty());
				Q_ASSERT(
				    cntrs.unreadReceivedGroups.find(year) !=
				    cntrs.unreadReceivedGroups.constEnd());
				if (cntrs.unreadReceivedGroups[year].unread > 0) {
					label += QLatin1String(" - ") +
					    tr("contains %1 unread")
					        .arg(cntrs.unreadReceivedGroups[year].unread);
				}
				return label;
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.sentGroups.size());
				if (cntrs.sentGroups[row] == MessageDb::invalidYearName) {
					return tr("invalid sent messages");
				}
				return tr("messages sent in year %1")
				    .arg(cntrs.sentGroups[row]);
			}
			break;
		default:
			return QVariant();
			break;
		}
		break;

	case ROLE_PLAIN_DISPLAY:
		switch (type) {
		case nodeReceivedYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.receivedGroups.size());
				return cntrs.receivedGroups[row];
			}
			break;
		case nodeSentYear:
			{
				int row = index.row();
				Q_ASSERT(row >= 0);
				Q_ASSERT(m_countersMap.find(uName) !=
				    m_countersMap.constEnd());
				const AccountCounters &cntrs(
				    m_countersMap[uName]);
				Q_ASSERT(row < cntrs.sentGroups.size());
				return cntrs.sentGroups[row];
			}
			break;
		default:
			return QVariant();
			break;
		}
		break;

	default:
		return QVariant();
		break;
	}

	return QVariant(); /* Is this really necessary? */
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	if ((Qt::Horizontal == orientation) && (Qt::DisplayRole == role) &&
	    (0 == section)) {
		return tr("Accounts");
	}

	return QVariant();
}

bool AccountModel::moveRows(const QModelIndex &sourceParent, int sourceRow,
    int count, const QModelIndex &destinationParent, int destinationChild)
{
	if (sourceParent.isValid() || destinationParent.isValid()) {
		/* Only moves within root node are allowed. */
		return false;
	}

	if ((sourceRow < 0) || (sourceRow >= rowCount()) ||
	    (count < 0) || ((sourceRow + count) > rowCount()) ||
	    (destinationChild < 0) || (destinationChild > rowCount())) {
		/* Boundaries or location outside limits. */
		return false;
	}

	if (count == 0) {
		return true;
	}

	if ((sourceRow <= destinationChild) &&
	    (destinationChild <= (sourceRow + count))) {
		/* Destination lies within source. */
		return true;
	}

	int newPosition; /* Position after removal. */
	if (destinationChild < sourceRow) {
		newPosition = destinationChild;
	} else if (destinationChild > (sourceRow + count)) {
		newPosition = destinationChild - count;
	} else {
		Q_ASSERT(0);
		return false;
	}

	beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1,
	    destinationParent, destinationChild);

	QList<int> movedData;
	for (int row = sourceRow; row < (sourceRow + count); ++row) {
		movedData.append(m_row2UserNameIdx.takeAt(sourceRow));
	}

	for (int i = 0; i < count; ++i) {
		m_row2UserNameIdx.insert(newPosition + i, movedData.takeAt(0));
	}
	Q_ASSERT(movedData.isEmpty());

	endMoveRows();

	return false;
}

bool AccountModel::removeRows(int row, int count, const QModelIndex &parent)
{
	Q_UNUSED(row);
	Q_UNUSED(count);
	Q_UNUSED(parent);

	return false;
}

Qt::DropActions AccountModel::supportedDropActions(void) const
{
	/* The model must provide removeRows() to be able to use move action. */
	return Qt::MoveAction;
}

Qt::ItemFlags AccountModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

	if (index.isValid()) {
		if (nodeAccountTop == nodeType(index)) {
			/* Allow drags on account top entries. */
			defaultFlags |= Qt::ItemIsDragEnabled;
		}
	} else {
		defaultFlags |= Qt::ItemIsDropEnabled;
	}

	return defaultFlags;
}

#define itemDataListMimeName \
	QLatin1String("application/x-qabstractitemmodeldatalist")

/* Custom mime type. */
#define itemIndexRowListMimeName \
	QLatin1String("application/x-qitemindexrowlist")

QStringList AccountModel::mimeTypes(void) const
{
	return QStringList(itemIndexRowListMimeName);
}

QMimeData *AccountModel::mimeData(const QModelIndexList &indexes) const
{
	if (indexes.isEmpty()) {
		return Q_NULLPTR;
	}

	QMimeData *mimeData = new (std::nothrow) QMimeData;
	if (Q_UNLIKELY(Q_NULLPTR == mimeData)) {
		return Q_NULLPTR;
	}

	/*
	 * TODO -- In order to encompass full account description then
	 * conversion to QVariant is needed.
	 * See QMimeData::setData() documentation.
	 */

	/* Convert row numbers into mime data. */
	QByteArray data;
	foreach (const QModelIndex &idx, indexes) {
		if (idx.isValid()) {
			qint64 row = idx.row();
			data.append((const char *)&row, sizeof(row));
		}
	}

	mimeData->setData(itemIndexRowListMimeName, data);

	return mimeData;
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 4, 1))
/*
 * There are documented bugs with regard to
 * QAbstractItemModel::canDropMimeData() in Qt prior to version 5.4.1.
 * See QTBUG-32362 and QTBUG-30534.
 */
#warning "Compiling against version < Qt-5.4.1 which may have bugs around QAbstractItemModel::dropMimeData()."
#endif /* < Qt-5.4.1 */

bool AccountModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
    int row, int column, const QModelIndex &parent) const
{
	Q_UNUSED(row);
	Q_UNUSED(column);

	if ((Q_NULLPTR == data) || (Qt::MoveAction != action) ||
	    parent.isValid()) {
		return false;
	}

	return data->hasFormat(itemIndexRowListMimeName);
}

bool AccountModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
    int row, int column, const QModelIndex &parent)
{
	if (!canDropMimeData(data, action, row, column, parent)) {
		return false;
	}

	if (row < 0) {
		/*
		 * Dropping onto root node. This usually occurs when dropping
		 * on empty space behind last entry. Treat as dropping past
		 * last element.
		 */
		row = rowCount();
	}

	QList<int> droppedRows;
	{
		/* Convert mime data into row numbers. */
		QByteArray bytes(data->data(itemIndexRowListMimeName));
		qint64 row;
		const char *constBytes = bytes.constData();
		for (int offs = 0; offs < bytes.size(); offs += sizeof(row)) {
			row = *(qint64 *)(constBytes + offs);
			droppedRows.append(row);
		}
	}

	if (Q_UNLIKELY(droppedRows.isEmpty())) {
		logErrorNL("%s", "Got drop with no entries.");
		Q_ASSERT(0);
		return false;
	}

	if (droppedRows.size() > 1) {
		logWarningNL("%s",
		    "Cannot process drops of multiple entries at once.");
		return false;
	}

	moveRows(QModelIndex(), droppedRows.at(0), 1, parent, row);

	return false; /* If false is returned then removeRows() won't be triggered. */
}

void AccountModel::loadFromSettings(const QString &confDir,
    const QSettings &settings)
{
	/* Load into global account settings. */
	GlobInstcs::acntMapPtr->loadFromSettings(confDir, settings);

	QStringList groups = settings.childGroups();
	QRegExp credRe(CredNames::creds + ".*");

	/* Clear present rows. */
	this->removeRows(0, this->rowCount());

	QStringList credetialList;
	/* Get list of credentials. */
	for (int i = 0; i < groups.size(); ++i) {
		/* Matches regular expression. */
		if (credRe.exactMatch(groups.at(i))) {
			credetialList.append(groups.at(i));
		}
	}

	/* Sort the credentials list. */
	qSort(credetialList.begin(), credetialList.end(),
	    AcntSettings::credentialsLessThan);

	beginResetModel();

	m_userNames.clear();
	m_row2UserNameIdx.clear();
	m_countersMap.clear();

	/* For all credentials. */
	foreach(const QString &group, credetialList) {
		const QString userName(settings.value(group + "/" + CredNames::userName,
		    QString()).toString());

		/* Add user name into the model. */
		m_countersMap[userName] = AccountCounters();

		m_userNames.append(userName);
		m_row2UserNameIdx.append(m_userNames.size() - 1);
	}

	endResetModel();
}

void AccountModel::saveToSettings(const QString &pinVal, const QString &confDir,
    QSettings &settings) const
{
	QString groupName;

	for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
		const QString &userName(m_userNames.at(uNameIdx));
		const AcntSettings &itemSettings(
		    (*GlobInstcs::acntMapPtr)[userName]);

		Q_ASSERT(userName == itemSettings.userName());

		groupName = CredNames::creds;
		if (row > 0) {
			groupName.append(QString::number(row + 1));
		}

		itemSettings.saveToSettings(pinVal, confDir, settings,
		    groupName);
	}
}

int AccountModel::addAccount(const AcntSettings &acntSettings, QModelIndex *idx)
{
	const QString userName(acntSettings.userName());

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return -1;
	}

	if (GlobInstcs::acntMapPtr->find(userName) !=
	    GlobInstcs::acntMapPtr->end()) {
		logErrorNL("Account with user name '%s' already exists.",
		    userName.toUtf8().constData());
		return -2;
	}

	Q_ASSERT(!m_userNames.contains(userName));

	beginInsertRows(QModelIndex(), rowCount(), rowCount());

	(*GlobInstcs::acntMapPtr)[userName] = acntSettings;

	m_countersMap[userName] = AccountCounters();

	m_userNames.append(userName);
	m_row2UserNameIdx.append(m_userNames.size() - 1);

	endInsertRows();

	if (Q_NULLPTR != idx) {
		*idx = index(m_row2UserNameIdx.size() - 1, 0, QModelIndex());
	}

	return 0;
}

void AccountModel::deleteAccount(const QString &userName)
{
	int row = topAcntRow(userName);

	if (row < 0) {
		Q_ASSERT(0);
		return;
	}

	int uNameIdx = m_row2UserNameIdx.at(row);
	Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));
	Q_ASSERT(userName == m_userNames.at(uNameIdx));

	beginRemoveRows(QModelIndex(), row, row);

	m_userNames[uNameIdx] = QString();
	m_row2UserNameIdx.removeAt(row);

	m_countersMap.remove(userName);

	GlobInstcs::acntMapPtr->remove(userName);

	endRemoveRows();
}

QString AccountModel::userName(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QString();
	}

	int uNameIdx = internalIdUserNameIndex(index.internalId());

	if (uNameIdx < m_userNames.size()) {
		return m_userNames.at(uNameIdx);
	} else {
		return QString();
	}
}

QModelIndex AccountModel::topAcntIndex(const QString &userName) const
{
	int row = topAcntRow(userName);
	if (row >= 0) {
		return index(row, 0);
	}

	return QModelIndex();
}

enum AccountModel::NodeType AccountModel::nodeType(const QModelIndex &index)
{
	if (!index.isValid()) {
		return nodeUnknown; /* nodeRoot ? */
	}

	/* TODO -- Add runtime value check? */
	return internalIdNodeType(index.internalId());
}

bool AccountModel::nodeTypeIsReceived(const QModelIndex &index)
{
	switch (nodeType(index)) {
	case nodeRecentReceived:
	case nodeReceived:
	case nodeReceivedYear:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool AccountModel::nodeTypeIsSent(const QModelIndex &index)
{
	switch (nodeType(index)) {
	case nodeRecentSent:
	case nodeSent:
	case nodeSentYear:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool AccountModel::updateRecentUnread(const QString &userName,
    enum AccountModel::NodeType nodeType, unsigned unreadMsgs)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	unsigned *unreadRecent = 0;
	QModelIndex childIndex;

	if (nodeRecentReceived == nodeType) {
		unreadRecent = &cntrs.unreadRecentReceived;
		/* Get recently received node. */
		childIndex = topIndex.child(0, 0);
	} else if (nodeRecentSent == nodeType) {
		unreadRecent = &cntrs.unreadRecentSent;
		/* Get recently sent node. */
		childIndex = topIndex.child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(0 != unreadRecent);
	Q_ASSERT(childIndex.isValid());

	*unreadRecent = unreadMsgs;
	emit dataChanged(childIndex, childIndex);

	return true;
}

/*!
 * @brief Get position of the newly inserted year element.
 *
 * @param[in] yearList  List of years.
 * @param[in] addedYear Year To be added.
 * @param[in] sorting   Sorting.
 * @return Position of new element, -1 on any error.
 */
static
int addedYearPosistion(const QList<QString> &yearList, const QString &addedYear,
    enum AccountModel::Sorting sorting)
{
	static QRegularExpression yearRe("^[0-9][0-9][0-9][0-9]$");

	if (yearList.isEmpty()) {
		return 0;
	}

	bool addedIsYear = yearRe.match(addedYear).hasMatch();

	switch (sorting) {
	case AccountModel::UNSORTED:
		/* Just append. */
		return yearList.size();
		break;
	case AccountModel::ASCENDING:
		for (int pos = 0; pos < yearList.size(); ++pos) {
			const QString &yearEntry(yearList.at(pos));
			bool entryIsYear = yearRe.match(yearEntry).hasMatch();
			if (addedIsYear == entryIsYear) {
				if (addedYear < yearEntry) {
					return pos;
				}
			} else if (addedIsYear) {
				return pos; /* Years first. */
			}
		}
		return yearList.size();
		break;
	case AccountModel::DESCENDING:
		for (int pos = 0; pos < yearList.size(); ++pos) {
			const QString &yearEntry(yearList.at(pos));
			bool entryIsYear = yearRe.match(yearEntry).hasMatch();
			if (addedIsYear == entryIsYear) {
				if (addedYear > yearEntry) {
					return pos;
				}
			} else if (addedIsYear) {
				return pos; /* Years first. */
			}
		}
		return yearList.size();
		break;
	default:
		Q_ASSERT(0);
		return -1;
		break;
	}
}

bool AccountModel::updateYearNodes(const QString &userName,
    enum AccountModel::NodeType nodeType,
    const QList< QPair<QString, YearCounter> > &yearlyUnreadList,
    enum AccountModel::Sorting sorting, bool prohibitYearRemoval)
{
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	QList<QString> *groups = Q_NULLPTR;
	QMap<QString, YearCounter> *unreadGroups = Q_NULLPTR;
	QModelIndex childTopIndex;

	{
		QModelIndex topIndex(topAcntIndex(userName));
		if (!topIndex.isValid()) {
			return false;
		}

		if (nodeReceivedYear == nodeType) {
			groups = &cntrs.receivedGroups;
			unreadGroups = &cntrs.unreadReceivedGroups;
			/* Get received node. */
			childTopIndex = topIndex.child(2, 0).child(0, 0);
		} else if (nodeSentYear == nodeType) {
			groups = &cntrs.sentGroups;
			unreadGroups = &cntrs.unreadSentGroups;
			/* Get sent node. */
			childTopIndex = topIndex.child(2, 0).child(1, 0);
		} else {
			Q_ASSERT(0);
			return false;
		}
	}
	Q_ASSERT(Q_NULLPTR != groups);
	Q_ASSERT(Q_NULLPTR != unreadGroups);
	Q_ASSERT(childTopIndex.isValid());

	/*
	 * Delete model elements that don't exist in new list or update
	 * existing entries.
	 */
	int row = groups->size() - 1;
	typedef QPair<QString, YearCounter> Pair;
	while (row >= 0) { /* In reverse order. */
		bool found = false;
		YearCounter yCounter;
		const QString &groupName(groups->at(row));
		foreach (const Pair &pair, yearlyUnreadList) {
			if (pair.first == groupName) {
				found = true;
				yCounter = pair.second;
				break;
			}
		}
		if (!prohibitYearRemoval && !found) {
			/* Remove row, don't increment row. */
			beginRemoveRows(childTopIndex, row, row);
			unreadGroups->remove(groupName);
			groups->removeAt(row);
			endRemoveRows();
		} else {
			/* Update unread, increment row. */
			(*unreadGroups)[groupName] = yCounter;
			QModelIndex childIndex(childTopIndex.child(row, 0));
			emit dataChanged(childIndex, childIndex);
		}
		--row;
	}

	/* Add missing elements. */
	foreach (const Pair &pair, yearlyUnreadList) {
		if (!groups->contains(pair.first)) {
			int newRow = addedYearPosistion(*groups, pair.first,
			    sorting);
			if (Q_UNLIKELY(newRow < 0)) {
				Q_ASSERT(0);
				return false;
			}
			beginInsertRows(childTopIndex, newRow, newRow);
			groups->insert(newRow, pair.first);
			(*unreadGroups)[pair.first] = pair.second;
			endInsertRows();
		}
	}

	return true;
}

bool AccountModel::updateYear(const QString &userName,
    enum AccountModel::NodeType nodeType, const QString &year,
    const YearCounter &yCounter)
{
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QModelIndex topIndex(topAcntIndex(userName));
	if (!topIndex.isValid()) {
		return false;
	}

	AccountCounters &cntrs(m_countersMap[userName]);
	QList<QString> *groups = Q_NULLPTR;
	QMap<QString, YearCounter> *unreadGroups = Q_NULLPTR;
	QModelIndex childIndex; /* Top index of children. */

	if (nodeReceivedYear == nodeType) {
		groups = &cntrs.receivedGroups;
		unreadGroups = &cntrs.unreadReceivedGroups;
		/* Get received node. */
		childIndex = topIndex.child(2, 0).child(0, 0);
	} else if (nodeSentYear == nodeType) {
		groups = &cntrs.sentGroups;
		unreadGroups = &cntrs.unreadSentGroups;
		/* Get sent node. */
		childIndex = topIndex.child(2, 0).child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	Q_ASSERT(Q_NULLPTR != groups);
	Q_ASSERT(Q_NULLPTR != unreadGroups);
	Q_ASSERT(childIndex.isValid());

	int row = 0;
	for (; row < groups->size(); ++row) {
		if (year == groups->at(row)) {
			break;
		}
	}
	if (row >= groups->size()) {
		return false;
	}
	childIndex = childIndex.child(row, 0); /* Child index. */

	Q_ASSERT(childIndex.isValid());

	(*unreadGroups)[year] = yCounter;
	emit dataChanged(childIndex, childIndex);

	return true;
}

void AccountModel::removeAllYearNodes(void)
{
	for (int row = 0; row < rowCount(); ++row) {
		QModelIndex topIndex(index(row, 0));

		Q_ASSERT(topIndex.isValid());

		removeYearNodes(topIndex);
	}
}

bool AccountModel::changePosition(const QString &userName, int shunt)
{
	if (0 == shunt) {
		return false;
	}

	int row = topAcntRow(userName);
	if ((row < 0) || (row >= m_row2UserNameIdx.size())) {
		Q_ASSERT(0);
		return false;
	}
	int newRow = row + shunt;
	if ((newRow < 0) || (newRow >= m_row2UserNameIdx.size())) {
		return false;
	}

	int destChild = newRow;

	if (shunt > 0) {
		/*
		 * Because we move one item.
		 * See QAbstractItemModel::beginMoveRows() documentation.
		 */
		destChild += 1;
	}

	beginMoveRows(QModelIndex(), row, row, QModelIndex(), destChild);

	int auxNameIdx = m_row2UserNameIdx.at(row);
	m_row2UserNameIdx[row] = m_row2UserNameIdx.at(newRow);
	m_row2UserNameIdx[newRow] = auxNameIdx;

	endMoveRows();

	return true;
}

void AccountModel::handleAccountDataChange(const QString &userName)
{
	QModelIndex topIndex(topAcntIndex(userName));

	if (topIndex.isValid()) {
		emit dataChanged(topIndex, topIndex);
	}
}

void AccountModel::removeYearNodes(const QModelIndex &topIndex)
{
	Q_ASSERT(topIndex.isValid());

	const QString uName(userName(topIndex));
	Q_ASSERT(!uName.isEmpty());

	AccountCounters &cntrs(m_countersMap[uName]);

	/* Received. */
	QModelIndex childTopIndex = topIndex.child(2, 0).child(0, 0);
	int rows = rowCount(childTopIndex);
	if (rows > 0) {
		beginRemoveRows(childTopIndex, 0, rows - 1);
		cntrs.receivedGroups.clear();
		cntrs.unreadReceivedGroups.clear();
		endRemoveRows();
	}

	/* Sent. */
	childTopIndex = topIndex.child(2, 0).child(1, 0);
	rows = rowCount(childTopIndex);
	if (rows > 0) {
		beginRemoveRows(childTopIndex, 0, rows - 1);
		cntrs.sentGroups.clear();
		cntrs.unreadSentGroups.clear();
		endRemoveRows();
	}
}

enum AccountModel::NodeType AccountModel::childNodeType(
    enum AccountModel::NodeType parentType, int childRow)
{
	switch (parentType) {
	case nodeUnknown:
		return nodeUnknown;
		break;
	case nodeRoot:
		return nodeAccountTop;
		break;
	case nodeAccountTop:
		switch (childRow) {
		case 0:
			return nodeRecentReceived;
			break;
		case 1:
			return nodeRecentSent;
			break;
		case 2:
			return nodeAll;
			break;
		default:
			return nodeUnknown;
			break;
		}
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
		return nodeUnknown;
		break;
	case nodeAll:
		switch (childRow) {
		case 0:
			return nodeReceived;
			break;
		case 1:
			return nodeSent;
			break;
		default:
			return nodeUnknown;
			break;
		}
		break;
	case nodeReceived:
		return nodeReceivedYear;
		break;
	case nodeSent:
		return nodeSentYear;
		break;
	case nodeReceivedYear:
	case nodeSentYear:
		return nodeUnknown;
		break;
	default:
		Q_ASSERT(0);
		return nodeUnknown;
		break;
	}
}

enum AccountModel::NodeType AccountModel::parentNodeType(
    enum AccountModel::NodeType childType, int *parentRow)
{
	switch (childType) {
	case nodeUnknown:
	case nodeRoot:
		if (Q_NULLPTR != parentRow) {
			*parentRow = -1;
		}
		return nodeUnknown;
		break;
	case nodeAccountTop:
		if (Q_NULLPTR != parentRow) {
			*parentRow = -1;
		}
		return nodeRoot;
		break;
	case nodeRecentReceived:
	case nodeRecentSent:
	case nodeAll:
		if (Q_NULLPTR != parentRow) {
			*parentRow = -1;
		}
		return nodeAccountTop;
		break;
	case nodeReceived:
	case nodeSent:
		if (Q_NULLPTR != parentRow) {
			*parentRow = 2;
		}
		return nodeAll;
		break;
	case nodeReceivedYear:
		if (Q_NULLPTR != parentRow) {
			*parentRow = 0;
		}
		return nodeReceived;
		break;
	case nodeSentYear:
		if (Q_NULLPTR != parentRow) {
			*parentRow = 1;
		}
		return nodeSent;
		break;
	default:
		Q_ASSERT(0);
		if (Q_NULLPTR != parentRow) {
			*parentRow = -1;
		}
		return nodeUnknown;
		break;
	}
}

int AccountModel::topAcntRow(const QString &userName) const
{
	int foundRow = -1;

	for (int row = 0; row < m_row2UserNameIdx.size(); ++row) {
		const int uNameIdx = m_row2UserNameIdx.at(row);
		Q_ASSERT((uNameIdx >= 0) && (uNameIdx < m_userNames.size()));

		if (userName == m_userNames.at(uNameIdx)) {
			foundRow = row;
			break;
		}
	}

	return foundRow;
}
