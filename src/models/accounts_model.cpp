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


#include <QtAlgorithms> /* qSort() */
#include <QDebug>
#include <QRegExp>

#include "accounts_model.h"
#include "src/common.h"
#include "src/log/log.h"


/* ========================================================================= */
AccountModel::SettingsMap::SettingsMap(void)
/* ========================================================================= */
    : QMap<QString, QVariant>()
{
}


/* ========================================================================= */
AccountModel::SettingsMap::SettingsMap(const QMap<QString, QVariant> &map)
/* ========================================================================= */
    : QMap<QString, QVariant>(map)
{
}


/* ========================================================================= */
void AccountModel::SettingsMap::setDbDir(const QString &path)
/* ========================================================================= */
{
	if (path == globPref.confDir()) {
		/* Default path is empty. */
		QMap<QString, QVariant>::operator[](DB_DIR) = QString();
	} else {
		QMap<QString, QVariant>::operator[](DB_DIR) = path;
	}
}


QMap<QString, AccountModel::SettingsMap> AccountModel::globAccounts;


/* ========================================================================= */
/*
 * Empty account model constructor.
 */
AccountModel::AccountModel(QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(parent)
{
	/* Set header. */
	this->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("Accounts")));
}


/* ========================================================================= */
/*
 * Compute viewed data.
 */
QVariant AccountModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
	QVariant storedData;
	switch (role) {
	case Qt::DisplayRole:
		storedData = QStandardItemModel::data(index,
		    ROLE_ACNT_UNREAD_MSGS);
		if (storedData.isValid()) {
			QString retStr = QStandardItemModel::data(index,
			    role).toString() + " (" +
			    QString::number(storedData.toInt()) + ")";
			return retStr;
		}

		return QStandardItemModel::data(index, role);
		break;

#if 0
	case Qt::DecorationRole:
		if (nodeAccountTop == nodeType(index)) {
			const QString userName =
			    index.data(ROLE_ACNT_USER_NAME).toString();
			qDebug() << "A001" << globPref.confDir();
			if (!globAccounts[userName].dbDir().isEmpty()) {
				return QIcon(ICON_16x16_PATH "grey.png");
			}
		}
		return QStandardItemModel::data(index, role);
		break;
#endif

	case Qt::FontRole:
		if (nodeAccountTop == nodeType(index)) {
			QFont retFont;
			retFont.setBold(true);
			return retFont;
		}

		storedData = QStandardItemModel::data(index,
		    ROLE_ACNT_UNREAD_MSGS);
		if (storedData.isValid()) {
			QFont retFont;
			retFont.setBold(true);
			return retFont;
		}

		return QStandardItemModel::data(index, role);
		break;

	default:
		return QStandardItemModel::data(index, role);
		break;
	}
}


/* ========================================================================= */
/*!
 * @brief Used for sorting credentials.
 *
 * @param[in] s1  credentials[0-9]*
 * @param[in] s2  credentials[0-9]*
 * @return True if s1 comes before s2.
 *
 * @note The number is taken by its value rather like a string of characters.
 * cred < cred1 < cred2 < ... < cred10 < ... < cred100 < ...
 */
static
bool credentialsLessThan(const QString &s1, const QString &s2)
/* ========================================================================= */
{
	QRegExp trailingNumRe("(.*[^0-9]+)*([0-9]+)");
	QString a1, a2;
	int n1, n2;
	int pos;

	pos = trailingNumRe.indexIn(s1);
	if (pos > -1) {
		a1 = trailingNumRe.cap(1);
		n1 = trailingNumRe.cap(2).toInt();
	} else {
		a1 = s1;
		n1 = -1;
	}

	pos = trailingNumRe.indexIn(s2);
	if (pos > -1) {
		a2 = trailingNumRe.cap(1);
		n2 = trailingNumRe.cap(2).toInt();
	} else {
		a2 = s2;
		n2 = -1;
	}

	return (a1 != a2) ? (a1 < a2) : (n1 < n2);
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void AccountModel::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	debugFuncCall();

	QStringList groups = settings.childGroups();
	QRegExp credRe(CREDENTIALS".*");
	SettingsMap itemSettings;

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
	qSort(credetialList.begin(), credetialList.end(), credentialsLessThan);

	/* For all credentials. */
	foreach(QString group, credetialList) {
		itemSettings.clear();
		/*
		 * String containing comma character are loaded as
		 * a string list.
		 *
		 * FIXME -- Any white-space characters trailing
		 * the comma are lost.
		 */
		itemSettings.setAccountName(
		    settings.value(group + "/" + ACCOUNT_NAME,
		        "").toStringList().join(", "));
		itemSettings.setUserName(
		    settings.value(group + "/" + USER,
		        "").toString());
		itemSettings.setLoginMethod(
		    settings.value(group + "/" + LOGIN,
		    "").toString());
		itemSettings.setPassword(fromBase64(
		    settings.value(group + "/" + PWD,
		        "").toString()));
		itemSettings.setTestAccount(
		    settings.value(group + "/" + TEST_ACCOUNT,
		        "").toBool());
		itemSettings.setRememberPwd(
		    settings.value(group + "/" + REMEMBER_PWD,
		        "").toBool());
		itemSettings.setDbDir(
		    settings.value(group + "/" + DB_DIR,
		        "").toString());
		itemSettings.setSyncWithAll(
		    settings.value(group + "/" + SYNC_WITH_ALL,
		        "").toBool());
		itemSettings.setP12File(
		    settings.value(group + "/" + P12FILE,
		        "").toString());
		itemSettings.setLastMsg(
		    settings.value(group + "/" + LAST_MSG_ID,
		        "").toLongLong());
		itemSettings.setLastAttachSavePath(
		    settings.value(group + "/" + LAST_SAVE_ATTACH,
		        "").toString());
		itemSettings.setLastAttachAddPath(
		    settings.value(group + "/" + LAST_ADD_ATTACH,
		        "").toString());
		itemSettings.setLastCorrespPath(
		    settings.value(group + "/" + LAST_CORRESPOND,
		        "").toString());
		itemSettings.setLastZFOExportPath(
		    settings.value(group + "/" + LAST_ZFO,
		        "").toString());

		/* Associate map with item node. */
		addAccount(itemSettings.accountName(), itemSettings);
	}
}


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void AccountModel::saveToSettings(QSettings &settings) const
/* ========================================================================= */
{
	QString groupName;

	for (int i = 0; i < this->rowCount(); ++i) {
		const QString userName =
		    this->item(i)->data(ROLE_ACNT_USER_NAME).toString();
		const SettingsMap &itemSettings = globAccounts[userName];

		groupName = CREDENTIALS;
		if (i > 0) {
			groupName.append(QString::number(i + 1));
		}
		settings.beginGroup(groupName);

		settings.setValue(ACCOUNT_NAME, itemSettings.accountName());
		settings.setValue(USER, itemSettings.userName());
		settings.setValue(LOGIN, itemSettings.loginMethod());
		settings.setValue(TEST_ACCOUNT, itemSettings.isTestAccount());
		settings.setValue(REMEMBER_PWD, itemSettings.rememberPwd());
		if (itemSettings.rememberPwd()) {
			if (!itemSettings.password().isEmpty()) {
				settings.setValue(PWD,
				    toBase64(itemSettings.password()));
			}
		}

		if (!itemSettings.dbDir().isEmpty()) {
			if (itemSettings.dbDir() != globPref.confDir()) {
				settings.setValue(DB_DIR,
				    itemSettings.dbDir());
			}
		}
		if (!itemSettings.p12File().isEmpty()) {
			settings.setValue(P12FILE, itemSettings.p12File());
		}

		settings.setValue(SYNC_WITH_ALL, itemSettings.syncWithAll());

		if (0 <= itemSettings.lastMsg()) {
			settings.setValue(LAST_MSG_ID, itemSettings.lastMsg());
		}

		/* Save last attachments save path. */
		if (!itemSettings.lastAttachSavePath().isEmpty()) {
			settings.setValue(LAST_SAVE_ATTACH,
			    itemSettings.lastAttachSavePath());
		}

		/* Save last attachments add path. */
		if (!itemSettings.lastAttachAddPath().isEmpty()) {
			settings.setValue(LAST_ADD_ATTACH,
			    itemSettings.lastAttachAddPath());
		}

		/* Save last correspondence export path. */
		if (!itemSettings.lastCorrespPath().isEmpty()) {
			settings.setValue(LAST_CORRESPOND,
			    itemSettings.lastCorrespPath());
		}

		/* save last ZFO export path */
		if (!itemSettings.lastZFOExportPath().isEmpty()) {
			settings.setValue(LAST_ZFO,
			    itemSettings.lastZFOExportPath());
		}

		settings.endGroup();
	}
}


/* ========================================================================= */
/*
 * Add account.
 */
QModelIndex AccountModel::addAccount(const QString &name,
    const AccountModel::SettingsMap &settings)
/* ========================================================================= */
{
	/* Defining a couple of items. */
	QStandardItem *account = new QStandardItem(name);
	account->setFlags(account->flags() & ~Qt::ItemIsEditable);
	QStandardItem *recentReceived =
	    new QStandardItem(QObject::tr("Recent Received"));
	recentReceived->setFlags(recentReceived->flags() & ~Qt::ItemIsEditable);
	QStandardItem *recentSent = new QStandardItem(QObject::tr("Recent Sent"));
	recentSent->setFlags(recentSent->flags() & ~Qt::ItemIsEditable);
	QStandardItem *all = new QStandardItem(QObject::tr("All"));
	all->setFlags(all->flags() & ~Qt::ItemIsEditable);
	QStandardItem *allReceived = new QStandardItem(QObject::tr("Received"));
	allReceived->setFlags(allReceived->flags() & ~Qt::ItemIsEditable);
	QStandardItem *allSent = new QStandardItem(QObject::tr("Sent"));
	allSent->setFlags(allSent->flags() & ~Qt::ItemIsEditable);

	Q_ASSERT(!settings.userName().isEmpty());
	account->setData(settings.userName(), ROLE_ACNT_USER_NAME);
	globAccounts[settings.userName()] = settings;

	/* Account node is drawn bold in the data() method. */
	account->setIcon(QIcon(ICON_3PARTY_PATH + QString("letter_16.png")));

	recentReceived->setIcon(
	    QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));
	recentSent->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-reply.png")));
	allReceived->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-download.png")));
	allSent->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-reply.png")));

	/* Building up the hierarchy. */
	account->appendRow(recentReceived);
	account->appendRow(recentSent);
	account->appendRow(all);
	all->appendRow(allReceived);
	all->appendRow(allSent);

	invisibleRootItem()->appendRow(account);
	return account->index();
}


/* ========================================================================= */
/*!
 * @brief Returns node type.
 */
AccountModel::NodeType AccountModel::nodeType(const QModelIndex &index)
/* ========================================================================= */
{
	if (-1 == index.parent().row()) {
		return nodeAccountTop;
	} else if (-1 == index.parent().parent().row()) {
		switch (index.row()) {
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
	} else if (-1 == index.parent().parent().parent().row()) {
		switch (index.row()) {
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
	} else if (-1 == index.parent().parent().parent().parent().row()) {
		switch (index.parent().row()) {
		case 0:
			return nodeReceivedYear;
			break;
		case 1:
			return nodeSentYear;
			break;
		default:
			return nodeUnknown;
			break;
		}
	} else {
		return nodeUnknown;
	}
}


/* ========================================================================= */
/*
 * Returns true when node type is received.
 */
bool AccountModel::nodeTypeIsReceived(const QModelIndex &index)
/* ========================================================================= */
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

/* ========================================================================= */
/*
 * Returns true when node type is sent.
 */
bool AccountModel::nodeTypeIsSent(const QModelIndex &index)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 * Returns pointer to related top-most item.
 */
const QStandardItem * AccountModel::itemTop(const QStandardItem *item)
/* ========================================================================= */
{
	if (0 == item) {
		return 0;
	}

	while (0 != item->parent()) {
		item = item->parent();
	}

	return item;
}


/* ========================================================================= */
QStandardItem * AccountModel::itemTop(QStandardItem *item)
/* ========================================================================= */
{
	if (0 == item) {
		return 0;
	}

	while (0 != item->parent()) {
		item = item->parent();
	}

	return item;
}


/* ========================================================================= */
/*
 * Returns index to related top-most item.
 */
QModelIndex AccountModel::indexTop(const QModelIndex &index)
/* ========================================================================= */
{
	QModelIndex top = index;

	if (!top.isValid()) {
		return top;
	}

	while (top.parent().isValid()) {
		top = top.parent();
	}

	return top;
}


/* ========================================================================= */
/*
 * Return related settings map.
 */
AccountModel::SettingsMap AccountModel::settingsMap(QStandardItem *item)
/* ========================================================================= */
{
	Q_ASSERT(0 != item);
	QStandardItem *topItem = itemTop(item);

	Q_ASSERT(0 != topItem);
	const QString userName = topItem->data(ROLE_ACNT_USER_NAME).toString();
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(userName == globAccounts[userName].userName());
	return globAccounts[userName];
}


/* ========================================================================= */
/*
 * Set settings map to related account.
 */
void AccountModel::setSettingsMap(QStandardItem *item,
    const AccountModel::SettingsMap &map)
/* ========================================================================= */
{
	Q_ASSERT(0 != item);
	QStandardItem *topItem = itemTop(item);

	Q_ASSERT(0 != topItem);
	Q_ASSERT(!map.userName().isEmpty());
	globAccounts[map.userName()] = map;
	topItem->setData(map.userName(), ROLE_ACNT_USER_NAME);
}


/* ========================================================================= */
/*
 * Set number of unread messages in recent model nodes.
 */
bool AccountModel::updateRecentUnread(QStandardItem *item, NodeType nodeType,
    unsigned unreadMsgs)
/* ========================================================================= */
{
	/* Find account top. */
	item = itemTop(item);

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	if (nodeRecentReceived == nodeType) {
		/* Get recently received node. */
		item = item->child(0, 0);
	} else if (nodeRecentSent == nodeType) {
		/* Get recently sent node. */
		item = item->child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	if (0 < unreadMsgs) {
		item->setData(unreadMsgs, ROLE_ACNT_UNREAD_MSGS);
	} else {
		item->setData(QVariant(), ROLE_ACNT_UNREAD_MSGS);
	}

	return true;
}


/* ========================================================================= */
/*
 * Add year node into account.
 */
bool AccountModel::addYear(QStandardItem *item, NodeType nodeType,
    const QString &year, unsigned unreadMsgs)
/* ========================================================================= */
{
	/* Find account top. */
	item = itemTop(item);

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	if (nodeReceivedYear == nodeType) {
		/* Get received node. */
		item = item->child(2, 0)->child(0, 0);
	} else if (nodeSentYear == nodeType) {
		/* Get sent node. */
		item = item->child(2, 0)->child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	/* Check whether year already exists as child element. */
	for (int i = 0; i < item->rowCount(); ++i) {
		if (year == item->child(i, 0)->text()) {
			return false;
		}
	}

	/* Add new child item and set proper icon. */
	QStandardItem *yearItem = new QStandardItem(year);
	if (nodeReceivedYear == nodeType) {
		yearItem->setIcon(QIcon(ICON_16x16_PATH +
		    QString("datovka-message-download.png")));
	} else {
		yearItem->setIcon(QIcon(ICON_16x16_PATH +
		    QString("datovka-message-reply.png")));
	}
	yearItem->setFlags(yearItem->flags() & ~Qt::ItemIsEditable);
	if (0 < unreadMsgs) {
		yearItem->setData(unreadMsgs, ROLE_ACNT_UNREAD_MSGS);
	}
	item->appendRow(yearItem);

	return true;
}


/* ========================================================================= */
/*
 * Update existing year node in account.
 */
bool AccountModel::updateYear(QStandardItem *item, NodeType nodeType,
    const QString &year, unsigned unreadMsgs)
/* ========================================================================= */
{
	/* Find account top. */
	item = itemTop(item);

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	if (nodeReceivedYear == nodeType) {
		/* Get received node. */
		item = item->child(2, 0)->child(0, 0);
	} else if (nodeSentYear == nodeType) {
		/* Get sent node. */
		item = item->child(2, 0)->child(1, 0);
	} else {
		Q_ASSERT(0);
		return false;
	}

	if (0 == item) {
		Q_ASSERT(0);
		return false;
	}

	/* Check whether year already exists as child element. */
	for (int i = 0; i < item->rowCount(); ++i) {
		if (year == item->child(i, 0)->text()) {
			if (0 < unreadMsgs) {
				item->child(i, 0)->setData(unreadMsgs,
				    ROLE_ACNT_UNREAD_MSGS);
			} else {
				item->child(i, 0)->setData(QVariant(),
				    ROLE_ACNT_UNREAD_MSGS);
			}
			return true;
		}
	}

	return false; /* Element not found. */
}


/* ========================================================================= */
/*
 * Delete year-related nodes in model for given account.
 */
void AccountModel::removeYearNodes(const QModelIndex &topIndex)
/* ========================================================================= */
{
	QStandardItem *topItem;
	QStandardItem *item;

	if (!topIndex.isValid()) {
		Q_ASSERT(0);
		return;
	}
	Q_ASSERT(topIndex == indexTop(topIndex));

	topItem = this->itemFromIndex(topIndex);
	Q_ASSERT(0 != topItem);
	/* Received. */
	item = topItem->child(2, 0)->child(0, 0);
	Q_ASSERT(0 != item);
	item->removeRows(0, item->rowCount());
	/* Sent. */
	item = topItem->child(2, 0)->child(1, 0);
	Q_ASSERT(0 != item);
	item->removeRows(0, item->rowCount());
}


/* ========================================================================= */
/*
 * Delete all year-related nodes in model.
 */
void AccountModel::removeAllYearNodes(void)
/* ========================================================================= */
{
	QStandardItem *itemRoot = this->invisibleRootItem();
	QStandardItem *item;

//	qDebug() << "Deleting all year items.";

	for (int i = 0; i < itemRoot->rowCount(); ++i) {
		/* Received. */
		item = itemRoot->child(i, 0)->child(2, 0)->child(0, 0);
		Q_ASSERT(0 != item);
		item->removeRows(0, item->rowCount());
		/* Sent. */
		item = itemRoot->child(i, 0)->child(2, 0)->child(1, 0);
		Q_ASSERT(0 != item);
		item->removeRows(0, item->rowCount());
	}
}


/* ========================================================================= */
/* Static function definitions below here. */
/* ========================================================================= */
