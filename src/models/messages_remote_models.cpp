

#include "messages_remote_models.h"


/* ========================================================================= */
ReceivedMessagesRemoteModel::ReceivedMessagesRemoteModel(QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(0, 6, parent)
{
	/* Add header. */
	this->setHorizontalHeaderItem(0,
	    new QStandardItem(QString(tr("Id"))));
	this->setHorizontalHeaderItem(1,
	    new QStandardItem(QString(tr("Message Subject"))));
	this->setHorizontalHeaderItem(2,
	    new QStandardItem(QString(tr("Sender"))));
	this->setHorizontalHeaderItem(3,
	    new QStandardItem(QString(tr("Recieved"))));
	this->setHorizontalHeaderItem(4,
	    new QStandardItem(QString(tr("Accepted"))));
	this->setHorizontalHeaderItem(5,
	    new QStandardItem(QString(tr("Locally read"))));

	/* Load content. */
	fillContent();
}


/* ========================================================================= */
bool ReceivedMessagesRemoteModel::addMessage(int row, const QString &id,
    const QString &title, const QString &sender, const QString &delivered,
    const QString &accepted)
/* ========================================================================= */
{
	QStandardItem *item;

	item = new QStandardItem(id);
	this->setItem(row, 0, item);
	item = new QStandardItem(title);
	this->setItem(row, 1, item);
	item = new QStandardItem(sender);
	this->setItem(row, 2, item);
	item = new QStandardItem(delivered);
	this->setItem(row, 3, item);
	item = new QStandardItem(accepted);
	this->setItem(row, 4, item);

	return true;
}


/* ========================================================================= */
void ReceivedMessagesRemoteModel::fillContent(void)
/* ========================================================================= */
{
	for(int i=0; i<3; i++) {
		this->addMessage(i, "12365",
		    "Dodávka světelných mečů", "Orion", "12.3.2014 12:12",
		    "12.3.2014 12:15");
	}
}


/* ========================================================================= */
SentMessagesRemoteModel::SentMessagesRemoteModel(QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(0, 6, parent)
{
	/* Add header. */
	this->setHorizontalHeaderItem(0,
	    new QStandardItem(QString(tr("Id"))));
	this->setHorizontalHeaderItem(1,
	    new QStandardItem(QString(tr("Message Subject"))));
	this->setHorizontalHeaderItem(2,
	    new QStandardItem(QString(tr("Recipient"))));
	this->setHorizontalHeaderItem(3,
	    new QStandardItem(QString(tr("Recieved"))));
	this->setHorizontalHeaderItem(4,
	    new QStandardItem(QString(tr("Accepted"))));
	this->setHorizontalHeaderItem(5,
	    new QStandardItem(QString(tr("Status"))));

	/* Load content. */
	fillContent();
}


/* ========================================================================= */
bool SentMessagesRemoteModel::addMessage(int row, const QString &id,
    const QString &title, const QString &recipient, const QString &status,
    const QString &delivered, const QString &accepted)
/* ========================================================================= */
{
	QStandardItem *item;

	item = new QStandardItem(id);
	this->setItem(row,0, item);
	item = new QStandardItem(title);
	this->setItem(row,1, item);
	item = new QStandardItem(recipient);
	this->setItem(row,2, item);
	item = new QStandardItem(status);
	this->setItem(row,3, item);
	item = new QStandardItem(delivered);
	this->setItem(row,4, item);
	item = new QStandardItem(accepted);
	this->setItem(row,5, item);

	return true;
}


/* ========================================================================= */
void SentMessagesRemoteModel::fillContent(void)
/* ========================================================================= */
{
	for(int i=0; i<3; i++) {
		this->addMessage(i, "12365",
		    "Dodávka světelných mečů", "Orion", "12.3.2014 12:12",
		    "12.3.2014 12:15", "Ok");
	}
}
