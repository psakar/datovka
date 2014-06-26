

#include <QDebug>
#include <QRegExp>
#include "accounts_model.h"
#include "src/common.h"


/*!
 * @brief Converts base64 encoded string into plain text.
 */
static
QString fromBase64(const QString &base64);


/*!
 * @brief Converts string into base64.
 */
static
QString toBase64(const QString &plain);


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
QString AccountModel::SettingsMap::accountName(void) const
/* ========================================================================= */
{
	return (*this)[NAME].toString();
}


/* ========================================================================= */
QString AccountModel::SettingsMap::loginMethod(void) const
/* ========================================================================= */
{
	return (*this)[LOGIN].toString();
}


/* ========================================================================= */
QString AccountModel::SettingsMap::userName(void) const
/* ========================================================================= */
{
	return (*this)[USER].toString();
}


/* ========================================================================= */
QString AccountModel::SettingsMap::password(void) const
/* ========================================================================= */
{
	return (*this)[PWD].toString();
}


/* ========================================================================= */
void AccountModel::SettingsMap::setPassword(const QString &pwd)
/* ========================================================================= */
{
	(*this)[PWD] = pwd;
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
/*
 * Empty account model constructor.
 */
AccountModel::AccountModel(QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(parent)
{
	/* Set header. */
	this->setHorizontalHeaderItem(0, new QStandardItem(tr("Accounts")));
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
			itemSettings.insert(NAME,
			    settings.value(groups.at(i) + "/" + NAME, ""));
			itemSettings.insert(USER,
			    settings.value(groups.at(i) + "/" + USER, ""));
			itemSettings.insert(LOGIN,
			    settings.value(groups.at(i) + "/" + LOGIN, ""));
			itemSettings.insert(PWD, fromBase64(
			    settings.value(groups.at(i) + "/" + PWD,
			        "").toString()));
			itemSettings.insert(TEST,
			    settings.value(groups.at(i) + "/" + TEST,
			        "").toBool());
			itemSettings.insert(REMEMBER,
			    settings.value(groups.at(i) + "/" + REMEMBER,
			        "").toBool());
			itemSettings.insert(DB_DIR,
			    settings.value(groups.at(i) + "/" + DB_DIR, ""));
			itemSettings.insert(SYNC,
			    settings.value(groups.at(i) + "/" + SYNC,
			        "").toBool());
			itemSettings.insert(P12FILE,
			    settings.value(groups.at(i) + "/" + P12FILE,
			        "").toString());
			/* Associate map with item node. */
			addAccount(itemSettings[NAME].toString(),
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
//		qDebug() << this->item(i)->text() << groupName;
		settings.beginGroup(groupName);

		settings.setValue(NAME, itemSettings.value(NAME));
		settings.setValue(USER, itemSettings.value(USER));
		settings.setValue(LOGIN, itemSettings.value(LOGIN));
		if (!itemSettings.value(PWD).isNull() &&
		    itemSettings.value(PWD).isValid() &&
		    !itemSettings.value(PWD).toString().isEmpty()) {
			settings.setValue(PWD,
			    toBase64(itemSettings.value(PWD).toString()));
		}
		settings.setValue(TEST, itemSettings.value(TEST));
		settings.setValue(REMEMBER, itemSettings.value(REMEMBER));
		if (!itemSettings.value(DB_DIR).isNull() &&
		    itemSettings.value(DB_DIR).isValid() &&
		    !itemSettings.value(DB_DIR).toString().isEmpty()) {
			settings.setValue(DB_DIR, itemSettings.value(DB_DIR));
		}
		if (!itemSettings.value(P12FILE).isNull() &&
		    itemSettings.value(P12FILE).isValid() &&
		    !itemSettings.value(P12FILE).toString().isEmpty()) {
			settings.setValue(P12FILE, itemSettings.value(P12FILE));
		}

		settings.setValue(SYNC, itemSettings.value(SYNC));

		settings.endGroup();
	}
}


/* ========================================================================= */
/*
 * Add account.
 */
bool AccountModel::addAccount(const QString &name, const QVariant &data)
/* ========================================================================= */
{
	/* Defining a couple of items. */
	QStandardItem *account = new QStandardItem(name);
	account->setFlags(account->flags() & ~Qt::ItemIsEditable);
	QStandardItem *recentRecieved =
	    new QStandardItem(tr("Recent Recieved"));
	recentRecieved->setFlags(recentRecieved->flags() & ~Qt::ItemIsEditable);
	QStandardItem *recentSent = new QStandardItem(tr("Recent Sent"));
	recentSent->setFlags(recentSent->flags() & ~Qt::ItemIsEditable);
	QStandardItem *all = new QStandardItem(tr("All"));
	all->setFlags(all->flags() & ~Qt::ItemIsEditable);
	QStandardItem *allRecieved = new QStandardItem(tr("Recieved"));
	allRecieved->setFlags(allRecieved->flags() & ~Qt::ItemIsEditable);
	QStandardItem *allSent = new QStandardItem(tr("Sent"));
	allSent->setFlags(allSent->flags() & ~Qt::ItemIsEditable);

	account->setData(data, ROLE_ACNT_CONF_SETTINGS);

	/* Account node is drawn bold in the data() method. */
	account->setIcon(QIcon(ICON_3PARTY_PATH + QString("letter_16.png")));

	recentRecieved->setIcon(
	    QIcon(ICON_16x16_PATH + QString("datovka-message-download.png")));
	recentSent->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-reply.png")));
	allRecieved->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-download.png")));
	allSent->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-reply.png")));

	/* Building up the hierarchy. */
	account->appendRow(recentRecieved);
	account->appendRow(recentSent);
	account->appendRow(all);
	all->appendRow(allRecieved);
	all->appendRow(allSent);

	invisibleRootItem()->appendRow(account);
	return true;
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


/* ========================================================================= */
/*
 * Converts base64 encoded string into plain text.
 */
static
QString fromBase64(const QString &base64)
/* ========================================================================= */
{
	return QString::fromUtf8(QByteArray::fromBase64(base64.toUtf8()));
}


/* ========================================================================= */
/*!
 * @brief Converts string into base64.
 */
static
QString toBase64(const QString &plain)
/* ========================================================================= */
{
	return QString::fromUtf8(plain.toUtf8().toBase64());
}
