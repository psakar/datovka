

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>

#include "message_db.h"


static
const QVector<QString> receivedHeaderVector = {
	QObject::tr("Id"),
	QObject::tr("Message Subject"),
	QObject::tr("Sender"),
	QObject::tr("Received"),
	QObject::tr("Accepted"),
	QObject::tr("Locally read")
};


static
const QVector<QString> sentHeaderVector = {
	QObject::tr("Id"),
	QObject::tr("Message Subject"),
	QObject::tr("Recipient"),
	QObject::tr("Received"),
	QObject::tr("Accepted"),
	QObject::tr("Status")
};


/* ========================================================================= */
MessageDb::MessageDb(const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_sqlModel()
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
QAbstractTableModel * MessageDb::receivedModel(const QString &recipDbId)
/* ========================================================================= */
{
	/* TODO */
	QSqlQuery query(m_db);
	QString queryStr =
	    "SELECT dmID, dmAnnotation, dmSender, dmDeliveryTime, "
	    "dmAcceptanceTime FROM messages WHERE dbIDRecipient = '" +
	    recipDbId + "'";
	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);

	return &m_sqlModel;
}


/* ========================================================================= */
QAbstractTableModel * MessageDb::sentModel(const QString &sendDbId)
/* ========================================================================= */
{
	/* TODO */
	QSqlQuery query(m_db);
	QString queryStr =
	    "SELECT dmID, dmAnnotation, dmRecipient, dmMessageStatus, "
	    "dmDeliveryTime, dmAcceptanceTime "
	    "FROM messages WHERE dbIDSender = '" +
	    sendDbId + "'";
	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Adds _dmType column.
 */
bool  MessageDb::addDmtypeColumn(void)
/* ========================================================================= */
{
	if (false == m_db.isOpen()) {
		return false;
	}

	/*
	 * Create _dmType column if it does not exist.
	 */
	QSqlQuery query(m_db);
	query.prepare("SELECT _dmType FROM messages LIMIT 1");
	if (false == query.exec()) {
		query.clear();
		query.prepare("ALTER TABLE messages ADD COLUMN _dmType TEXT");
		query.exec();
	}

	return true;
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
