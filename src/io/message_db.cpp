

#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlError>
#include <QSqlQuery>


#include "src/common.h"
#include "message_db.h"


const QVector<QString> MessageDb::receivedItemIds = {"dmID", "dmAnnotation",
    "dmSender", "dmDeliveryTime", "dmAcceptanceTime"};


const QVector<QString> MessageDb::sentItemIds = {"dmID", "dmAnnotation",
    "dmRecipient", "dmMessageStatus", "dmDeliveryTime",
    "dmAcceptanceTime"};


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
QAbstractTableModel * MessageDb::msgsRcvdModel(const QString &recipDbId)
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
		    MsgsTbl::attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days.
 */
QAbstractTableModel * MessageDb::msgsRcvdWithin90DaysModel(
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
		    MsgsTbl::attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return received messages within given year.
 */
QAbstractTableModel * MessageDb::msgsRcvdInYearModel(const QString &recipDbId,
    const QString &year)
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
	    "(strftime('%Y', dmDeliveryTime) = '" + year + "')";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::msgsRcvdYears(const QString &recipDbId) const
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
 * Return list of years and number of messages in database.
 */
QList< QPair<QString, int> > MessageDb::msgsRcvdYearlyCounts(
    const QString &recipDbId) const
/* ========================================================================= */
{
	QList< QPair<QString, int> > yearlyCounts;
	QList<QString> yearList = msgsRcvdYears(recipDbId);
	QSqlQuery query(m_db);
	QString queryStr;

	for (int i = 0; i < yearList.size(); ++i) {
		queryStr = "SELECT COUNT(*) AS nrRecords FROM messages WHERE "
		    "(dbIDRecipient = '" + recipDbId + "')"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = '" + yearList[i] + "')";
//		qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive()) {
			query.first();
			yearlyCounts.append(QPair<QString, int>(yearList[i],
			    query.value(0).toInt()));
		}
	}

	return yearlyCounts;
}


/* ========================================================================= */
/*
 * Return sent messages model.
 */
QAbstractTableModel * MessageDb::msgsSntModel(const QString &sendDbId)
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
		    MsgsTbl::attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(sentItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return sent messages within past 90 days.
 */
QAbstractTableModel * MessageDb::msgsSntWithin90DaysModel(
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
		    MsgsTbl::attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(sentItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return sent messages within given year.
 */
QAbstractTableModel * MessageDb::msgsSntInYearModel(const QString &sendDbId,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDSender = '" + sendDbId + "')"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = '" + year + "')";
//	qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    MsgsTbl::attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::msgsSntYears(const QString &sendDbId) const
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
 * Return list of years and number of messages in database.
 */
QList< QPair<QString, int> > MessageDb::msgsSntYearlyCounts(
    const QString &sendDbId) const
/* ========================================================================= */
{
	QList< QPair<QString, int> > yearlyCounts;
	QList<QString> yearList = msgsSntYears(sendDbId);
	QSqlQuery query(m_db);
	QString queryStr;

	for (int i = 0; i < yearList.size(); ++i) {
		queryStr = "SELECT COUNT(*) AS nrRecords FROM messages WHERE "
		    "(dbIDSender = '" + sendDbId + "')"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = '" + yearList[i] + "')";
//		qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive()) {
			query.first();
			if (query.isValid()) {
				yearlyCounts.append(QPair<QString, int>(
				    yearList[i], query.value(0).toInt()));
			}
		}
	}

	return yearlyCounts;
}


/* ========================================================================= */
/*
 * Generate information for reply dialog.
 */
QVector<QString> MessageDb::msgsReplyDataTo(int dmId) const
/* ========================================================================= */
{
	QVector<QString> reply(4);
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "dmAnnotation, dbIDSender, dmSender, dmSenderAddress"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
//	qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			reply[0] = query.value(0).toString();
			reply[1] = query.value(1).toString();
			reply[2] = query.value(2).toString();
			reply[3] = query.value(3).toString();
		}
	}

	return reply;
}


/* ========================================================================= */
/*
 * Return contact list from message db.
 */
QList< QVector<QString> > MessageDb::uniqueContacts(void)
/* ========================================================================= */
{
	QList<QVector<QString>> list_contacts;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT "
	   "dbIDSender, dmSender, dmSenderAddress"
	   " FROM messages";
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
			QVector<QString> contact;
			contact.append(query.value(0).toString());
			contact.append(query.value(1).toString());
			contact.append(query.value(2).toString());
			list_contacts.append(contact);
			query.next();
		}
	}
	return list_contacts;
}


/* ========================================================================= */
/*
 * Read data from supplementary message data table.
 */
QJsonDocument MessageDb::smsgdCustomData(int msgId) const
/* ========================================================================= */
{
	QJsonDocument jsonDoc;
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "custom_data"
	    " FROM supplementary_message_data WHERE "
	    "message_id = "  + QString::number(msgId);
//	qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			jsonDoc = QJsonDocument::fromJson(
			    query.value(0).toByteArray());
		}
	}

	return jsonDoc;
}


/* ========================================================================= */
/*
 * Return message HTML formatted description.
 */
QString MessageDb::descriptionHtml(int dmId, bool showId) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;

	html += "<h3>" + tr("Identification") + "</h3>";
	if (showId) {
		html += strongAccountInfoLine(tr("ID"), QString::number(dmId));
	}

	queryStr = "SELECT "
	    "dmAnnotation, _dmType, dmSender, dmSenderAddress"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
	qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		html += strongAccountInfoLine(tr("Subject"),
		    query.value(0).toString());
		if (!query.value(1).toString().isEmpty() &&
		    (!dmTypeToText(query.value(1).toString()).isEmpty())) {
			html += strongAccountInfoLine(tr("Message type"),
			    dmTypeToText(query.value(1).toString()));
		}

		html += "<br/>";

		/* Information about message author. */
		html += strongAccountInfoLine("From",
		    query.value(2).toString());
		html += strongAccountInfoLine("Sender Address",
		    query.value(3).toString());
		/* Custom data. */
		QJsonDocument customData = smsgdCustomData(dmId);
		if (!customData.isEmpty() && customData.isObject()) {
			QJsonValue value =
			    customData.object().value("message_author");
			if (value.isObject()) {
				html += strongAccountInfoLine(
				    tr("Message author"),
				    value.toObject().value(
				        "authorName").toString() + ", " +
				    authorTypeToText(value.toObject().value(
				        "userType").toString()));
			}
		}

	}

//	html += QString::number(dmId);
	qDebug() << html;
	/* TODO */

	return html;
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
