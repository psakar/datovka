

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>

#include "message_db.h"


const QVector< QPair<QString, dbEntryType> >
    MessageDb::messagesTblKnownAttrs = {
	{"dmID", DB_INTEGER},
	{"is_verified", DB_BOOLEAN},
	{"_origin", DB_TEXT},
	{"dbIDSender", DB_TEXT},
	{"dmSender", DB_TEXT},
	{"dmSenderAddress", DB_TEXT},
	{"dmSenderType", DB_INTEGER},
	{"dmRecipient", DB_TEXT},
	{"dmRecipientAddress", DB_TEXT},
	{"dmAmbiguousRecipient", DB_TEXT},
	{"dmSenderOrgUnit", DB_TEXT},
	{"dmSenderOrgUnitNum", DB_TEXT},
	{"dbIDRecipient", DB_TEXT},
	{"dmRecipientOrgUnit", DB_TEXT},
	{"dmRecipientOrgUnitNum", DB_TEXT},
	{"dmToHands", DB_TEXT},
	{"dmAnnotation", DB_TEXT},
	{"dmRecipientRefNumber", DB_TEXT},
	{"dmSenderRefNumber", DB_TEXT},
	{"dmRecipientIdent", DB_TEXT},
	{"dmSenderIdent", DB_TEXT},
	{"dmLegalTitleLaw", DB_TEXT},
	{"dmLegalTitleYear", DB_TEXT},
	{"dmLegalTitleSect", DB_TEXT},
	{"dmLegalTitlePar", DB_TEXT},
	{"dmLegalTitlePoint", DB_TEXT},
	{"dmPersonalDelivery", DB_BOOLEAN},
	{"dmAllowSubstDelivery", DB_BOOLEAN},
	{"dmQTimestamp", DB_TEXT},
	{"dmDeliveryTime", DB_DATETIME},
	{"dmAcceptanceTime", DB_DATETIME},
	{"dmMessageStatus", DB_INTEGER},
	{"dmAttachmentSize", DB_INTEGER},
	{"_dmType", DB_TEXT}
};


const QMap<QString, QString> MessageDb::messagesTblAttrNames = {
	{"dmID", QObject::tr("ID")},
	{"is_verified", ""},
	{"_origin", ""},
	{"dbIDSender", ""},
	{"dmSender", QObject::tr("Sender")},
	{"dmSenderAddress", ""},
	{"dmSenderType", ""},
	{"dmRecipient", QObject::tr("Recipient")},
	{"dmRecipientAddress", ""},
	{"dmAmbiguousRecipient", ""},
	{"dmSenderOrgUnit", ""},
	{"dmSenderOrgUnitNum", ""},
	{"dbIDRecipient", ""},
	{"dmRecipientOrgUnit", ""},
	{"dmRecipientOrgUnitNum", ""},
	{"dmToHands", ""},
	{"dmAnnotation", QObject::tr("Title")},
	{"dmRecipientRefNumber", ""},
	{"dmSenderRefNumber", ""},
	{"dmRecipientIdent", ""},
	{"dmSenderIdent", ""},
	{"dmLegalTitleLaw", ""},
	{"dmLegalTitleYear", ""},
	{"dmLegalTitleSect", ""},
	{"dmLegalTitlePar", ""},
	{"dmLegalTitlePoint", ""},
	{"dmPersonalDelivery", ""},
	{"dmAllowSubstDelivery", ""},
	{"dmQTimestamp", ""},
	{"dmDeliveryTime", QObject::tr("Delivered")},
	{"dmAcceptanceTime", QObject::tr("Accepted")},
	{"dmMessageStatus", QObject::tr("Status")},
	{"dmAttachmentSize", ""},
	{"_dmType", ""}
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
	static QVector<QString> itemIds = {"dmID", "dmAnnotation", "dmSender",
	    "dmDeliveryTime", "dmAcceptanceTime"};
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (itemIds.size() - 1); ++i) {
		queryStr += itemIds[i] + ", ";
	}
	queryStr += itemIds.last();
	queryStr += " FROM messages WHERE dbIDRecipient = '" + recipDbId + "'";
	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < itemIds.size(); ++i) {
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(itemIds[i]));
	}

	return &m_sqlModel;
}


/* ========================================================================= */
QAbstractTableModel * MessageDb::sentModel(const QString &sendDbId)
/* ========================================================================= */
{
	static QVector<QString> itemIds = {"dmID", "dmAnnotation",
	    "dmRecipient", "dmMessageStatus", "dmDeliveryTime",
	    "dmAcceptanceTime"};
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (itemIds.size() - 1); ++i) {
		queryStr += itemIds[i] + ", ";
	}
	queryStr += itemIds.last();
	queryStr += " FROM messages WHERE dbIDSender = '" +
	    sendDbId + "'";
	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < itemIds.size(); ++i) {
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(itemIds[i]));
	}

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
