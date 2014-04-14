

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>


#include "src/common.h"
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


const QVector<QString> MessageDb::receivedItemIds = {"dmID", "dmAnnotation",
    "dmSender", "dmDeliveryTime", "dmAcceptanceTime"};
const QVector<dbEntryType> MessageDb::receivedItemTypes = {DB_INTEGER, DB_TEXT,
    DB_TEXT, DB_DATETIME, DB_DATETIME};


const QVector<QString> MessageDb::sentItemIds = {"dmID", "dmAnnotation",
    "dmRecipient", "dmMessageStatus", "dmDeliveryTime",
    "dmAcceptanceTime"};
const QVector<dbEntryType> MessageDb::sentItemTypes = {DB_INTEGER, DB_TEXT,
    DB_TEXT, DB_INTEGER, DB_DATETIME, DB_DATETIME};


/* ========================================================================= */
/*
 * Convert viewed data in date/time columns.
 */
QVariant dbTableModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
	if ((Qt::DisplayRole == role) &&
	    (this->headerData(index.column(), Qt::Horizontal,
	         ROLE_DB_ENTRY_TYPE).toInt() == DB_DATETIME)) {
		/* Convert date on display. */
		return dateTimeFromDbFormat(
		    QSqlQueryModel::data(index, role).toString(),
		    dateTimeDisplayFormat);
	} else {
		return QSqlQueryModel::data(index, role);
	}
}


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
/*
 * Return received messages within past 90 days;
 */
QAbstractTableModel * MessageDb::receivedModel(const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE dbIDRecipient = '" + recipDbId + "'";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(receivedItemIds[i]));
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    receivedItemTypes[i], ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days.
 */
QAbstractTableModel * MessageDb::receivedWithin90DaysModel(
    const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDRecipient = '" + recipDbId + "')"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(receivedItemIds[i]));
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    receivedItemTypes[i], ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::receivedYears(const QString &recipDbId)
/* ========================================================================= */
{
	QList<QString> yearList;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT strftime('%Y', dmDeliveryTime) "
	    "FROM messages WHERE "
	    "dbIDRecipient = '" + recipDbId + "'"
	    " ORDER BY dmDeliveryTime ASC";

//	qDebug() << "Generating received year list" << recipDbId;
//	qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
//			qDebug() << query.value(0).toString();
			yearList.append(query.value(0).toString());
			query.next();
		}
	}

	return yearList;
}


/* ========================================================================= */
/*
 * Return sent messages model.
 */
QAbstractTableModel * MessageDb::sentModel(const QString &sendDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE dbIDSender = '" +
	    sendDbId + "'";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(sentItemIds[i]));
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal, sentItemTypes[i],
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return sent messages within past 90 days.
 */
QAbstractTableModel * MessageDb::sentWithin90DaysModel(
    const QString &sendDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDSender = '" + sendDbId + "')"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    messagesTblAttrNames.value(sentItemIds[i]));
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal, sentItemTypes[i],
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::sentYears(const QString &sendDbId)
/* ========================================================================= */
{
	QList<QString> yearList;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT strftime('%Y', dmDeliveryTime) "
	    "FROM messages WHERE "
	    "dbIDSender = '" + sendDbId + "'"
	    " ORDER BY dmDeliveryTime ASC";

//	qDebug() << "Generating received year list" << recipDbId;
//	qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
//			qDebug() << query.value(0).toString();
			yearList.append(query.value(0).toString());
			query.next();
		}
	}

	return yearList;
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
    const QString &locDir, bool testing)
/* ========================================================================= */
{
	MessageDb *db;

	/* Already opened. */
	if (this->find(key) != this->end()) {
//		qDebug() << key << "db found";
		return (*this)[key];
	}

//	qDebug() << "creating new" << key;
	db = new MessageDb(key);

//	qDebug() << "searching for file" << key << "in" << locDir;
	/* TODO -- Handle file name deviations! */
//	qDebug() << "opening";
	/*
	 * Test accounts have ___1 in their names, ___0 relates to standard
	 * accounts.
	 */
	db->openDb(locDir + "/" + key + "___" + (testing ? "1" : "0") + ".db");

	this->insert(key, db);
	return db;
}
