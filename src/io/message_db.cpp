

#include <QDebug>
#include <QDir>

#include "message_db.h"


static
QVector<QString> receivedHeaderVector = {
	QObject::tr("Id"),
	QObject::tr("Message Subject"),
	QObject::tr("Sender"),
	QObject::tr("Received"),
	QObject::tr("Accepted"),
	QObject::tr("Locally read")
};


static
QVector<QString> sentHeaderVector = {
	QObject::tr("Id"),
	QObject::tr("Message Subject"),
	QObject::tr("Recipient"),
	QObject::tr("Received"),
	QObject::tr("Accepted"),
	QObject::tr("Status")
};


/* ========================================================================= */
ReceivedMesagesDbModel::ReceivedMesagesDbModel(QSqlDatabase &db,
    QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(0, 6, parent),
    m_dbRef(db)
{
	/* Add header. */
	for (int i = 0; i < receivedHeaderVector.size(); ++i) {
		this->setHorizontalHeaderItem(i,
		    new QStandardItem(receivedHeaderVector[i]));
	}

	/* Load content. */
	fillContent();
}


/* ========================================================================= */
bool ReceivedMesagesDbModel::addMessage(int row, const QString &id,
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
void ReceivedMesagesDbModel::fillContent(void)
/* ========================================================================= */
{
	for(int i=0; i<3; i++) {
		this->addMessage(i, "12365",
		    "Dodávka světelných mečů", "Orion", "12.3.2014 12:12",
		    "12.3.2014 12:15");
	}
}


/* ========================================================================= */
SentMesagesDbModel::SentMesagesDbModel(QSqlDatabase &db, QObject *parent)
/* ========================================================================= */
    : QStandardItemModel(0, 6, parent),
    m_dbRef(db)
{
	/* Add header. */
	for (int i = 0; i < sentHeaderVector.size(); ++i) {
		this->setHorizontalHeaderItem(i,
		    new QStandardItem(sentHeaderVector[i]));
	}

	/* Load content. */
	fillContent();
}


/* ========================================================================= */
bool SentMesagesDbModel::addMessage(int row, const QString &id,
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
void SentMesagesDbModel::fillContent(void)
/* ========================================================================= */
{
	for(int i=0; i<3; i++) {
		this->addMessage(i, "12365",
		    "Dodávka světelných mečů", "Orion", "12.3.2014 12:12",
		    "12.3.2014 12:15", "Ok");
	}
}


/* ========================================================================= */
MessageDb::MessageDb(const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_receivedModel(m_db, this),
    m_sentModel(m_db, this)
{
	m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
}


/* ========================================================================= */
MessageDb::~MessageDb(void)
/* ========================================================================= */
{
	m_db.close();
}


/* ========================================================================= */
/*
 * Open database file.
 */
bool MessageDb::openDb(const QString &fileName)
/* ========================================================================= */
{
	m_db.setDatabaseName(QDir::toNativeSeparators(fileName));

	m_db.setDatabaseName(fileName);

	return m_db.open();
}


/* ========================================================================= */
QStandardItemModel * MessageDb::receivedModel(void)
/* ========================================================================= */
{
	/* TODO */

	return &m_receivedModel;
}


/* ========================================================================= */
QStandardItemModel * MessageDb::sentModel(void)
/* ========================================================================= */
{
	/* TODO */

	return &m_sentModel;
}


/* ========================================================================= */
dbContainer::dbContainer(void)
/* ========================================================================= */
    : QMap<QString, MessageDb *>()
{
}


/* ========================================================================= */
dbContainer::~dbContainer(void)
/* ========================================================================= */
{
	QMap<QString, MessageDb *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}


/* ========================================================================= */
MessageDb * dbContainer::accessMessageDb(const QString &key,
    const QString &locDir)
/* ========================================================================= */
{
	MessageDb *db;

	/* Already opened. */
	if (this->find(key) != this->end()) {
		qDebug() << key << "db found";
		return (*this)[key];
	}

	qDebug() << "creating new" << key;
	db = new MessageDb(key);

	qDebug() << "searching for file" << key << "in" << locDir;
	/* TODO -- Handle file name deviations! */
	qDebug() << "opening" << db->openDb(locDir + "/" + key + "___1.db");

	this->insert(key, db);
	return db;
}
