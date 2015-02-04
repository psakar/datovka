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
		(*this)[DB_DIR] = QString();
	} else {
		(*this)[DB_DIR] = path;
	}
}


/* ========================================================================= */
void AccountModel::SettingsMap::setLastMsg(const QString &dmId)
/* ========================================================================= */
{
	(*this)[LASTMSG] = dmId;
}


/* ========================================================================= */
void AccountModel::SettingsMap::setLastAttachPath(const QString &path)
/* ========================================================================= */
{
	(*this)[LASTATTACH] = path;
}

/* ========================================================================= */
void AccountModel::SettingsMap::setLastCorrespPath(const QString &path)
/* ========================================================================= */
{
	(*this)[LASTCORRESP] = path;
}

/* ========================================================================= */
void AccountModel::SettingsMap::setLastZFOExportPath(const QString &path)
/* ========================================================================= */
{
	(*this)[LASTZFO] = path;
}


/* ========================================================================= */
bool AccountModel::SettingsMap::testAccount(void) const
/* ========================================================================= */
{
	return (*this)[TEST].toBool();
}

/* ========================================================================= */
QString AccountModel::SettingsMap::certPath(void) const
/* ========================================================================= */
{
	return (*this)[P12FILE].toString();
}


/* ========================================================================= */
QString AccountModel::SettingsMap::lastMsg(void) const
/* ========================================================================= */
{
	return (*this)[LASTMSG].toString();
}

/* ========================================================================= */
QString AccountModel::SettingsMap::lastAttachPath(void) const
/* ========================================================================= */
{
	return (*this)[LASTATTACH].toString();
}

/* ========================================================================= */
QString AccountModel::SettingsMap::lastCorrespPath(void) const
/* ========================================================================= */
{
	return (*this)[LASTCORRESP].toString();
}

/* ========================================================================= */
QString AccountModel::SettingsMap::lastZFOExportPath(void) const
/* ========================================================================= */
{
	return (*this)[LASTZFO].toString();
}

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
			SettingsMap settingsMap =
			    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();
			qDebug() << "A001" << globPref.confDir();
			if (!settingsMap.dbDir().isEmpty()) {
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

	/* Get all credentials. */
	for (int i = 0; i < groups.size(); ++i) {
		/* Matches regular expression. */
		if (credRe.exactMatch(groups.at(i))) {
			itemSettings.clear();
			/*
			 * String containing comma character are loaded as
			 * a string list.
			 *
			 * FIXME -- Any white-space characters trailing
			 * the comma are lost.
			 */
			itemSettings.setAccountName(
			    settings.value(groups.at(i) + "/" + ACCOUNT_NAME,
			        "").toStringList().join(", "));
			itemSettings.setUserName(
			    settings.value(groups.at(i) + "/" + USER,
			        "").toString());
			itemSettings.setLoginMethod(
			    settings.value(groups.at(i) + "/" + LOGIN,
			    "").toString());
			itemSettings.setPassword(fromBase64(
			    settings.value(groups.at(i) + "/" + PWD,
			        "").toString()));
			itemSettings.insert(TEST,
			    settings.value(groups.at(i) + "/" + TEST,
			        "").toBool());
			itemSettings.insert(REMEMBER,
			    settings.value(groups.at(i) + "/" + REMEMBER,
			        "").toBool());
			itemSettings.setDbDir(
			    settings.value(groups.at(i) + "/" + DB_DIR,
			        "").toString());
			itemSettings.insert(SYNC,
			    settings.value(groups.at(i) + "/" + SYNC,
			        "").toBool());
			itemSettings.insert(P12FILE,
			    settings.value(groups.at(i) + "/" + P12FILE,
			        "").toString());
			itemSettings.insert(LASTMSG,
			    settings.value(groups.at(i) + "/" + LASTMSG,
			        "").toString());
			itemSettings.insert(LASTATTACH,
			    settings.value(groups.at(i) + "/" + LASTATTACH,
			        "").toString());
			itemSettings.insert(LASTCORRESP,
			    settings.value(groups.at(i) + "/" + LASTCORRESP,
			        "").toString());
			itemSettings.insert(LASTZFO,
			    settings.value(groups.at(i) + "/" + LASTZFO,
			        "").toString());

			/* Associate map with item node. */
			addAccount(itemSettings[ACCOUNT_NAME].toString(),
			    itemSettings);
		}
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
		const SettingsMap &itemSettings =
		    this->item(i)->data(ROLE_ACNT_CONF_SETTINGS).toMap();

		groupName = CREDENTIALS;
		if (i > 0) {
			groupName.append(QString::number(i + 1));
		}
		settings.beginGroup(groupName);

		settings.setValue(ACCOUNT_NAME, itemSettings.accountName());
		settings.setValue(USER, itemSettings.userName());
		settings.setValue(LOGIN, itemSettings.loginMethod());
		if (!itemSettings.password().isEmpty()) {
			settings.setValue(PWD,
			    toBase64(itemSettings.password()));
		}
		settings.setValue(TEST, itemSettings.value(TEST));
		settings.setValue(REMEMBER, itemSettings.value(REMEMBER));
		if (!itemSettings.dbDir().isEmpty()) {
			if (itemSettings.dbDir() != globPref.confDir()) {
				settings.setValue(DB_DIR,
				    itemSettings.dbDir());
			}
		}
		if (!itemSettings.value(P12FILE).isNull() &&
		    itemSettings.value(P12FILE).isValid() &&
		    !itemSettings.value(P12FILE).toString().isEmpty()) {
			settings.setValue(P12FILE, itemSettings.value(P12FILE));
		}

		settings.setValue(SYNC, itemSettings.value(SYNC));

		if (!itemSettings.value(LASTMSG).isNull() &&
		    itemSettings.value(LASTMSG).isValid() &&
		    !itemSettings.value(LASTMSG).toString().isEmpty()) {
			settings.setValue(LASTMSG, itemSettings.value(LASTMSG));
		}

		/* save last attachments path */
		if (!itemSettings.value(LASTATTACH).isNull() &&
		    itemSettings.value(LASTATTACH).isValid() &&
		    !itemSettings.value(LASTATTACH).toString().isEmpty()) {
			settings.setValue(LASTATTACH, itemSettings.value(LASTATTACH));
		}

		/* save last correspondence export path */
		if (!itemSettings.value(LASTCORRESP).isNull() &&
		    itemSettings.value(LASTCORRESP).isValid() &&
		    !itemSettings.value(LASTCORRESP).toString().isEmpty()) {
			settings.setValue(LASTCORRESP, itemSettings.value(LASTCORRESP));
		}

		/* save last ZFO export path */
		if (!itemSettings.value(LASTZFO).isNull() &&
		    itemSettings.value(LASTZFO).isValid() &&
		    !itemSettings.value(LASTZFO).toString().isEmpty()) {
			settings.setValue(LASTZFO, itemSettings.value(LASTZFO));
		}

		settings.endGroup();
	}
}


/* ========================================================================= */
/*
 * Add account.
 */
QModelIndex AccountModel::addAccount(const QString &name, const QVariant &data)
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

	account->setData(data, ROLE_ACNT_CONF_SETTINGS);

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
	return topItem->data(ROLE_ACNT_CONF_SETTINGS).toMap();
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
	topItem->setData(map, ROLE_ACNT_CONF_SETTINGS);
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(0 != item);
	if (0 == item) {
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

	Q_ASSERT(topIndex.isValid());
	if (!topIndex.isValid()) {
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
