

#include <QAbstractTableModel>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QModelIndex>
#include <QObject>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSslCertificate>
#include <QString>
#include <QVariant>
#include <QVector>

#include "message_db.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/pkcs7.h"


/* Joinned tables messages and supplementary_message_data. */
const QVector<QString> MessageDb::receivedItemIds = {"dmID", "dmAnnotation",
    "dmSender", "dmDeliveryTime", "dmAcceptanceTime", "read_locally"};


const QVector<QString> MessageDb::sentItemIds = {"dmID", "dmAnnotation",
    "dmRecipient", "dmDeliveryTime", "dmAcceptanceTime", "dmMessageStatus"};


const QVector<QString> MessageDb::msgAttribs2 = {"dmSenderIdent",
    "dmSenderRefNumber", "dmRecipientIdent", "dmRecipientRefNumber",
    "dmToHands", "dmLegalTitleLaw", "dmLegalTitleYear", "dmLegalTitleSect",
    "dmLegalTitlePar", "dmLegalTitlePoint"};


const QVector<QString> MessageDb::msgStatus = {"dmDeliveryTime",
    "dmAcceptanceTime", "dmMessageStatus"};


const QVector<QString> MessageDb::fileItemIds = {"id", "message_id",
    "dmEncodedContent", "_dmFileDescr", "LENGTH(dmEncodedContent)"};


/* ========================================================================= */
/*
 * Constructor.
 */
DbMsgsTblModel::DbMsgsTblModel(QObject *parent)
/* ========================================================================= */
    : QSqlQueryModel(parent),
    m_overridden()
{
}


/* ========================================================================= */
/*
 * Used for data conversion on display.
 */
QVariant DbMsgsTblModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
#define READLOC_COL 5
	switch (role) {
	case Qt::DisplayRole:
		if (this->headerData(index.column(), Qt::Horizontal,
		         ROLE_MSGS_DB_ENTRY_TYPE).toInt() == DB_DATETIME) {
			/* Convert date on display. */
			return dateTimeStrFromDbFormat(
			    QSqlQueryModel::data(index, role).toString(),
			    dateTimeDisplayFormat);
		}
		if ((READLOC_COL == index.column()) &&
		    (DB_BOOLEAN == headerData(index.column(), Qt::Horizontal,
		         ROLE_MSGS_DB_ENTRY_TYPE).toInt())) {
			/* Hide text for 'read locally'. */
			return QVariant();
		}

		return QSqlQueryModel::data(index, role);
		break;

	case Qt::DecorationRole:
		if ((READLOC_COL == index.column()) &&
		    (DB_BOOLEAN == headerData(index.column(), Qt::Horizontal,
		         ROLE_MSGS_DB_ENTRY_TYPE).toInt())) {
			/* Show icon for 'read locally'. */
			int dmId = QSqlQueryModel::data(
			    index.sibling(index.row(), 0),
			    Qt::DisplayRole).toInt();
			if ((m_overridden.value(dmId, false)) ||
			    QSqlQueryModel::data(index).toBool()) {
				return QIcon(ICON_3PARTY_PATH "tick_16.png");
			} else {
				return QIcon(ICON_3PARTY_PATH "delete_16.png");
			}
		}

		return QSqlQueryModel::data(index, role);
		break;

	case Qt::FontRole:
		if (DB_BOOLEAN == headerData(READLOC_COL, Qt::Horizontal,
		         ROLE_MSGS_DB_ENTRY_TYPE).toInt()) {
			/* In read messages. */
			int dmId = QSqlQueryModel::data(
			    index.sibling(index.row(), 0),
			    Qt::DisplayRole).toInt();
			if ((!QSqlQueryModel::data(index.sibling(index.row(),
			         READLOC_COL)).toBool()) &&
			    (!m_overridden.value(dmId, false))) {
				/* Unread messages are shown bold. */
				QFont boldFont;
				boldFont.setBold(true);
				return boldFont;
			}
		}

		return QSqlQueryModel::data(index, role);
		break;

	default:
		return QSqlQueryModel::data(index, role);
		break;
	}
#undef READLOC_COL
}


/* ========================================================================= */
/*
 * Override message as being read.
 */
bool DbMsgsTblModel::overideRead(int dmId, bool forceRead)
/* ========================================================================= */
{
	m_overridden[dmId] = forceRead;

	return true;
}


/* ========================================================================= */
/*
 * Compute viewed data in file size column.
 */
QVariant DbFlsTblModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
	/* TODO -- Add accurate attachment size computation. */
	if ((Qt::DisplayRole == role) && (4 == index.column())) {
		/* Compute attachment size from base64 length. */
		return QSqlQueryModel::data(index, role).toInt() * 3 / 4;
		/* TODO -- Add fast accurate attachment size computation. */
		//const QByteArray &b64 = QSqlQueryModel::data(
		//    index.sibling(index.row(), 2), role).toByteArray();
		//return QByteArray::fromBase64(b64).size();
	} else {
		return QSqlQueryModel::data(index, role);
	}
}


/* ========================================================================= */
MessageDb::MessageDb(const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_sqlMsgsModel(),
    m_sqlFilesModel()
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
	bool ret;

	m_db.setDatabaseName(QDir::toNativeSeparators(fileName));

	ret = m_db.open();

	if (ret) {
		/* Ensure database contains all tables. */
		createEmptyMissingTables();
	}

	return ret;
}


/* ========================================================================= */
/*
 * Get file name.
 */
QString MessageDb::fileName(void) const
/* ========================================================================= */
{
	return m_db.databaseName();
}


/* ========================================================================= */
/*
 * Return received messages model.
 */
DbMsgsTblModel * MessageDb::msgsRcvdModel(const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE (dbIDRecipient = :recipDbId) ";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(receivedItemIds[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(receivedItemIds[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(
			    receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			Q_ASSERT(0);
		}
	}

	return &m_sqlMsgsModel;
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days.
 */
DbMsgsTblModel * MessageDb::msgsRcvdWithin90DaysModel(
    const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDRecipient = :recipDbId)"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(receivedItemIds[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(receivedItemIds[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(
			    receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			Q_ASSERT(0);
		}
	}

	return &m_sqlMsgsModel;
}


/* ========================================================================= */
/*
 * Return received messages within given year.
 */
DbMsgsTblModel * MessageDb::msgsRcvdInYearModel(const QString &recipDbId,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDRecipient = :recipDbId)"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = :year)";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	query.bindValue(":year", year);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* TODO -- Handle the joined tables in a better way. */
		if (msgsTbl.attrProps.find(receivedItemIds[i]) !=
		    msgsTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    msgsTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else if (smsgdtTbl.attrProps.find(receivedItemIds[i]) !=
		    smsgdtTbl.attrProps.end()) {
			/* Description. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(
			    receivedItemIds[i]).desc);
			/* Data type. */
			m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
			    smsgdtTbl.attrProps.value(receivedItemIds[i]).type,
			    ROLE_MSGS_DB_ENTRY_TYPE);
		} else {
			Q_ASSERT(0);
		}
	}

	return &m_sqlMsgsModel;
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
	    "dbIDRecipient = :recipDbId"
	    " ORDER BY dmDeliveryTime ASC";
	//qDebug() << "Generating received year list" << recipDbId;
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
			// qDebug() << query.value(0).toString();
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
		    "(dbIDRecipient = :recipDbId)"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = :year)";
		qDebug() << queryStr;
		if (!query.prepare(queryStr)) {
			/* TODO -- Handle error. */
		}
		query.bindValue(":recipDbId", recipDbId);
		query.bindValue(":year", yearList[i]);
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
 * Return number of unread messages received within past 90
 *     days.
 */
int MessageDb::msgsRcvdUnreadWithin90Days(const QString &recipDbId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDRecipient = :recipDbId)"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))"
	    " and "
	    "(read_locally = 0)";
	//qDebug() << queryStr << recipDbId;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toInt();
	}

	return 0;
}


/* ========================================================================= */
/*
 * Return number of unread received messages in year.
 */
int MessageDb::msgsRcvdUnreadInYear(const QString &recipDbId,
    const QString &year) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDRecipient = :recipDbId)"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = :year)"
	    " and "
	    "(read_locally = 0)";
	//qDebug() << queryStr << recipDbId << year;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":recipDbId", recipDbId);
	query.bindValue(":year", year);
	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toInt();
	}

	return 0;
}


/* ========================================================================= */
/*
 * Return sent messages model.
 */
DbMsgsTblModel * MessageDb::msgsSntModel(const QString &sendDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE dbIDSender = :sendDbId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_MSGS_DB_ENTRY_TYPE);
	}

	return &m_sqlMsgsModel;
}


/* ========================================================================= */
/*
 * Return sent messages within past 90 days.
 */
DbMsgsTblModel * MessageDb::msgsSntWithin90DaysModel(
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
	    "(dbIDSender = :sendDbId)"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_MSGS_DB_ENTRY_TYPE);
	}

	return &m_sqlMsgsModel;
}


/* ========================================================================= */
/*
 * Return sent messages within given year.
 */
DbMsgsTblModel * MessageDb::msgsSntInYearModel(const QString &sendDbId,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDSender = :sendDbId)"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = :year)";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	query.bindValue(":year", year);
	query.exec(); /* TODO -- Handle error. */

	m_sqlMsgsModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlMsgsModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_MSGS_DB_ENTRY_TYPE);
	}

	return &m_sqlMsgsModel;
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
	    "dbIDSender = :sendDbId"
	    " ORDER BY dmDeliveryTime ASC";
	//qDebug() << "Generating received year list" << recipDbId;
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
			// qDebug() << query.value(0).toString();
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
		    "(dbIDSender = :sendDbId)"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = :year)";
		//qDebug() << queryStr;
		if (!query.prepare(queryStr)) {
			/* TODO -- Handle error. */
		}
		query.bindValue(":sendDbId", sendDbId);
		query.bindValue(":year", yearList[i]);
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
 * Return number of unread messages sent within past 90
 *     days.
 */
int MessageDb::msgsSntUnreadWithin90Days(const QString &sendDbId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDSender = :sendDbId)"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))"
	    " and "
	    "(read_locally = 0)";
	//qDebug() << queryStr << sendDbId;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toInt();
	}

	return 0;
}


/* ========================================================================= */
/*
 * Return number of unread sent messages in year.
 */
int MessageDb::msgsSntUnreadInYear(const QString &sendDbId,
    const QString &year) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages LEFT JOIN supplementary_message_data "
	    "ON (messages.dmID = supplementary_message_data.message_id) "
	    "WHERE "
	    "(dbIDSender = :sendDbId)"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = :year)"
	    " and "
	    "(read_locally = 0)";
	//qDebug() << queryStr << sendDbId << year;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":sendDbId", sendDbId);
	query.bindValue(":year", year);
	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toInt();
	}

	return 0;
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
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
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
 * Returns true if verification attempt was performed.
 */
bool MessageDb::msgsVerificationAttempted(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool ok;

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		/* If no value is set then the conversion will fail. */
		query.value(0).toInt(&ok);
		return ok;
	}

	return false;
}


/* ========================================================================= */
/*
 * Was message locally read.
 */
bool MessageDb::smsgdtLocallyRead(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT read_locally FROM supplementary_message_data "
	    "WHERE "
	    "message_id = :dmId";
	//qDebug() << queryStr << dmId;
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec()) {
		query.first();
		return query.value(0).toBool();
	}

	return false;
}


/* ========================================================================= */
/*
 * Set message status to locally read.
 */
bool MessageDb::smsgdtSetLocallyRead(int dmId, bool read)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "UPDATE supplementary_message_data "
	    "SET read_locally = :read WHERE "
	    "message_id = :dmId";
	qDebug() << queryStr << dmId;
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":read", read);
	query.bindValue(":dmId", dmId);
	return query.exec();
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
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
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
 * Return message delivery info HTML to PDF
 */
QString MessageDb::deliveryInfoHtmlToPdf(int dmId) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;
	QString tmp;

	html += indentDivStart;

	html += "<table width=\"100%\" style=\"padding: 30px 30px 30px 30px; font-size: 20px;\"><tr><td>" +
	    strongMessagePdf(tr("Advice of Delivery")) + "</td><td align=\"right\">" +
	    tr("Message ID: ") + strongMessagePdf(QString::number(dmId)) +
	    "</td></tr></table><br/><br/>";

	queryStr = "SELECT "
	    "dmSender, dmSenderAddress, dbIDSender, _dmType, "
	    "dmRecipient, dmRecipientAddress, dmDeliveryTime, dmAnnotation, "
	    "dmLegalTitleLaw, dmLegalTitleYear, dmLegalTitleSect, "
	    "dmLegalTitlePar, dmLegalTitlePoint, "
	    "dmRecipientRefNumber, dmRecipientIdent, "
	    "dmSenderRefNumber, dmSenderIdent, "
	    "dmToHands, dmPersonalDelivery, dmAllowSubstDelivery, "
	    "dmAcceptanceTime "
	    "FROM messages WHERE "
	    "dmID = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();

		/* Sender info */
		html += messageTableSectionPdf(tr("Sender"));
		html += messageTableInfoStartPdf();
		tmp = query.value(0).toString() + QString(", ") +
		    query.value(1).toString();
		html += messageTableInfoPdf(tr("Name"), tmp);
		html += messageTableInfoEndPdf();

		/* Recipient info */
		html += messageTableSectionPdf(tr("Recipient"));
		html += messageTableInfoStartPdf();
		tmp = query.value(4).toString() + QString(", ") +
		    query.value(5).toString();
		html += messageTableInfoPdf(tr("Name"), tmp);
		html += messageTableInfoEndPdf();

		/* General info */
		html += messageTableSectionPdf(tr("General Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(tr("Subject"),
		    query.value(7).toString());

		tmp = query.value(8).toString() + QString(" / ") +
		    query.value(9).toString() + QString(" § ") +
		    query.value(10).toString() + QString(tr(" paragraph ")) +
		    query.value(11).toString() + QString(tr(" letter ")) +
		    query.value(12).toString();

		html += messageTableInfoPdf(tr("Delegation"), tmp);

		(query.value(13).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(13).toString();
		html += messageTableInfoPdf(tr("Our ref.number"), tmp);
		(query.value(14).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(14).toString();
		html += messageTableInfoPdf(tr("Our doc.id"), tmp);
		(query.value(15).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(15).toString();
		html += messageTableInfoPdf(tr("Your ref.number"), tmp);
		(query.value(16).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(16).toString();
		html += messageTableInfoPdf(tr("Your doc.id"), tmp);
		(query.value(17).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(17).toString();
		html += messageTableInfoPdf(tr("To hands"), tmp);

		((query.value(18)).toInt()) ? tmp = tr("yes")
		: tmp = tr("no");
		html += messageTableInfoPdf(tr("Personal Delivery"), tmp);

		((query.value(19)).toInt()) ? tmp = tr("no")
		: tmp = tr("yes");
		html += messageTableInfoPdf(tr("Prohibit Delivery by Fiction"),
		    tmp);

		html += messageTableInfoEndPdf();

		/* Delivery info */
		html += messageTableSectionPdf(
		    tr("Delivery/Acceptance Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(tr("Delivery"),
		        dateTimeStrFromDbFormat(query.value(6).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoPdf(tr("Acceptance"),
		        dateTimeStrFromDbFormat(query.value(20).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoEndPdf();
	}

	queryStr = "SELECT dmEventTime, dmEventDescr "
	    "FROM events WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}

	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		/* Attachments info */
		html += messageTableSectionPdf(tr("Events"));
		html += messageTableInfoStartPdf();
		int i = 1;
		while (query.isValid()) {
			tmp = dateTimeStrFromDbFormat(query.value(0).toString(),
			    dateTimeDisplayFormat) + " - " +
			    query.value(1).toString();
			html += messageTableInfoPdf(tr("Time"), tmp);
			query.next();
			i++;
		}
		html += messageTableInfoEndPdf();
	}

	html += divEnd;

	return html;
}


/* ========================================================================= */
/*
 * Return message envelope HTML to PDF
 */
QString MessageDb::envelopeInfoHtmlToPdf(int dmId) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;
	QString tmp;

	html += indentDivStart;

	html += "<table width=\"100%\" style=\"padding: 30px 30px 30px 30px; font-size: 20px;\"><tr><td>" +
	    strongMessagePdf(tr("Envelope")) + "</td><td align=\"right\">" +
	    tr("Message ID: ") + strongMessagePdf(QString::number(dmId)) +
	    "</td></tr></table><br/><br/>";

	queryStr = "SELECT "
	    "dmSender, dmSenderAddress, dbIDSender, _dmType, "
	    "dmRecipient, dmRecipientAddress, dmDeliveryTime, dmAnnotation, "
	    "dmLegalTitleLaw, dmLegalTitleYear, dmLegalTitleSect, "
	    "dmLegalTitlePar, dmLegalTitlePoint, "
	    "dmRecipientRefNumber, dmRecipientIdent, "
	    "dmSenderRefNumber, dmSenderIdent, "
	    "dmToHands, dmPersonalDelivery, dmAllowSubstDelivery "
	    "FROM messages WHERE "
	    "dmID = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();

		/* Sender info */
		html += messageTableSectionPdf(tr("Sender"));
		html += messageTableInfoStartPdf();
		tmp = query.value(0).toString() + QString(", ") +
		    query.value(1).toString();
		html += messageTableInfoPdf(tr("Name"), tmp);
		html += messageTableInfoPdf(tr("Databox ID"),
		    query.value(2).toString());
		html += messageTableInfoPdf(tr("Databox Type"),
		    query.value(3).toString());
		html += messageTableInfoEndPdf();

		/* Recipient info */
		html += messageTableSectionPdf(tr("Recipient"));
		html += messageTableInfoStartPdf();
		tmp = query.value(4).toString() + QString(", ") +
		    query.value(5).toString();
		html += messageTableInfoPdf(tr("Name"), tmp);
		html += messageTableInfoPdf(tr("Delivery"),
		        dateTimeStrFromDbFormat(query.value(6).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoEndPdf();

		/* General info */
		html += messageTableSectionPdf(tr("General Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(tr("Subject"),
		    query.value(7).toString());

		tmp = query.value(8).toString() + QString(" / ") +
		    query.value(9).toString() + QString(" § ") +
		    query.value(10).toString() + QString(tr(" paragraph ")) +
		    query.value(11).toString() + QString(tr(" letter ")) +
		    query.value(12).toString();

		html += messageTableInfoPdf(tr("Delegation"), tmp);

		(query.value(13).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(13).toString();
		html += messageTableInfoPdf(tr("Our ref.number"), tmp);
		(query.value(14).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(14).toString();
		html += messageTableInfoPdf(tr("Our doc.id"), tmp);
		(query.value(15).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(15).toString();
		html += messageTableInfoPdf(tr("Your ref.number"), tmp);
		(query.value(16).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(16).toString();
		html += messageTableInfoPdf(tr("Your doc.id"), tmp);
		(query.value(17).toString()).isEmpty() ? tmp=tr("Not specified")
		: tmp = query.value(17).toString();
		html += messageTableInfoPdf(tr("To hands"), tmp);


		((query.value(18)).toInt()) ? tmp = tr("yes")
		: tmp = tr("no");
		html += messageTableInfoPdf(tr("Personal Delivery"), tmp);

		((query.value(19)).toInt()) ? tmp = tr("no")
		: tmp = tr("yes");
		html += messageTableInfoPdf(tr("Prohibit Delivery by Fiction"),
		    tmp);

		html += messageTableInfoEndPdf();
	}

	queryStr = "SELECT _dmFileDescr FROM files WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}

	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		/* Attachments info */
		html += messageTableSectionPdf(tr("Attachments"));
		html += messageTableInfoStartPdf();
		int i = 1;
		while (query.isValid()) {
			html += messageTableInfoPdf(QString::number(i),
			query.value(0).toString());
			query.next();
			i++;
		}
		html += messageTableInfoEndPdf();
	}

	html += divEnd;

	return html;
}


/* ========================================================================= */
/*
 * Return message HTML formatted description.
 */
QString MessageDb::descriptionHtml(int dmId, bool showId, bool warnOld) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;

	html += indentDivStart;
	html += "<h3>" + tr("Identification") + "</h3>";
	if (showId) {
		html += strongAccountInfoLine(tr("ID"), QString::number(dmId));
	}

	queryStr = "SELECT "
	    "dmAnnotation, _dmType, dmSender, dmSenderAddress, "
	    "dmRecipient, dmRecipientAddress"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
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

		html += "<br/>";

		html += strongAccountInfoLine(tr("To"),
		    query.value(4).toString());
		html += strongAccountInfoLine(tr("Recipient Address"),
		    query.value(5).toString());
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (msgAttribs2.size() - 1); ++i) {
		queryStr += msgAttribs2[i] + ", ";
	}
	queryStr += msgAttribs2.last();
	queryStr += " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		for (int i = 0; i < msgAttribs2.size(); ++i) {
			if (!query.value(i).toString().isEmpty()) {
				html += strongAccountInfoLine(
				    msgsTbl.attrProps[msgAttribs2[i]].desc,
				    query.value(i).toString());
			}
		}
	}

	html += "<h3>" + tr("Status") + "</h3>";
	/* Status. */
	queryStr = "SELECT ";
	for (int i = 0; i < (msgStatus.size() - 1); ++i) {
		queryStr += msgStatus[i] + ", ";
	}
	queryStr += msgStatus.last();
	queryStr += " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[0]].desc,
		    dateTimeStrFromDbFormat(query.value(0).toString(),
		        dateTimeDisplayFormat));
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[1]].desc,
		    dateTimeStrFromDbFormat(query.value(1).toString(),
		        dateTimeDisplayFormat));
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[2]].desc,
		    QString::number(query.value(2).toInt()) + " -- " +
		    msgStatusToText(query.value(2).toInt()));
	}
	/* Events. */
	queryStr = "SELECT "
	    "dmEventTime, dmEventDescr"
	    " FROM events WHERE "
	    "message_id = :dmId"
	    " ORDER BY dmEventTime ASC";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			html += strongAccountInfoLine(tr("Events"), "");
		}
		while (query.isValid()) {
			html += indentDivStart +
			    strongAccountInfoLine(
			        dateTimeStrFromDbFormat(
			            query.value(0).toString(),
			            dateTimeDisplayFormat),
			        query.value(1).toString()) +
			    divEnd;
			query.next();
		} 
	}
	/* Attachments. */
	queryStr = "SELECT COUNT(*) AS nrFiles "
	    " FROM files WHERE "
	    "message_id = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid() &&
	    (query.value(0).toInt() > 0)) {
		html += strongAccountInfoLine(tr("Attachments"), 
		    QString::number(query.value(0).toInt()) + " " +
		    tr("(downloaded and ready)"));
	} else {
		queryStr = "SELECT "
		    "dmAttachmentSize"
		    " FROM messages WHERE "
		    "dmID = :dmId";
		//qDebug() << queryStr;
		if (!query.prepare(queryStr)) {
			/* TODO -- Handle error. */
		}
		query.bindValue(":dmId", dmId);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid() &&
		   (query.value(0).toInt() > 0)) {
			html += strongAccountInfoLine(tr("Attachments"),
			    tr("not downloaded yet, ~") +
			    QString::number(query.value(0).toInt()) +
			    tr(" KB; use 'Download' to get them."));
		} else {
			html += strongAccountInfoLine(tr("Attachments"), 
			    tr("(not available)"));
		}
	}
	if (warnOld) {
		/* TODO */
	}

	html += "<h3>" + tr("Signature") + "</h3>";
	/* Signature. */
	if (!msgsVerificationAttempted(dmId)) {
		/* Verification no attempted. */
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Not present"));
		/* TODO -- Enable verification button. */
	} else if (!msgsVerified(dmId)) {
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Invalid")  + " -- " +
		    tr("Message signature and content do not correspond!"));
	} else {
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Valid"));
		/* Check signing certificate. */
		// qDebug() << msgsVerificationDate(dmId);
		bool verified = msgCertValidAtDate(dmId,
		    msgsVerificationDate(dmId), !globPref.check_crl);
		QString verifiedText = verified ? tr("Valid") : tr("Invalid");
		if (!globPref.check_crl) {
			verifiedText += " (" +
			    tr("Certificate revocation check is turned off!") +
			    ")";
		}
		html += strongAccountInfoLine(tr("Signing certificate"),
		    verifiedText);
		/* TODO */
	}

	/* Time-stamp. */
	QDateTime tst;
	bool valid = msgsCheckTimestamp(dmId, tst);
	QString timeStampStr;
	if (!tst.isValid()) {
		timeStampStr = tr("Not present");
	} else {
		timeStampStr = valid ? tr("Valid") : tr("Invalid");
		/* TODO -- Print time-stamp. */
	}
	html += strongAccountInfoLine(tr("Time-stamp"),
	    "TODO");

	/* TODO */

	html += divEnd;

	//html += QString::number(dmId);
	// qDebug() << html;
	/* TODO */

	return html;
}


/* ========================================================================= */
/*
 * Return files related to given message.
 */
QAbstractTableModel * MessageDb::flsModel(int msgId)
/* ========================================================================= */
{
	int i;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (i = 0; i < (fileItemIds.size() - 1); ++i) {
		queryStr += fileItemIds[i] + ", ";
	}
	queryStr += fileItemIds.last();
	queryStr += " FROM files WHERE "
	    "message_id = :msgId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":msgId", msgId);
	query.exec(); /* TODO -- Handle error. */

	/* First three columns ought to be hidden. */

	m_sqlFilesModel.setQuery(query);
	for (i = 0; i < fileItemIds.size(); ++i) {
		/* Description. */
		m_sqlFilesModel.setHeaderData(i, Qt::Horizontal,
		    flsTbl.attrProps.value(fileItemIds[i]).desc);
		/* Data type. */
		m_sqlFilesModel.setHeaderData(i, Qt::Horizontal,
		    flsTbl.attrProps.value(fileItemIds[i]).type,
		    ROLE_MSGS_DB_ENTRY_TYPE);
	}

	/* Rename last column to file size. */
	m_sqlFilesModel.setHeaderData(i - 1, Qt::Horizontal, tr("File Size"));

	return &m_sqlFilesModel;
}


/* ========================================================================= */
/*
 * Check if any message (dmID) exists in the table
 */
bool MessageDb::isInMessageDb(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool inMessages = false;
	bool inSupplementary = false;

	queryStr = "SELECT count(*) FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			Q_ASSERT(query.value(0).toInt() < 2);
			inMessages = (1 == query.value(0).toInt());
		}
	}

	/*
	 * Entry in supplementary_message_data must exist if exists in
	 * messages.
	 */
	queryStr = "SELECT count(*) FROM supplementary_message_data WHERE "
	    "message_id = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			Q_ASSERT(query.value(0).toInt() < 2);
			inSupplementary = (1 == query.value(0).toInt());
		}
	}

	Q_ASSERT(inMessages == inSupplementary);

	return inMessages && inSupplementary;
}


/* ========================================================================= */
/*
 * Get message hash from db
 */
QList<QString> MessageDb::msgsGetHashFromDb(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QList<QString> retitem;
	QString queryStr;

	queryStr = "SELECT value,_algorithm FROM hashes WHERE "
	    "message_id = :dmId";

	if (!query.prepare(queryStr)) {
		qDebug() << "Select hashes errro:" << query.lastError();
	}

	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			retitem.append(query.value(0).toString());
			retitem.append(query.value(1).toString());
			return retitem;
		}
	}
	retitem.append("");
	retitem.append("");
	return retitem;
}


/* ========================================================================= */
/*
 * Insert additional info about author (sender) into db
 */
bool MessageDb::addMessageAuthorInfo(int dmID, const QString &sender_type,
    const QString &sender_name)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QVariantMap map, map2;
	map2.insert("userType", sender_type);
	map2.insert("authorName", sender_name);
	map.insert("message_author", map2);
	QJsonObject object = QJsonObject::fromVariantMap(map);
	QJsonDocument document;
	document.setObject(object);
	QString  json = document.toJson(QJsonDocument::Compact);

	QString queryStr = "UPDATE supplementary_message_data SET "
	    "custom_data = :custom_data WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {
		qDebug() << "Update supplementary_message_data error:"
		    << query.lastError();
		return false;
	}

	query.bindValue(":dmId", dmID);
	query.bindValue(":custom_data", json);

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Update supplementary_message_data error:"
		    << query.lastError();
		return false;
	}
}

/* ========================================================================= */
/*
 * Insert/update message file into file table
 */
bool MessageDb::msgsInsertUpdateMessageFile(int dmId,
    const QString &dmFileDescr, const QString &dmUpFileGuid,
    const QString &dmFileGuid, const QString &dmMimeType,
    const QString &dmFormat, const QString &dmFileMetaType,
    const QString &dmEncodedContent)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int dbId;

	QString queryStr = "SELECT id FROM files WHERE "
	    "message_id = :message_id AND _dmFileDescr = :dmFileDescr "
	    "AND _dmMimeType = :dmMimeType";

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageFile"
		    << query.lastError();
		return false;
	}

	query.bindValue(":message_id", dmId);
	query.bindValue(":dmFileDescr", dmFileDescr);
	query.bindValue(":dmMimeType", dmMimeType);

	if (!query.exec()) {
		qDebug() << "Error: msgsInsertUpdateMessageFile"
		    << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = 0;
		}
	}

	if (dbId != 0) {
		queryStr = "UPDATE files SET "
		" _dmFileDescr = :_dmFileDescr, _dmUpFileGuid = :_dmUpFileGuid,"
		" _dmFileGuid = :_dmFileGuid, _dmMimeType = :_dmMimeType, "
		"_dmFormat = :_dmFormat, _dmFileMetaType = :_dmFileMetaType, "
		"dmEncodedContent = :dmEncodedContent "
		"WHERE id = :dbId";
	} else {
		queryStr = "INSERT INTO files ("
		    "message_id, _dmFileDescr, _dmUpFileGuid, _dmFileGuid, "
		    "_dmMimeType, _dmFormat, _dmFileMetaType, dmEncodedContent"
		    ") VALUES ("
		    ":message_id, :_dmFileDescr, :_dmUpFileGuid, :_dmFileGuid,"
		    " :_dmMimeType, :_dmFormat, :_dmFileMetaType, "
		    ":dmEncodedContent)";
	}

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageFile"
		    << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	query.bindValue(":_dmFileDescr", dmFileDescr);
	query.bindValue(":_dmUpFileGuid", dmUpFileGuid);
	query.bindValue(":_dmFileGuid", dmFileGuid);
	query.bindValue(":_dmMimeType", dmMimeType);
	query.bindValue(":_dmFormat", dmFormat);
	query.bindValue(":_dmFileMetaType", dmFileMetaType);
	query.bindValue(":dmEncodedContent", dmEncodedContent);
	if (dbId != 0) {
	    query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Error: msgsInsertUpdateMessageFile"
		    << query.lastError();
		return false;
	}
}


/* ========================================================================= */
/*
 * Insert message hash into hashes table
 */
bool MessageDb::msgsInsertUpdateMessageHash(int dmId, const QString &value,
    const QString &algorithm)
/* ========================================================================= */
{

	QSqlQuery query(m_db);
	int dbId;

	QString queryStr = "SELECT id FROM hashes WHERE "
	    "message_id = :message_id";

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageHash"
		    << query.lastError();
		return false;
	}

	query.bindValue(":message_id", dmId);

	if (!query.exec()) {
		qDebug() << "Error: msgsInsertUpdateMessageHash"
		    << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = 0;
		}
	}

	if (dbId != 0) {
		queryStr = "UPDATE hashes SET "
		"value = :value, _algorithm = :algorithm "
		"WHERE id = :dbId";
	} else {
		queryStr = "INSERT INTO hashes (message_id, value, _algorithm)"
		" VALUES (:dmId, :value, :algorithm)";

	}

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageHash"
		    << query.lastError();
		return false;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":value", value);
	query.bindValue(":algorithm", algorithm);
	if (dbId != 0) {
	    query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Error: msgsInsertUpdateMessageHash"
		    << query.lastError();
		return false;
	}
}


/* ========================================================================= */
/*
 * Insert raw message data into raw_message_data table
 */
bool MessageDb::msgsInsertUpdateMessageRaw(int dmId, const QString &raw,
	int message_type)
/* ========================================================================= */
{

	QSqlQuery query(m_db);
	int dbId;

	QString queryStr = "SELECT message_id FROM raw_message_data WHERE "
	    "message_id = :message_id";

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageRaw"
		    << query.lastError();
		return false;
	}

	query.bindValue(":message_id", dmId);

	if (!query.exec()) {
		qDebug() << "Error: msgsInsertUpdateMessageRaw"
		    << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = 0;
		}
	}

	if (dbId != 0) {
		queryStr = "UPDATE raw_message_data SET "
		"data = :data, message_type = :message_type "
		"WHERE message_id = :dbId";
	} else {
		queryStr = "INSERT INTO raw_message_data "
		"(message_id, message_type, data) "
		"VALUES (:dmId, :data, :message_type)";

	}

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageRaw"
		    << query.lastError();
		return false;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":data", raw);
	query.bindValue(":message_type", message_type);
	if (dbId != 0) {
	    query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Error: msgsInsertUpdateMessageRaw"
		    << query.lastError();
		return false;
	}
}


/* ========================================================================= */
/*
 * Insert raw delivery info into raw_delivery_info_data table
 */
bool MessageDb::msgsInsertUpdateDeliveryRaw(int dmId, const QString &raw)
/* ========================================================================= */
{

	QSqlQuery query(m_db);
	int dbId;

	QString queryStr = "SELECT message_id FROM raw_delivery_info_data WHERE"
	    " message_id = :message_id";

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateDeliveryRaw"
		    << query.lastError();
		return false;
	}

	query.bindValue(":message_id", dmId);

	if (!query.exec()) {
		qDebug() << "Error: msgsInsertUpdateDeliveryRaw"
		    << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = 0;
		}
	}

	if (dbId != 0) {
		queryStr = "UPDATE raw_delivery_info_data SET "
		"data = :data WHERE message_id = :dbId";
	} else {
		queryStr = "INSERT INTO raw_delivery_info_data "
		"(message_id, data) VALUES (:dmId, :data)";

	}

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateDeliveryRaw"
		    << query.lastError();
		return false;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":data", raw);
	if (dbId != 0) {
	    query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Error: msgsInsertUpdateDeliveryRaw"
		    << query.lastError();
		return false;
	}
}


/* ========================================================================= */
/*
 * Insert/update message event into events table
 */
bool MessageDb::msgsInsertUpdateMessageEvent(int dmId, const QString &dmEventTime,
    const QString &dmEventType, const QString &dmEventDescr)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int dbId;

	QString queryStr = "SELECT id FROM events WHERE "
	    "message_id = :message_id AND dmEventTime = :dmEventTime";

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageEvent"
		    << query.lastError();
		return false;
	}

	query.bindValue(":message_id", dmId);
	query.bindValue(":dmEventTime", dmEventTime);

	if (!query.exec()) {
		qDebug() << "Error: msgsInsertUpdateMessageEvent"
		    << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = 0;
		}
	}

	QString dmEventDescrType = dmEventType + dmEventDescr;

	if (dbId != 0) {
		queryStr = "UPDATE events SET "
		"dmEventTime = :dmEventTime, dmEventDescr = :dmEventDescr "
		"WHERE id = :dbId";
	} else {
		queryStr = "INSERT INTO events (message_id, dmEventTime, "
		    "dmEventDescr) VALUES (:dmId, :dmEventTime, :dmEventDescr)";
	}

	if (!query.prepare(queryStr)) {
		qDebug() << "Error: msgsInsertUpdateMessageEvent"
		    << query.lastError();
		return false;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":dmEventTime", dmEventTime);
	query.bindValue(":dmEventDescr", dmEventDescrType);
	if (dbId != 0) {
	    query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Error: msgsInsertUpdateMessageEvent"
		    << query.lastError();
		return false;
	}
}

/* ========================================================================= */
/*
 * Insert message envelope into messages table
 */
bool MessageDb::msgsInsertMessageEnvelope(int dmId, bool is_verified,
    const QString &_origin, const QString &dbIDSender,
    const QString &dmSender, const QString &dmSenderAddress,
    int dmSenderType, const QString &dmRecipient,
    const QString &dmRecipientAddress,
    const QString &dmAmbiguousRecipient,
    const QString &dmSenderOrgUnit, const QString &dmSenderOrgUnitNum,
    const QString &dbIDRecipient, const QString &dmRecipientOrgUnit,
    const QString &dmRecipientOrgUnitNum, const QString &dmToHands,
    const QString &dmAnnotation, const QString &dmRecipientRefNumber,
    const QString &dmSenderRefNumber, const QString &dmRecipientIdent,
    const QString &dmSenderIdent, const QString &dmLegalTitleLaw,
    const QString &dmLegalTitleYear, const QString &dmLegalTitleSect,
    const QString &dmLegalTitlePar, const QString &dmLegalTitlePoint,
    bool dmPersonalDelivery, bool dmAllowSubstDelivery,
    const QString &dmQTimestamp, const QString &dmDeliveryTime,
    const QString &dmAcceptanceTime, int dmMessageStatus,
    int dmAttachmentSize, const QString &_dmType, const QString messtype)
/* ========================================================================= */
{

	QSqlQuery query(m_db);

	QString queryStr = "INSERT INTO messages ("
	    "dmID, is_verified, _origin, dbIDSender, dmSender, "
	    "dmSenderAddress, dmSenderType, dmRecipient, "
	    "dmRecipientAddress, dmAmbiguousRecipient, dmSenderOrgUnit, "
	    "dmSenderOrgUnitNum, dbIDRecipient, dmRecipientOrgUnit, "
	    "dmRecipientOrgUnitNum, dmToHands, dmAnnotation, "
	    "dmRecipientRefNumber, dmSenderRefNumber, dmRecipientIdent, "
	    "dmSenderIdent, dmLegalTitleLaw, dmLegalTitleYear, "
	    "dmLegalTitleSect, dmLegalTitlePar, dmLegalTitlePoint, "
	    "dmPersonalDelivery, dmAllowSubstDelivery, dmQTimestamp, "
	    "dmDeliveryTime, dmAcceptanceTime, dmMessageStatus, "
	    "dmAttachmentSize, _dmType"
	    ") VALUES ("
	    ":dmId, :is_verified, :_origin, :dbIDSender, :dmSender, "
	    ":dmSenderAddress, :dmSenderType, :dmRecipient, "
	    ":dmRecipientAddress, :dmAmbiguousRecipient, :dmSenderOrgUnit, "
	    ":dmSenderOrgUnitNum, :dbIDRecipient, :dmRecipientOrgUnit, "
	    ":dmRecipientOrgUnitNum, :dmToHands, :dmAnnotation, "
	    ":dmRecipientRefNumber, :dmSenderRefNumber, :dmRecipientIdent, "
	    ":dmSenderIdent, :dmLegalTitleLaw, :dmLegalTitleYear, "
	    ":dmLegalTitleSect, :dmLegalTitlePar, :dmLegalTitlePoint,"
	    ":dmPersonalDelivery, :dmAllowSubstDelivery, :dmQTimestamp, "
	    ":dmDeliveryTime, :dmAcceptanceTime, :dmMessageStatus, "
	    ":dmAttachmentSize, :_dmType"
	    ")";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":is_verified", is_verified);
	query.bindValue(":_origin", _origin);
	query.bindValue(":dbIDSender", dbIDSender);
	query.bindValue(":dmSender", dmSender);
	query.bindValue(":dmSenderAddress", dmSenderAddress);
	query.bindValue(":dmSenderType", dmSenderType);
	query.bindValue(":dmRecipient", dmRecipient);
	query.bindValue(":dmRecipientAddress", dmRecipientAddress);
	query.bindValue(":dmAmbiguousRecipient", dmAmbiguousRecipient);
	query.bindValue(":dmSenderOrgUnit", dmSenderOrgUnit);
	query.bindValue(":dmSenderOrgUnitNum", dmSenderOrgUnitNum);
	query.bindValue(":dbIDRecipient", dbIDRecipient);
	query.bindValue(":dmRecipientOrgUnit", dmRecipientOrgUnit);
	query.bindValue(":dmRecipientOrgUnitNum", dmRecipientOrgUnitNum);
	query.bindValue(":dmToHands", dmToHands);
	query.bindValue(":dmAnnotation", dmAnnotation);
	query.bindValue(":dmRecipientRefNumber", dmRecipientRefNumber);
	query.bindValue(":dmSenderRefNumber", dmSenderRefNumber);
	query.bindValue(":dmRecipientIdent", dmRecipientIdent);
	query.bindValue(":dmSenderIdent", dmSenderIdent);
	query.bindValue(":dmLegalTitleLaw", dmLegalTitleLaw);
	query.bindValue(":dmLegalTitleYear", dmLegalTitleYear);
	query.bindValue(":dmLegalTitleSect", dmLegalTitleSect);
	query.bindValue(":dmLegalTitlePar", dmLegalTitlePar);
	query.bindValue(":dmLegalTitlePoint", dmLegalTitlePoint);
	query.bindValue(":dmPersonalDelivery", dmPersonalDelivery);
	query.bindValue(":dmAllowSubstDelivery", dmAllowSubstDelivery);
	query.bindValue(":dmQTimestamp", dmQTimestamp);
	query.bindValue(":dmDeliveryTime", dmDeliveryTime);
	query.bindValue(":dmAcceptanceTime", dmAcceptanceTime);
	query.bindValue(":dmMessageStatus", dmMessageStatus);
	query.bindValue(":dmAttachmentSize", dmAttachmentSize);
	query.bindValue(":_dmType", _dmType);

	if (!query.exec()) {
		qDebug() << "Insert messages error:"
		    << query.lastError();
		return false;
	}

	queryStr = "INSERT INTO supplementary_message_data ("
	    "message_id, message_type, read_locally, download_date, custom_data"
	    ") VALUES (:dmId, :message_type, :read_locally, :download_date,"
	    ":custom_data)";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}

	QDateTime current = QDateTime::currentDateTime();
	QString dmDownloadTime = qDateTimeToDbFormat(current);

	query.bindValue(":dmId", dmId);
	if (messtype == "received") {
		query.bindValue(":message_type", 1);
	} else {
		query.bindValue(":message_type", 2);
	}
	query.bindValue(":read_locally", false);
	query.bindValue(":download_date", dmDownloadTime);
	query.bindValue(":custom_data", "null");

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Insert supplementary_message_data error:"
		    << query.lastError();
		return false;
	}
}


/* ========================================================================= */
/*
 * Update exist message envelope/supplementary data in db
 */
bool MessageDb::msgsUpdateMessageEnvelope(int dmId, bool is_verified,
    const QString &_origin, const QString &dbIDSender,
    const QString &dmSender, const QString &dmSenderAddress,
    int dmSenderType, const QString &dmRecipient,
    const QString &dmRecipientAddress,
    const QString &dmAmbiguousRecipient,
    const QString &dmSenderOrgUnit, const QString &dmSenderOrgUnitNum,
    const QString &dbIDRecipient, const QString &dmRecipientOrgUnit,
    const QString &dmRecipientOrgUnitNum, const QString &dmToHands,
    const QString &dmAnnotation, const QString &dmRecipientRefNumber,
    const QString &dmSenderRefNumber, const QString &dmRecipientIdent,
    const QString &dmSenderIdent, const QString &dmLegalTitleLaw,
    const QString &dmLegalTitleYear, const QString &dmLegalTitleSect,
    const QString &dmLegalTitlePar, const QString &dmLegalTitlePoint,
    bool dmPersonalDelivery, bool dmAllowSubstDelivery,
    const QString &dmQTimestamp, const QString &dmDeliveryTime,
    const QString &dmAcceptanceTime, int dmMessageStatus,
    int dmAttachmentSize, const QString &_dmType,
    const QString messtype)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "UPDATE messages SET "
	    "is_verified = :is_verified, _origin = :_origin, "
	    "dbIDSender = :dbIDSender, dmSender = :dmSender, "
	    "dmSenderAddress = :dmSenderAddress, "
	    "dmSenderType = :dmSenderType, "
	    "dmRecipient = :dmRecipient, "
	    "dmRecipientAddress = :dmRecipientAddress, "
	    "dmAmbiguousRecipient = :dmAmbiguousRecipient, "
	    "dmSenderOrgUnit = :dmSenderOrgUnit, "
	    "dmSenderOrgUnitNum = :dmSenderOrgUnitNum, "
	    "dbIDRecipient = :dbIDRecipient, "
	    "dmRecipientOrgUnit = :dmRecipientOrgUnit, "
	    "dmRecipientOrgUnitNum = :dmRecipientOrgUnitNum, "
	    "dmToHands = :dmToHands, dmAnnotation = :dmAnnotation, "
	    "dmRecipientRefNumber = :dmRecipientRefNumber, "
	    "dmSenderRefNumber = :dmSenderRefNumber, "
	    "dmRecipientIdent = :dmRecipientIdent, "
	    "dmSenderIdent = :dmSenderIdent, "
	    "dmLegalTitleLaw = :dmLegalTitleLaw, "
	    "dmLegalTitleYear = :dmLegalTitleYear, "
	    "dmLegalTitleSect = :dmLegalTitleSect, "
	    "dmLegalTitlePar = :dmLegalTitlePar, "
	    "dmLegalTitlePoint = :dmLegalTitlePoint, "
	    "dmPersonalDelivery = :dmPersonalDelivery, "
	    "dmAllowSubstDelivery = :dmAllowSubstDelivery, "
	    "dmQTimestamp = :dmQTimestamp, "
	    "dmDeliveryTime = :dmDeliveryTime, "
	    "dmAcceptanceTime = :dmAcceptanceTime, "
	    "dmMessageStatus = :dmMessageStatus, "
	    "dmAttachmentSize = :dmAttachmentSize, "
	    "_dmType = :_dmType WHERE dmID = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":is_verified", is_verified);
	query.bindValue(":_origin", _origin);
	query.bindValue(":dbIDSender", dbIDSender);
	query.bindValue(":dmSender", dmSender);
	query.bindValue(":dmSenderAddress", dmSenderAddress);
	query.bindValue(":dmSenderType", dmSenderType);
	query.bindValue(":dmRecipient", dmRecipient);
	query.bindValue(":dmRecipientAddress", dmRecipientAddress);
	query.bindValue(":dmAmbiguousRecipient", dmAmbiguousRecipient);
	query.bindValue(":dmSenderOrgUnit", dmSenderOrgUnit);
	query.bindValue(":dmSenderOrgUnitNum", dmSenderOrgUnitNum);
	query.bindValue(":dbIDRecipient", dbIDRecipient);
	query.bindValue(":dmRecipientOrgUnit", dmRecipientOrgUnit);
	query.bindValue(":dmRecipientOrgUnitNum", dmRecipientOrgUnitNum);
	query.bindValue(":dmToHands", dmToHands);
	query.bindValue(":dmAnnotation", dmAnnotation);
	query.bindValue(":dmRecipientRefNumber", dmRecipientRefNumber);
	query.bindValue(":dmSenderRefNumber", dmSenderRefNumber);
	query.bindValue(":dmRecipientIdent", dmRecipientIdent);
	query.bindValue(":dmSenderIdent", dmSenderIdent);
	query.bindValue(":dmLegalTitleLaw", dmLegalTitleLaw);
	query.bindValue(":dmLegalTitleYear", dmLegalTitleYear);
	query.bindValue(":dmLegalTitleSect", dmLegalTitleSect);
	query.bindValue(":dmLegalTitlePar", dmLegalTitlePar);
	query.bindValue(":dmLegalTitlePoint", dmLegalTitlePoint);
	query.bindValue(":dmPersonalDelivery", dmPersonalDelivery);
	query.bindValue(":dmAllowSubstDelivery", dmAllowSubstDelivery);
	query.bindValue(":dmQTimestamp", dmQTimestamp);
	query.bindValue(":dmDeliveryTime", dmDeliveryTime);
	query.bindValue(":dmAcceptanceTime", dmAcceptanceTime);
	query.bindValue(":dmMessageStatus", dmMessageStatus);
	query.bindValue(":dmAttachmentSize", dmAttachmentSize);
	query.bindValue(":_dmType", _dmType);

	if (!query.exec()) {
		qDebug() << "Update messages error:" << query.lastError();
		return false;
	}

	queryStr = "UPDATE supplementary_message_data SET "
	    "message_type = :message_type, read_locally = :read_locally, "
	    "custom_data = :custom_data "
	    "WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {

	}

	query.bindValue(":dmId", dmId);
	if (messtype == "received") {
		query.bindValue(":message_type", 1);
	} else {
		query.bindValue(":message_type", 2);
	}
	query.bindValue(":read_locally", true);
	query.bindValue(":custom_data", "null");

	if (query.exec()) {
		return true;
	} else {
		qDebug() << "Update supplementary_message_data error:"
		    << query.lastError();
		return false;
	}
}

/* ========================================================================= */
/*
 * Create empty tables if tables do not already exist.
 */
void MessageDb::createEmptyMissingTables(void)
/* ========================================================================= */
{
	bool ret;

	if (!msgsTbl.existsInDb(this->m_db)) {
		ret = msgsTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!flsTbl.existsInDb(this->m_db)) {
		ret = flsTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!hshsTbl.existsInDb(this->m_db)) {
		ret = hshsTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!evntsTbl.existsInDb(this->m_db)) {
		ret = evntsTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!rwmsgdtTbl.existsInDb(this->m_db)) {
		ret = rwmsgdtTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!rwdlvrinfdtTbl.existsInDb(this->m_db)) {
		ret = rwdlvrinfdtTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!smsgdtTbl.existsInDb(this->m_db)) {
		ret = smsgdtTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!crtdtTbl.existsInDb(this->m_db)) {
		ret = crtdtTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
	if (!msgcrtdtTbl.existsInDb(this->m_db)) {
		ret = msgcrtdtTbl.createEmpty(this->m_db);
		Q_ASSERT(ret); /* TODO -- Proper check and recovery? */
	}
}


/* ========================================================================= */
/*
 * Returns whether message is verified.
 */
bool MessageDb::msgsVerified(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toBool();
	}

	return false;
}


/* ========================================================================= */
/*
 * Returns verification date.
 */
QDateTime MessageDb::msgsVerificationDate(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (GlobPreferences::DOWNLOAD_DATE ==
	    globPref.certificate_validation_date) {

		queryStr = "SELECT "
		    "download_date"
		    " FROM supplementary_message_data WHERE "
		    "message_id = :dmId";
		//qDebug() << queryStr;
		if (!query.prepare(queryStr)) {
			/* TODO -- Handle error. */
		}
		query.bindValue(":dmId", dmId);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid()) {
			// qDebug() << "dateTime" << query.value(0).toString();
			QDateTime dateTime =
			    dateTimeFromDbFormat(query.value(0).toString());

			if (dateTime.isValid()) {
				return dateTime;
			}
		}
	}

	return QDateTime::currentDateTime();
}


/* ========================================================================= */
/*
 * Returns time-stamp validity.
 */
bool MessageDb::msgsCheckTimestamp(int dmId, QDateTime &tst) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	QList<QSslCertificate> certs;

	tst = QDateTime();

	queryStr = "SELECT "
	    "dmQTimestamp"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		//qDebug() << "timestamp" << query.value(0).toString();
		QByteArray byteArray = query.value(0).toByteArray();
		if (byteArray.isEmpty()) {
			return false;
		}

		//qDebug() << QByteArray::fromBase64(byteArray);
		/*
		 * TODO -- Time from base64 encoded timestamp.
		 * RFC3161 (ASN.1 encoded).
		 * ISDS provozni rad, appendix 2 -- Manipulace s datovymi
		 * zpravami.
		 */

	}

	return false;
}


/* ========================================================================= */
/*
 * Delete message records from db
 */
bool MessageDb::msgsDeleteMessageData(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	/* Delete hash from hashes table */
	queryStr = "DELETE FROM hashes WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error1: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error1: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Delete file(s) from files table */
	queryStr = "DELETE FROM files WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error2: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error2: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Delete event(s) from events table */
	queryStr = "DELETE FROM events WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error3: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error3: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Delete raw message data from raw_message_data table */
	queryStr= "DELETE FROM raw_message_data WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error4: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error4: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Delete raw info data from raw_delivery_info_data table */
	queryStr = "DELETE FROM raw_delivery_info_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error5: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error5: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Delete supplementary from supplementary_message_data table */
	queryStr = "DELETE FROM supplementary_message_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error6: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error6: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Select certificate_id from message_certificate_data table */
	int certificate_id = 0;
	queryStr = "SELECT certificate_id FROM message_certificate_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error7: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error7: msgsDeleteMessageData" << query.lastError();
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			certificate_id = query.value(0).toInt();
		}
	}
	/* Delete certificate reference from message_certificate_data table */
	queryStr = "DELETE FROM message_certificate_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error8: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error8: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	/* Select certificate id from message_certificate_data table */
	bool deleteCert = true;
	queryStr = "SELECT count(*) FROM message_certificate_data WHERE "
	    "certificate_id = :certificate_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error9: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":certificate_id", certificate_id);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() > 0) {
				deleteCert = false;
			}
		}
	}

	/* Delete certificate data from certificate_data table if no messages */
	if (deleteCert) {
		queryStr = "DELETE FROM certificate_data WHERE "
		    "id = :certificate_id";
		if (!query.prepare(queryStr)) {
			qDebug() << "Error9: msgsDeleteMessageData"
			    << query.lastError();
			return false;
		}
		query.bindValue(":certificate_id", certificate_id);
		if (!query.exec()) {
			qDebug() << "Error9: msgsDeleteMessageData"
			    << query.lastError();
			return false;
		}
	}

	/* Delete message from messages table */
	queryStr = "DELETE FROM messages WHERE dmID = :message_id";
	if (!query.prepare(queryStr)) {
		qDebug() << "Error10: msgsDeleteMessageData" << query.lastError();
		return false;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		qDebug() << "Error10: msgsDeleteMessageData" << query.lastError();
		return false;
	}

	return true;
}


/* ========================================================================= */
/*
 * Get raw message data from raw_message_data table.
 */
QString MessageDb::msgsGetMessageRaw(int dmId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT data FROM raw_message_data WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}

	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toString();
	}
	return "";
}


/* ========================================================================= */
/*
 * Get raw delivery info from raw_delivery_info_data table.
 */
QString MessageDb::msgsGetDeliveryInfoRaw(int dmId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT data FROM raw_delivery_info_data WHERE message_id = :dmId";

	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}

	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		return query.value(0).toString();
	}
	return "";
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
	    "message_id = :msgId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":msgId", msgId);
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
 * Certificates related to given message.
 */
QList<QSslCertificate> MessageDb::msgCerts(int dmId) const
/* ========================================================================= */
{
	QList<QSslCertificate> certList;
	QSqlQuery query(m_db);
	QString queryStr;
	QList<int> certIds;

	queryStr = "SELECT "
	    "certificate_id"
	    " FROM message_certificate_data WHERE "
	    "message_id = :dmId";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		/* TODO -- Handle error. */
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			bool ok;
			int ret;
			ret = query.value(0).toInt(&ok);
			Q_ASSERT(ok);
			if (ok) {
				certIds.append(ret);
			}
		}
	}

	if (certIds.size() > 0) {
		queryStr = "SELECT "
		    "der_data"
		    " FROM certificate_data WHERE ";
		for (int i = 0; i < (certIds.size() - 1); ++i) {
			queryStr += "(id = ?) or ";
		}
		queryStr += "(id = ?)";
		//qDebug() << queryStr;
		if (!query.prepare(queryStr)) {
			/* TODO -- Handle error. */
		}
		for (int i = 0; i < certIds.size(); ++i) {
			query.addBindValue(certIds[i]);
		}
		if (query.exec() && query.isActive()) {
			query.first();
			while (query.isValid()) {
				QList<QSslCertificate> certs =
				    QSslCertificate::fromData(
				        QByteArray::fromBase64(
				            query.value(0).toByteArray()),
				        QSsl::Der);
				Q_ASSERT(1 == certs.size());

				certList.append(certs.first());

				query.next();
			}
		}
	}

	return certList;
}


/* ========================================================================= */
/*
 * Check whether message signature was valid at given date.
 */
bool MessageDb::msgCertValidAtDate(int dmId, const QDateTime &dateTime,
    bool ignoreMissingCrlCheck) const
/* ========================================================================= */
{
	QList<QSslCertificate> certList = msgCerts(dmId);

	if (certList.size() > 0) {
		Q_ASSERT(1 == certList.size());

		const QSslCertificate &cert = certList.first();

		bool timeValid = certTimeValidAtTime(cert, dateTime);
		bool crlValid = ignoreMissingCrlCheck; /* true; */
		if (globPref.check_crl) {
			crlValid = certCrlValidAtTime(cert, dateTime);
		}

		return timeValid && crlValid;
	}

	return false;
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
		// qDebug() << key << "db found";
		return (*this)[key];
	}

	// qDebug() << "creating new" << key;
	db = new MessageDb(key);

	// qDebug() << "searching for file" << key << "in" << locDir;
	/* TODO -- Handle file name deviations! */
	// qDebug() << "opening";
	/*
	 * Test accounts have ___1 in their names, ___0 relates to standard
	 * accounts.
	 */
	db->openDb(locDir + "/" + key + "___" + (testing ? "1" : "0") + ".db");

	this->insert(key, db);
	return db;
}


/* ========================================================================= */
/*
 * Delete message db.
 */
bool dbContainer::deleteMessageDb(MessageDb * deleted)
/* ========================================================================= */
{
	Q_ASSERT(0 != deleted);
	if (0 == deleted) {
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDb *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != deleted)) {
		++it;
	}
	/* Must exist. */
	Q_ASSERT(this->end() != it);
	if (this->end() == it) {
		return false;
	}

	/* Remove from container. */
	this->erase(it);

	/* Get file name. */
	QString fileName = deleted->fileName();

	/* Close database. */
	delete deleted;

	/* Delete file. */
	qDebug() << "Deleting database file" << fileName;

	if (!QFile::remove(fileName)) {
		qWarning() << "Failed deleting database file" << fileName;
		return false;
	}

	return true;
}
