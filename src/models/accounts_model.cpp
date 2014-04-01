

#include <QDebug>
#include <QRegExp>

#include "accounts_model.h"
#include "src/common.h"


/*!
 * @brief Converts base64 encoded string into plain text.
 */
static
QString fromBase64(const QString &base64);


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

//	/* Load content. */
//	addAccount("Testovací účet 1");
//	addAccount("Testovací účet 2");
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void AccountModel::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	QStringList groups = settings.childGroups();
	QRegExp credRe("credentials.*");
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
			itemSettings.insert(SYNC,
			    settings.value(groups.at(i) + "/" + SYNC,
			        "").toBool());

			/* Associate map with item node. */
			addAccount(itemSettings[NAME].toString(),
			    itemSettings);
		}
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

	QFont font;
	QStandardItem *recentRecieved =
	    new QStandardItem(tr("Recent Recieved"));
	QStandardItem *recentSent = new QStandardItem(tr("Recent Sent"));
	QStandardItem *all = new QStandardItem(tr("All"));
	QStandardItem *allRecieved = new QStandardItem(tr("Recieved"));
	QStandardItem *allSent = new QStandardItem(tr("Sent"));

	account->setData(data);

	font.setBold(true);
//	font.setItalic(true);
	account->setFont(font);
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

/* TODO */
bool AccountModel::addYearItemToAccount(const QModelIndex &index, const QString &year)
{
	qDebug() << index.parent().row() << " - " << index.row();

	QStandardItem *yearitem = new QStandardItem(year);
	yearitem->setIcon(QIcon(
	    ICON_16x16_PATH + QString("datovka-message-download.png")));
	AccountModel::item(index.parent().row(),0)->appendRow(yearitem);

	return true;
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
