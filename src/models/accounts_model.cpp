

#include "accounts_model.h"
#include "src/common.h"
#include <QDebug>

/* ========================================================================= */
AccountModel::AccountModel(void)
/* ========================================================================= */
    : QStandardItemModel()
{
	/* Add header. */
	this->setHorizontalHeaderItem(0, new QStandardItem(tr("Accounts")));

	/* Load content. */
	addAccount("Testovací účet 1");
	addAccount("Testovací účet 2");
}


/* ========================================================================= */
bool AccountModel::addAccount(const QString &accountName)
/* ========================================================================= */
{
	/* Defining a couple of items. */
	QStandardItem *account = new QStandardItem(accountName);
	QFont font;
	QStandardItem *recentRecieved =
	    new QStandardItem(tr("Recent Recieved"));
	QStandardItem *recentSent = new QStandardItem(tr("Recent Sent"));
	QStandardItem *all = new QStandardItem(tr("All"));
	QStandardItem *allRecieved = new QStandardItem(tr("Recieved"));
	QStandardItem *allSent = new QStandardItem(tr("Sent"));

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
