/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */


#include <QAbstractTableModel>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
#include <QSqlRecord>
#include <QString>
#include <QTimeZone>
#include <QVariant>
#include <QVector>

#include "message_db.h"
#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"


const QVector<QString> MessageDb::msgPrintedAttribs = {"dmSenderIdent",
    "dmSenderRefNumber", "dmRecipientIdent", "dmRecipientRefNumber",
    "dmToHands", "dmLegalTitleLaw", "dmLegalTitleYear", "dmLegalTitleSect",
    "dmLegalTitlePar", "dmLegalTitlePoint"};

const QVector<QString> MessageDb::msgStatus = {"dmDeliveryTime",
    "dmAcceptanceTime", "dmMessageStatus"};

const QVector<QString> MessageDb::fileItemIds = {"id", "message_id",
    "dmEncodedContent", "_dmFileDescr", "LENGTH(dmEncodedContent)"};

/* ========================================================================= */
MessageDb::MessageDb(const QString &dbDriverType,
    const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_sqlMsgsModel(),
    m_sqlFilesModel()
{
	m_db = QSqlDatabase::addDatabase(dbDriverType, connectionName);
}


/* ========================================================================= */
MessageDb::~MessageDb(void)
/* ========================================================================= */
{
	m_db.close();
}


const QString MessageDb::memoryLocation(":memory:");


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
 * Begin a transaction.
 */
bool MessageDb::beginTransaction(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr("BEGIN DEFERRED TRANSACTION");
	if (!query.prepare(queryStr)) {
		return false;
	}
	return query.exec();
}


/* ========================================================================= */
/*
 * End a transaction.
 */
bool MessageDb::commitTransaction(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr("COMMIT TRANSACTION");
	if (!query.prepare(queryStr)) {
		return false;
	}
	return query.exec();
}


/* ========================================================================= */
/*
 * Begin named transaction.
 */
bool MessageDb::savePoint(const QString &savePointName)
/* ========================================================================= */
{
	if (savePointName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSqlQuery query(m_db);
	QString queryStr("SAVEPOINT :name");
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":name", savePointName);
	return query.exec();
}


/* ========================================================================= */
/*
 * End named transaction.
 */
bool MessageDb::releaseSavePoint(const QString &savePointName)
/* ========================================================================= */
{
	if (savePointName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSqlQuery query(m_db);
	QString queryStr("RELEASE SAVEPOINT :name");
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":name", savePointName);
	return query.exec();
}


/* ========================================================================= */
/*
 * Roll back transaction.
 */
bool MessageDb::rollbackTransaction(const QString &savePointName)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	if (savePointName.isEmpty()) {

		QString queryStr("ROLLBACK TRANSACTION");
		if (!query.prepare(queryStr)) {
			return false;
		}
		return query.exec();

	} else {

		QString queryStr("ROLLBACK TRANSACTION TO SAVEPOINT :name");
		if (!query.prepare(queryStr)) {
			return false;
		}
		query.bindValue(":name", savePointName);
		return query.exec();

	}
}


/* ========================================================================= */
/*
 * Attaches a database file to opened database.
 */
bool MessageDb::attachDb2(QSqlDatabase &db, const QString &attachFileName)
/* ========================================================================= */
{
	QSqlQuery query(db);

	QString queryStr("ATTACH DATABASE :fileName AS " DB2);
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":fileName", attachFileName);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Detaches attached database file from opened database.
 */
bool MessageDb::detachDb2(QSqlDatabase &db)
/* ========================================================================= */
{
	QSqlQuery query(db);

	QString queryStr = "DETACH DATABASE " DB2;
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Return all received messages model.
 */
DbMsgsTblModel * MessageDb::msgsRcvdModel(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::rcvdItemIds().size() - 2); ++i) {
		queryStr += DbMsgsTblModel::rcvdItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded" ", "
	    "ifnull(p.state, 0) AS process_status"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "LEFT JOIN process_state AS p "
	    "ON (m.dmId = p.message_id) "
	    "WHERE "
	    "s.message_type = :message_type";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_RECEIVED);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setRcvdHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days.
 */
DbMsgsTblModel * MessageDb::msgsRcvdWithin90DaysModel(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::rcvdItemIds().size() - 2); ++i) {
		queryStr += DbMsgsTblModel::rcvdItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded" ", "
	    "ifnull(p.state, 0) AS process_status"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "LEFT JOIN process_state AS p "
	    "ON (m.dmId = p.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(m.dmDeliveryTime >= date('now','-90 day'))";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_RECEIVED);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setRcvdHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Return received messages within given year.
 */
DbMsgsTblModel * MessageDb::msgsRcvdInYearModel(const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::rcvdItemIds().size() - 2); ++i) {
		queryStr += DbMsgsTblModel::rcvdItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded" ", "
	    "ifnull(p.state, 0) AS process_status"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "LEFT JOIN process_state AS p "
	    "ON (m.dmId = p.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_RECEIVED);
	query.bindValue(":year", year);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setRcvdHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QStringList MessageDb::msgsYears(enum MessageDb::MessageType type,
    enum Sorting sorting) const
/* ========================================================================= */
{
	QStringList yearList;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE "
	    "s.message_type = :message_type";
//	if (TYPE_SENT == type) {
//		queryStr += " and "
//		    "(m.dmDeliveryTime IS NOT NULL)";
//	}
	switch (sorting) {
	case ASCENDING:
		queryStr += " ORDER BY dmDeliveryTime ASC";
		break;
	case DESCENDING:
		queryStr += " ORDER BY dmDeliveryTime DESC";
		break;
	default:
		break;
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", type);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
			yearList.append(query.value(0).toString());
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
	}

fail:
	return yearList;
}


/* ========================================================================= */
/*
 * Return list of years and number of messages in database.
 */
QList< QPair<QString, int> > MessageDb::msgsYearlyCounts(enum MessageType type,
    enum Sorting sorting) const
/* ========================================================================= */
{
	QList< QPair<QString, int> > yearlyCounts;
	QList<QString> yearList = msgsYears(type, sorting);
	QSqlQuery query(m_db);
	QString queryStr;

	for (int i = 0; i < yearList.size(); ++i) {
		queryStr = "SELECT COUNT(*) AS nrRecords "
		    "FROM messages AS m "
		    "LEFT JOIN supplementary_message_data AS s "
		    "ON (m.dmID = s.message_id) "
		    "WHERE "
		    "(s.message_type = :message_type)"
		    " and "
		    "(ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year)";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":message_type", type);
		query.bindValue(":year", yearList[i]);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid()) {
			yearlyCounts.append(QPair<QString, int>(yearList[i],
			    query.value(0).toInt()));
		} else {
			logErrorNL(
			    "Cannot execute SQL query and/or read SQL data: "
			    "%s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}

	return yearlyCounts;

fail:
	yearlyCounts.clear();
	return yearlyCounts;
}


/* ========================================================================= */
/*
 * Return number of unread messages received within past 90
 *     days.
 */
int MessageDb::msgsUnreadWithin90Days(enum MessageType type) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(m.dmDeliveryTime >= date('now','-90 day'))"
	    " and "
	    "(read_locally = 0)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", type);
	if (query.exec() && query.isActive() &&
	    query.first() &&query.isValid()) {
		return query.value(0).toInt();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Return number of unread received messages in year.
 */
int MessageDb::msgsUnreadInYear(enum MessageType type,
    const QString &year) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT COUNT(*) AS nrUnread "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year)"
	    " and "
	    "(read_locally = 0)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", type);
	query.bindValue(":year", year);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toInt();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Return all sent messages model.
 */
DbMsgsTblModel * MessageDb::msgsSntModel(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::sntItemIds().size() - 1); ++i) {
		queryStr += DbMsgsTblModel::sntItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "WHERE "
	    "s.message_type = :message_type";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_SENT);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setSntHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Return sent messages within past 90 days.
 */
DbMsgsTblModel * MessageDb::msgsSntWithin90DaysModel(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::sntItemIds().size() - 1); ++i) {
		queryStr += DbMsgsTblModel::sntItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(m.dmDeliveryTime >= date('now','-90 day'))";
//	    "((m.dmDeliveryTime >= date('now','-90 day')) or "
//	    " (m.dmDeliveryTime IS NULL))";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_SENT);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setSntHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Return sent messages within given year.
 */
DbMsgsTblModel * MessageDb::msgsSntInYearModel(const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (DbMsgsTblModel::sntItemIds().size() - 1); ++i) {
		queryStr += DbMsgsTblModel::sntItemIds()[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded"
	    " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", TYPE_SENT);
	query.bindValue(":year", year);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	m_sqlMsgsModel.clearOverridingData();
	m_sqlMsgsModel.setQuery(query);
	if (!m_sqlMsgsModel.setSntHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	return &m_sqlMsgsModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Get message information for reply/template send message dialog.
 */
QVector<QString> MessageDb::msgsReplyData(qint64 dmId) const
/* ========================================================================= */
{
	QVector<QString> msgData(21);
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "dbIDSender, dmSender, dmSenderAddress, dmSenderType, "
	    "dbIDRecipient, dmRecipient, dmRecipientAddress, "
	    "dmAnnotation, "
	    "dmSenderRefNumber, dmSenderIdent, "
	    "dmRecipientRefNumber, dmRecipientIdent, "
	    "dmToHands, dmPersonalDelivery, dmAllowSubstDelivery, "
	    "dmLegalTitleLaw, dmLegalTitleYear, dmLegalTitleSect, "
	    "dmLegalTitlePar, dmLegalTitlePoint, "
	    "_dmType "
	    "FROM messages WHERE dmID = :dmId";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		msgData[0] = query.value(0).toString();
		msgData[1] = query.value(1).toString();
		msgData[2] = query.value(2).toString();
		msgData[3] = query.value(3).toString();
		msgData[4] = query.value(4).toString();
		msgData[5] = query.value(5).toString();
		msgData[6] = query.value(6).toString();
		msgData[7] = query.value(7).toString();
		msgData[8] = query.value(8).toString();
		msgData[9] = query.value(9).toString();
		msgData[10] = query.value(10).toString();
		msgData[11] = query.value(11).toString();
		msgData[12] = query.value(12).toString();
		if (query.value(13).toBool()) {
			msgData[13] = "1";
		} else {
			msgData[13] = "0";
		}
		if (query.value(14).toBool()) {
			msgData[14] = "1";
		} else {
			msgData[14] = "0";
		}
		msgData[15] = query.value(15).toString();
		msgData[16] = query.value(16).toString();
		msgData[17] = query.value(17).toString();
		msgData[18] = query.value(18).toString();
		msgData[19] = query.value(19).toString();
		msgData[20] = query.value(20).toString();
		return msgData;
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QVector<QString>();
}


/* ========================================================================= */
/*
 * Returns true if verification attempt was performed.
 */
bool MessageDb::msgsVerificationAttempted(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	debugFuncCall();

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		/* If no value is set then the conversion will fail. */
		bool ret = ! query.value(0).isNull();
		return ret;
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Returns whether message is verified.
 */
bool MessageDb::msgsVerified(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toBool();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Returns whether message was read locally.
 */
bool MessageDb::smsgdtLocallyRead(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT read_locally FROM supplementary_message_data "
	    "WHERE "
	    "message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toBool();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set message read locally status.
 */
bool MessageDb::smsgdtSetLocallyRead(qint64 dmId, bool read)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "UPDATE supplementary_message_data "
	    "SET read_locally = :read WHERE "
	    "message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":read", read);
	query.bindValue(":dmId", dmId);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set message read locally for all messages.
 */
bool MessageDb::smsgdtSetAllReceivedLocallyRead(bool read)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "UPDATE supplementary_message_data "
	    "SET read_locally = :read WHERE "
	    "message_type = :message_type";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":read", read);
	query.bindValue(":message_type", TYPE_RECEIVED);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set message read locally for received messages in given year.
 */
bool MessageDb::smsgdtSetReceivedYearLocallyRead(const QString &year,
    bool read)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "INSERT OR REPLACE INTO supplementary_message_data ("
	    "message_id, message_type, read_locally, custom_data)"
	    " SELECT s.message_id, s.message_type, :read, s.custom_data "
	    "FROM supplementary_message_data AS s "
	    "LEFT JOIN messages AS m ON (s.message_id = m.dmID) "
	    "WHERE (ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year) and "
	    "(s.message_type = :message_type)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":read", read);
	query.bindValue(":year", year);
	query.bindValue(":message_type", TYPE_RECEIVED);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set message read locally for recently received messages.
 */
bool MessageDb::smsgdtSetWithin90DaysReceivedLocallyRead(bool read)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "INSERT OR REPLACE INTO supplementary_message_data ("
	    "message_id, message_type, read_locally, custom_data)"
	    " SELECT s.message_id, s.message_type, :read, s.custom_data "
	    "FROM supplementary_message_data AS s "
	    "LEFT JOIN messages AS m ON (s.message_id = m.dmID) "
	    "WHERE (m.dmDeliveryTime >= date('now','-90 day')) and "
	    "(s.message_type = :message_type)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":read", read);
	query.bindValue(":message_type", TYPE_RECEIVED);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Returns message identifier of message with given id number.
 */
MessageDb::MsgId MessageDb::msgsMsgId(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	MsgId ret;

	QString queryStr = "SELECT dmID, dmDeliveryTime FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		ret.dmId = query.value(0).toLongLong();
		ret.deliveryTime = dateTimeFromDbFormat(
		    query.value(1).toString());
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return ret;
}


/* ========================================================================= */
/*
 * Return contact list from message db.
 */
QList<MessageDb::ContactEntry> MessageDb::uniqueContacts(void) const
/* ========================================================================= */
{
	QMap<QString, ContactEntry> mapOfBoxes;
	QMap<QString, ContactEntry>::iterator it;
	QList<ContactEntry> contactList;
	QSqlQuery query(m_db);

	QString queryStr = "SELECT m.dmID AS id, m.dbIDRecipient, "
	    "m.dmRecipient, m.dmRecipientAddress "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type_sent)"
	    " and "
	    "(m.dmRecipientAddress IS NOT NULL)"
	    " UNION "
	    "SELECT m.dmID AS id, m.dbIDSender, m.dmSender, "
	    "m.dmSenderAddress "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type_received)"
	    " and "
	    "(m.dmSenderAddress IS NOT NULL)"
	    " ORDER BY m.dmID DESC";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type_sent", TYPE_SENT);
	query.bindValue(":message_type_received", TYPE_RECEIVED);
	if (query.exec() && query.isActive()) {
		query.first();
		ContactEntry newEntry;
		while (query.isValid()) {
			newEntry.dmId = query.value(0).toLongLong();
			newEntry.boxId = query.value(1).toString();
			newEntry.name = query.value(2).toString();
			newEntry.address = query.value(3).toString();
			QMap<QString, ContactEntry>::iterator found = mapOfBoxes.find(newEntry.boxId);
			if (mapOfBoxes.end() == found) {
				mapOfBoxes.insert(newEntry.boxId, newEntry);
			} else if (found->dmId < newEntry.dmId) {
				/* Prefer entries with larger ids. */
				*found = newEntry;
			}
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	foreach (const ContactEntry &entry, mapOfBoxes) {
		contactList.append(entry);
	}

fail:
	return contactList;
}


/* ========================================================================= */
/*
 * Return HTML formatted message description.
 */
QString MessageDb::descriptionHtml(qint64 dmId, QAbstractButton *verSigButton,
    bool showId, bool verSignature, bool warnOld) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;

	html += indentDivStart;
	html += "<h3>" + QObject::tr("Identification") + "</h3>";
	if (showId) {
		html += strongAccountInfoLine(QObject::tr("Message ID"),
		    QString::number(dmId));
	}

	queryStr = "SELECT "
	    "dmAnnotation, _dmType, dmSender, dmSenderAddress, "
	    "dmRecipient, dmRecipientAddress, dbIDSender, dbIDRecipient"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		html += strongAccountInfoLine(QObject::tr("Subject"),
		    query.value(0).toString());
		if (!query.value(1).toString().isEmpty() &&
		    (!dmTypeToText(query.value(1).toString()).isEmpty())) {
			html += strongAccountInfoLine(QObject::tr("Message type"),
			    dmTypeToText(query.value(1).toString()));
		}

		html += "<br/>";

		/* Information about message author. */
		html += strongAccountInfoLine(QObject::tr("Sender"),
		    query.value(2).toString());
		html += strongAccountInfoLine(QObject::tr("Sender Databox ID"),
		    query.value(6).toString());
		html += strongAccountInfoLine(QObject::tr("Sender Address"),
		    query.value(3).toString());
		/* Custom data. */
		QJsonDocument customData = smsgdCustomData(dmId);
		if (!customData.isEmpty() && customData.isObject()) {
			QJsonValue value =
			    customData.object().value("message_author");
			if (value.isObject()) {
				QString authorInfo = value.toObject().value(
				        "authorName").toString();
				if (!authorInfo.isEmpty()) {
					authorInfo += ", ";
				}
				html += strongAccountInfoLine(
				    QObject::tr("Message author"),
				    authorInfo +
				    authorTypeToText(value.toObject().value(
				        "userType").toString()));
			}
		}

		html += "<br/>";

		html += strongAccountInfoLine(QObject::tr("Recipient"),
		    query.value(4).toString());
		html += strongAccountInfoLine(
		    QObject::tr("Recipient Databox ID"),
		    query.value(7).toString());
		html += strongAccountInfoLine(QObject::tr("Recipient Address"),
		    query.value(5).toString());
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (msgPrintedAttribs.size() - 1); ++i) {
		queryStr += msgPrintedAttribs[i] + ", ";
	}
	queryStr += msgPrintedAttribs.last();
	queryStr += " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		for (int i = 0; i < msgPrintedAttribs.size(); ++i) {
			if (!query.value(i).toString().isEmpty()) {
				html += strongAccountInfoLine(
				    msgsTbl.attrProps[msgPrintedAttribs[i]].desc,
				    query.value(i).toString());
			}
		}
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	html += "<h3>" + QObject::tr("Status") + "</h3>";
	/* Status. */
	queryStr = "SELECT ";
	for (int i = 0; i < (msgStatus.size() - 1); ++i) {
		queryStr += msgStatus[i] + ", ";
	}
	queryStr += msgStatus.last();
	queryStr += " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
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
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	/* Events. */
	queryStr = "SELECT "
	    "dmEventTime, dmEventDescr"
	    " FROM events WHERE "
	    "message_id = :dmId"
	    " ORDER BY dmEventTime ASC";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			html += strongAccountInfoLine(QObject::tr("Events"),
			    "");
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
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	/* Attachments. */
	queryStr = "SELECT COUNT(*) AS nrFiles "
	    " FROM files WHERE "
	    "message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid() &&
	    (query.value(0).toInt() > 0)) {
		html += strongAccountInfoLine(QObject::tr("Attachments"),
		    QString::number(query.value(0).toInt()) + " " +
		    QObject::tr("(downloaded and ready)"));
	} else {
		queryStr = "SELECT "
		    "dmAttachmentSize"
		    " FROM messages WHERE "
		    "dmID = :dmId";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":dmId", dmId);
		if (query.exec() && query.isActive()) {
			query.first();
			if (query.isValid() && (query.value(0).toInt() > 0)) {
			html += strongAccountInfoLine(
			    QObject::tr("Attachments"),
			    QObject::tr("not downloaded yet, ~") +
			    QString::number(query.value(0).toInt()) +
			    QObject::tr(" KB; use 'Download' to get them."));
			} else {
				html += strongAccountInfoLine(
				    QObject::tr("Attachments"),
				    QObject::tr("(not available)"));
			}
		} else {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}
	if (warnOld) {
		/* TODO */
	}

	/* Disable verify signature button. It is re-enabled when needed. */
	if (0 != verSigButton) {
		verSigButton->setEnabled(false);
	}

	if (verSignature) {

		html += "<h3>" + QObject::tr("Signature") + "</h3>";
		/* Signature. */
		if (!msgsVerificationAttempted(dmId)) {
			/* Verification no attempted. */
			html += strongAccountInfoLine(
			    QObject::tr("Message signature"),
			    QObject::tr("Not present"));
			/* Enable verification button. */
			if (0 != verSigButton) {
				verSigButton->setEnabled(true);
			}
		} else if (!msgsVerified(dmId)) {
			html += strongAccountInfoLine(
			    QObject::tr("Message signature"),
			    QObject::tr("Invalid")  + " -- " +
			    QObject::tr("Message signature and content "
			        "do not correspond!"));
		} else {
			html += strongAccountInfoLine(
			    QObject::tr("Message signature"),
			    QObject::tr("Valid"));
			/* Check signing certificate. */
			bool verified = msgCertValidAtDate(dmId,
			    msgsVerificationDate(dmId), !globPref.check_crl);
			QString verifiedText = verified ?
			    QObject::tr("Valid") : QObject::tr("Invalid");
			if (!globPref.check_crl) {
				verifiedText += " (" +
				    QObject::tr("Certificate revocation "
				        "check is turned off!") +
				    ")";
			}
			html += strongAccountInfoLine(
			    QObject::tr("Signing certificate"), verifiedText);
		}

		{
			/* Time-stamp. */
			QDateTime tst;
			QByteArray tstData = msgsTimestampRaw(dmId);
			QString timeStampStr;
			if (tstData.isEmpty()) {
				timeStampStr = QObject::tr("Not present");
			} else {
				time_t utc_time = 0;
				int ret = raw_tst_verify(tstData.data(),
				    tstData.size(), &utc_time);

				if (-1 != ret) {
					tst = QDateTime::fromTime_t(utc_time);
				}

				timeStampStr = (1 == ret) ?
				    QObject::tr("Valid") :
				    QObject::tr("Invalid");
				if (-1 != ret) {
					timeStampStr +=
					    " (" +
					    tst.toString("dd.MM.yyyy hh:mm:ss")
					    + " " +
					    tst.timeZone().abbreviation(tst) +
					    ")";
				}
			}
			html += strongAccountInfoLine(
			    QObject::tr("Time stamp"), timeStampStr);
		}

	}

	html += divEnd;

	return html;

fail:
	return QString();
}


/* ========================================================================= */
/*
 * Return message envelope HTML to be used to generate a PDF.
 */
QString MessageDb::envelopeInfoHtmlToPdf(qint64 dmId,
    const QString &dbType) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;
	QString tmp;

	html += indentDivStart;

	html += "<table width=\"100%\" style=\"padding: 30px 30px 30px 30px; "
	    "font-size: 20px;\"><tr><td>" +
	    strongMessagePdf(QObject::tr("Envelope")) +
	    "</td><td align=\"right\">" +
	    QObject::tr("Message ID:") + " " +
	    strongMessagePdf(QString::number(dmId)) +
	    "</td></tr></table><br/><br/>";

	queryStr = "SELECT "
	    "dmSender, dmSenderAddress, dbIDSender, _dmType, "
	    "dmRecipient, dmRecipientAddress, dmDeliveryTime, dmAnnotation, "
	    "dmLegalTitleLaw, dmLegalTitleYear, dmLegalTitleSect, "
	    "dmLegalTitlePar, dmLegalTitlePoint, "
	    "dmRecipientRefNumber, dmRecipientIdent, "
	    "dmSenderRefNumber, dmSenderIdent, "
	    "dmToHands, dmPersonalDelivery, dmAllowSubstDelivery, "
	    "dbIDRecipient "
	    "FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {

		/* Sender info */
		html += messageTableSectionPdf(QObject::tr("Sender"));
		html += messageTableInfoStartPdf();
		tmp = query.value(0).toString() + QString(", ") +
		    query.value(1).toString();
		html += messageTableInfoPdf(QObject::tr("Name"), tmp);
		html += messageTableInfoPdf(QObject::tr("Databox ID"),
		    query.value(2).toString());
		html += messageTableInfoPdf(QObject::tr("Databox Type"),
		    dbType);
		html += messageTableInfoEndPdf();

		/* Recipient info */
		html += messageTableSectionPdf(QObject::tr("Recipient"));
		html += messageTableInfoStartPdf();
		tmp = query.value(4).toString() + QString(", ") +
		    query.value(5).toString();
		html += messageTableInfoPdf(QObject::tr("Name"), tmp);
		html += messageTableInfoPdf(QObject::tr("Databox ID"),
		    query.value(20).toString());
		html += messageTableInfoPdf(QObject::tr("Delivery"),
		        dateTimeStrFromDbFormat(query.value(6).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoEndPdf();

		/* General info */
		html += messageTableSectionPdf(
		    QObject::tr("General Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(QObject::tr("Subject"),
		    query.value(7).toString());

		if (query.value(8).toString().isEmpty()) {
			tmp = "0";
		} else {
			tmp = query.value(8).toString();
		}
		tmp += QString(" / ");
		if (query.value(9).toString().isEmpty()) {
			tmp += "0";
		} else {
			tmp += query.value(9).toString();
		}
		tmp += QString(" § ") + " " +
		    query.value(10).toString() + " " +
		    QString(QObject::tr("paragraph")) + " " +
		    query.value(11).toString() + " " +
		    QString(QObject::tr("letter")) + " " +
		    query.value(12).toString();

		html += messageTableInfoPdf(QObject::tr("Delegation"), tmp);

		if (query.value(13).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(13).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Our ref.number"),
		    tmp);
		if (query.value(14).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(14).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Our doc.id"), tmp);
		if (query.value(15).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(15).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Your ref.number"),
		    tmp);
		if (query.value(16).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(16).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Your doc.id"), tmp);
		if (query.value(17).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(17).toString();
		}
		html += messageTableInfoPdf(QObject::tr("To hands"), tmp);


		if (query.value(18).toInt()) {
			tmp = QObject::tr("yes");
		} else {
			tmp = QObject::tr("no");
		}
		html += messageTableInfoPdf(QObject::tr("Personal Delivery"),
		    tmp);

		if (query.value(19).toInt()) {
			tmp = QObject::tr("no");
		} else {
			tmp = QObject::tr("yes");
		}
		html += messageTableInfoPdf(
		    QObject::tr("Prohibit Delivery by Fiction"), tmp);

		html += messageTableInfoEndPdf();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "SELECT _dmFileDescr FROM files WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		/* Attachments info */
		html += messageTableSectionPdf(QObject::tr("Attachments"));
		html += messageTableInfoStartPdf();
		int i = 1;
		while (query.isValid()) {
			html += messageTableInfoPdf(QString::number(i),
			query.value(0).toString());
			query.next();
			i++;
		}
		html += messageTableInfoEndPdf();
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	html += divEnd;

	return html;

fail:
	return QString();
}


/* ========================================================================= */
/*
 * Return message delivery info HTML to PDF
 */
QString MessageDb::deliveryInfoHtmlToPdf(qint64 dmId) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;
	QString tmp;

	html += indentDivStart;

	html += "<table width=\"100%\" style=\"padding: 30px 30px 30px 30px; "
	    "font-size: 20px;\"><tr><td>" +
	    strongMessagePdf(QObject::tr("Advice of Delivery")) +
	    "</td><td align=\"right\">" +
	    QObject::tr("Message ID:") + " " +
	    strongMessagePdf(QString::number(dmId)) +
	    "</td></tr></table><br/><br/>";

	queryStr = "SELECT "
	    "dmSender, dmSenderAddress, dbIDSender, _dmType, "
	    "dmRecipient, dmRecipientAddress, dmDeliveryTime, dmAnnotation, "
	    "dmLegalTitleLaw, dmLegalTitleYear, dmLegalTitleSect, "
	    "dmLegalTitlePar, dmLegalTitlePoint, "
	    "dmRecipientRefNumber, dmRecipientIdent, "
	    "dmSenderRefNumber, dmSenderIdent, "
	    "dmToHands, dmPersonalDelivery, dmAllowSubstDelivery, "
	    "dmAcceptanceTime, dbIDSender, dbIDRecipient "
	    "FROM messages WHERE "
	    "dmID = :dmId";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {

		/* Sender info */
		html += messageTableSectionPdf(QObject::tr("Sender"));
		html += messageTableInfoStartPdf();
		tmp = query.value(0).toString() + QString(" (") +
		    query.value(21).toString() + QString("), ") +
		    query.value(1).toString();
		html += messageTableInfoPdf(QObject::tr("Name"), tmp);
		html += messageTableInfoEndPdf();

		/* Recipient info */
		html += messageTableSectionPdf(QObject::tr("Recipient"));
		html += messageTableInfoStartPdf();
		tmp = query.value(4).toString() + QString(" (") +
		    query.value(22).toString() + QString("), ") +
		    query.value(5).toString();
		html += messageTableInfoPdf(QObject::tr("Name"), tmp);
		html += messageTableInfoEndPdf();

		/* General info */
		html += messageTableSectionPdf(
		    QObject::tr("General Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(QObject::tr("Subject"),
		    query.value(7).toString());

		if (query.value(8).toString().isEmpty()) {
			tmp = "0";
		} else {
			tmp = query.value(8).toString();
		}
		tmp += QString(" / ");
		if (query.value(9).toString().isEmpty()) {
			tmp += "0";
		} else {
			tmp += query.value(9).toString();
		}
		tmp += QString(" § ") + " " +
		    query.value(10).toString() + " " +
		    QString(QObject::tr("paragraph")) + " " +
		    query.value(11).toString() + " " +
		    QString(QObject::tr("letter")) + " " +
		    query.value(12).toString();

		html += messageTableInfoPdf(QObject::tr("Delegation"), tmp);

		if (query.value(13).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(13).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Our ref.number"),
		    tmp);
		if (query.value(14).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(14).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Our doc.id"), tmp);
		if (query.value(15).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(15).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Your ref.number"),
		   tmp);
		if (query.value(16).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(16).toString();
		}
		html += messageTableInfoPdf(QObject::tr("Your doc.id"), tmp);
		if (query.value(17).toString().isEmpty()) {
			tmp = QObject::tr("Not specified");
		} else {
			tmp = query.value(17).toString();
		}
		html += messageTableInfoPdf(QObject::tr("To hands"), tmp);

		if ((query.value(18)).toInt()) {
			tmp = QObject::tr("yes");
		} else {
			tmp = QObject::tr("no");
		}
		html += messageTableInfoPdf(QObject::tr("Personal Delivery"),
		    tmp);

		if (query.value(19).toInt()) {
			tmp = QObject::tr("no");
		} else {
			tmp = QObject::tr("yes");
		}
		html += messageTableInfoPdf(
		    QObject::tr("Prohibit Delivery by Fiction"), tmp);

		html += messageTableInfoEndPdf();

		/* Delivery info */
		html += messageTableSectionPdf(
		    QObject::tr("Delivery/Acceptance Information"));
		html += messageTableInfoStartPdf();
		html += messageTableInfoPdf(QObject::tr("Delivery"),
		        dateTimeStrFromDbFormat(query.value(6).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoPdf(QObject::tr("Acceptance"),
		        dateTimeStrFromDbFormat(query.value(20).toString(),
		        dateTimeDisplayFormat));
		html += messageTableInfoEndPdf();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "SELECT dmEventTime, dmEventDescr "
	    "FROM events WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		/* Attachments info */
		html += messageTableSectionPdf(QObject::tr("Events"));
		html += messageTableInfoStartPdf();
		if (!query.isValid()) {
			goto fail;
		}
		while (query.isValid()) {
			tmp = dateTimeStrFromDbFormat(
			    query.value(0).toString(),
			    dateTimeDisplayFormat) + " - " +
			    query.value(1).toString();
			html += messageTableInfoPdf(QObject::tr("Time"), tmp);
			query.next();
		}
		html += messageTableInfoEndPdf();
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	html += divEnd;

	return html;

fail:
	return QString();
}


/* ========================================================================= */
/*
 * Return fileList related to given message.
 */
QList<QStringList> MessageDb::getFilesFromMessage(qint64 msgId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QList<QStringList> retList;

	QString queryStr = "SELECT _dmFileDescr, "
	    "dmEncodedContent FROM files WHERE message_id = :msgId";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":msgId", msgId);

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			QStringList fileItem;
			fileItem.append(query.value(0).toString());
			fileItem.append(query.value(1).toString());
			retList.append(fileItem);
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return retList;
fail:
	return QList<QStringList>();
}


/* ========================================================================= */
/*
 * Return files related to given message.
 */
QAbstractTableModel * MessageDb::flsModel(qint64 msgId)
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
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":msgId", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

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
	m_sqlFilesModel.setHeaderData(i - 1, Qt::Horizontal,
	    QObject::tr("File Size"));

	return &m_sqlFilesModel;

fail:
	return 0;
}


/* ========================================================================= */
/*
 * Check if any message (dmID) exists in the table
 */
int MessageDb::msgsStatusIfExists(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	int ret = -1;
	bool inSupplementary = false;

	queryStr = "SELECT dmMessageStatus FROM messages WHERE dmID = :dmId";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			ret = query.value(0).toInt();
		} else {
			/* No error message, just return. */
			goto fail;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Also check whether supplementary message data have been set. */
	queryStr = "SELECT count(*) FROM supplementary_message_data WHERE "
	    "message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		Q_ASSERT(query.value(0).toInt() < 2);
		inSupplementary = (1 == query.value(0).toInt());
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (inSupplementary) {
		return ret;
	} else {
		/*
		 * This should actually not happen, as the message was found
		 * in messages table but not in supplementary_message_data
		 * table.
		 */
		Q_ASSERT(0);
	}

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Check if delivery info exists in the table.
 */
bool MessageDb::isDeliveryInfoRawDb(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT count(*) FROM raw_delivery_info_data "
	    "WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return 0 < query.value(0).toInt();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}



/* ========================================================================= */
/*
 * Insert newly sent message into messages table.
 */
bool MessageDb::msgsInsertNewlySentMessageEnvelope(qint64 dmId,
    const QString &dbIDSender, const QString &dmSender,
    const QString &dbIDRecipient, const QString &dmRecipient,
    const QString &dmRecipientAddress, const QString &dmAnnotation)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "INSERT INTO messages ("
	    "dmID, dbIDSender, dmSender, "
	    "dmRecipient, dbIDRecipient, dmRecipientAddress, dmAnnotation, "
	    "dmPersonalDelivery, dmMessageStatus "
	    ") VALUES (:dmId, :dbIDSender, :dmSender, "
	    ":dmRecipient, :dbIDRecipient, :dmRecipientAddress, :dmAnnotation, "
	    ":dmPersonalDelivery, :dmMessageStatus)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":dmId", dmId);
	query.bindValue(":dbIDSender", dbIDSender);
	query.bindValue(":dmSender", dmSender);
	query.bindValue(":dmRecipient", dmRecipient);
	query.bindValue(":dmRecipientAddress", dmRecipientAddress);
	query.bindValue(":dbIDRecipient", dbIDRecipient);
	query.bindValue(":dmAnnotation", dmAnnotation);
	query.bindValue(":dmPersonalDelivery", 0);
	query.bindValue(":dmMessageStatus", 1);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	queryStr = "INSERT INTO supplementary_message_data ("
	    "message_id, message_type, read_locally, download_date, "
	    "custom_data) VALUES (:dmId, :message_type, :read_locally, "
	    ":download_date, :custom_data)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":message_type", TYPE_SENT);
	query.bindValue(":read_locally", true);
	query.bindValue(":download_date",
	    qDateTimeToDbFormat(QDateTime::currentDateTime()));
	query.bindValue(":custom_data", "null");

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return msgSetProcessState(dmId, UNSETTLED, true);
}


/* ========================================================================= */
/*
 * Insert message envelope into messages table.
 */
bool MessageDb::msgsInsertMessageEnvelope(qint64 dmId,
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
    const QByteArray &dmQTimestampBase64, const QString &dmDeliveryTime,
    const QString &dmAcceptanceTime, int dmMessageStatus,
    int dmAttachmentSize, const QString &_dmType,
    enum MessageDirection msgDirect)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "INSERT INTO messages ("
	    "dmID, _origin, dbIDSender, dmSender, "
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
	    ":dmId, :_origin, :dbIDSender, :dmSender, "
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
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
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
	query.bindValue(":dmQTimestamp", dmQTimestampBase64);
	query.bindValue(":dmDeliveryTime", dmDeliveryTime);
	query.bindValue(":dmAcceptanceTime", dmAcceptanceTime);
	query.bindValue(":dmMessageStatus", dmMessageStatus);
	query.bindValue(":dmAttachmentSize", dmAttachmentSize);
	query.bindValue(":_dmType", _dmType);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO supplementary_message_data ("
	    "message_id, message_type, read_locally, download_date, "
	    "custom_data) VALUES (:dmId, :message_type, :read_locally, "
	    ":download_date, :custom_data)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (MSG_RECEIVED == msgDirect) {
		query.bindValue(":message_type", TYPE_RECEIVED);
	} else {
		query.bindValue(":message_type", TYPE_SENT);
	}
	query.bindValue(":read_locally", false);
	query.bindValue(":download_date",
	    qDateTimeToDbFormat(QDateTime::currentDateTime()));
	query.bindValue(":custom_data", "null");

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return msgSetProcessState(dmId, UNSETTLED, true);

fail:
	return false;
}


/* ========================================================================= */
/*
 * Update exist message envelope/supplementary data in db
 */
bool MessageDb::msgsUpdateMessageEnvelope(qint64 dmId,
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
    const QByteArray &dmQTimestampBase64, const QString &dmDeliveryTime,
    const QString &dmAcceptanceTime, int dmMessageStatus,
    int dmAttachmentSize, const QString &_dmType,
    enum MessageDirection msgDirect)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "UPDATE messages SET "
	    "_origin = :_origin, "
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
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
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
	query.bindValue(":dmQTimestamp", dmQTimestampBase64);
	query.bindValue(":dmDeliveryTime", dmDeliveryTime);
	query.bindValue(":dmAcceptanceTime", dmAcceptanceTime);
	query.bindValue(":dmMessageStatus", dmMessageStatus);
	query.bindValue(":dmAttachmentSize", dmAttachmentSize);
	query.bindValue(":_dmType", _dmType);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "UPDATE supplementary_message_data SET "
	    "message_type = :message_type "
	    "WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (MSG_RECEIVED == msgDirect) {
		query.bindValue(":message_type", TYPE_RECEIVED);
	} else {
		query.bindValue(":message_type", TYPE_SENT);
	}

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Get message state.
 */
int MessageDb::messageState(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT dmMessageStatus "
	    "FROM messages WHERE dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toInt();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Advance message envelope search.
 */
QList<MessageDb::SoughtMsg> MessageDb::msgsAdvancedSearchMessageEnvelope(
    qint64 dmId,
    const QString &dmAnnotation,
    const QString &dbIDSender, const QString &dmSender,
    const QString &dmAddress,
    const QString &dbIDRecipient, const QString &dmRecipient,
    const QString &dmSenderRefNumber,
    const QString &dmSenderIdent,
    const QString &dmRecipientRefNumber,
    const QString &dmRecipientIdent,
    const QString &dmToHands,
    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
    enum MessageDirection msgDirect) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	(void) dmDeliveryTime; /* TODO - not used */
	(void) dmAcceptanceTime; /* TODO - not used */

	int i = 0;
	bool isMultiSelect = false;
	QString queryStr = "";
	QString andToken = " AND ";
	QString separ = " ";

	QStringList dmAnnotationList =
	    dmAnnotation.split(separ, QString::SkipEmptyParts);
	QStringList dmSenderList =
	    dmSender.split(separ, QString::SkipEmptyParts);
	QStringList dmAddressList =
	    dmAddress.split(separ, QString::SkipEmptyParts);
	QStringList dmRecipientList =
	    dmRecipient.split(separ, QString::SkipEmptyParts);
	QStringList dmToHandsList =
	    dmToHands.split(separ, QString::SkipEmptyParts);
	QList<SoughtMsg> msgList;

	/* Always ask for message type. */
	queryStr = "SELECT "
	    "m.dmID, m.dmDeliveryTime, "
	    "m.dmAnnotation, m.dmSender, m.dmRecipient, "
	    "s.message_type "
	    "FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "WHERE ";

	if (MSG_ALL == msgDirect) {
		/* select from all messages */
	} else if ((MSG_RECEIVED == msgDirect) || (MSG_SENT == msgDirect)) {
		/* means select only received (1) or sent (2) messages */
		isMultiSelect = true;
	} else {
		/* wrong input vaules from search dialog */
		return msgList;
	}

	if (dmId < 0) {

		bool isNotFirst = false;

		if (isMultiSelect) {
			queryStr += "s.message_type = :message_type";
			isNotFirst = true;
		}

		if (!dbIDSender.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dbIDSender = :dbIDSender";
		} else if (!dmSenderList.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmSender LIKE "
			    "'%'||:dmSender0||'%'";
			for (i = 1; i < dmSenderList.count(); i++) {
				queryStr += " AND m.dmSender LIKE "
				    "'%'||:dmSender" + QString::number(i) +
				    "||'%'";
			}
		}

		if (!dbIDRecipient.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dbIDRecipient = :dbIDRecipient";
		} else if (!dmRecipientList.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmRecipient LIKE "
			    "'%'||:dmRecipient0||'%'";
			for (i = 1; i < dmRecipientList.count(); i++) {
				queryStr += " AND m.dmRecipient LIKE "
				    "'%'||:dmRecipient" + QString::number(i) +
				    "||'%'";
			}
		}

		if (!dmSenderRefNumber.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmSenderRefNumber LIKE "
			    "'%'||:dmSenderRefNumber||'%'";
		}

		if (!dmRecipientRefNumber.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmRecipientRefNumber LIKE "
			    "'%'||:dmRecipientRefNumber||'%'";
		}

		if (!dmSenderIdent.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmSenderIdent LIKE "
			    "'%'||:dmSenderIdent||'%'";
		}

		if (!dmRecipientIdent.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmRecipientIdent LIKE "
			    "'%'||:dmRecipientIdent||'%'";
		}

		if (!dmAddressList.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "(m.dmSenderAddress LIKE "
			    "'%'||:dmSenderAddress0||'%' OR "
			    "m.dmRecipientAddress LIKE "
			    "'%'||:dmRecipientAddress0||'%')";
			for (i = 1; i < dmAddressList.count(); i++) {
				queryStr += " AND (m.dmSenderAddress LIKE "
				    "'%'||:dmSenderAddress" +
				    QString::number(i) +
				    "||'%' OR m.dmRecipientAddress LIKE "
				    "'%'||:dmRecipientAddress" +
				    QString::number(i) + "||'%')";
			}
		}

		if (!dmAnnotationList.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmAnnotation LIKE "
			    "'%'||:dmAnnotation0||'%'";
			for (i = 1; i < dmAnnotationList.count(); i++) {
				queryStr += " AND m.dmAnnotation LIKE "
				    "'%'||:dmAnnotation" + QString::number(i) +
				    "||'%'";
			}
		}

		if (!dmToHandsList.isEmpty()) {
			if (isNotFirst) {
				queryStr += andToken;
			}
			isNotFirst = true;
			queryStr += "m.dmToHands LIKE "
			    "'%'||:dmToHands0||'%'";
			for (i = 1; i < dmToHandsList.count(); i++) {
				queryStr += " AND m.dmToHands LIKE "
				    "'%'||:dmToHands" + QString::number(i) +
				    "||'%'";
			}
		}

		//qDebug() << queryStr;
		/* prepare query string */
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			return msgList;
		}

		/* query string binding */
		query.bindValue(":dbIDSender", dbIDSender);
		query.bindValue(":dbIDRecipient", dbIDRecipient);
		query.bindValue(":dmSenderRefNumber", dmSenderRefNumber);
		query.bindValue(":dmSenderIdent", dmSenderIdent);
		query.bindValue(":dmRecipientRefNumber", dmRecipientRefNumber);
		query.bindValue(":dmRecipientIdent", dmRecipientIdent);

		if (!dmAddressList.isEmpty()) {
			for (i = 0; i < dmAddressList.count(); i++) {
				query.bindValue(":dmSenderAddress" +
				    QString::number(i),
				    dmAddressList[i]);
			}
			for (i = 0; i < dmAddressList.count(); i++) {
				query.bindValue(":dmRecipientAddress" +
				    QString::number(i),
				    dmAddressList[i]);
			}
		}

		if (!dmSenderList.isEmpty()) {
			for (i = 0; i < dmSenderList.count(); i++) {
				query.bindValue(":dmSender" +
				    QString::number(i),
				    dmSenderList[i]);
			}
		}

		if (!dmRecipientList.isEmpty()) {
			for (i = 0; i < dmRecipientList.count(); i++) {
				query.bindValue(":dmRecipient" +
				    QString::number(i),
				    dmRecipientList[i]);
			}
		}

		if (!dmToHandsList.isEmpty()) {
			for (i = 0; i < dmToHandsList.count(); i++) {
				query.bindValue(":dmToHands" +
				    QString::number(i),
				    dmToHandsList[i]);
			}
		}

		if (!dmAnnotationList.isEmpty()) {
			for (i = 0; i < dmAnnotationList.count(); i++) {
				query.bindValue(":dmAnnotation" +
				    QString::number(i),
				    dmAnnotationList[i]);
			}
		}

		if (isMultiSelect) {
			if (MSG_RECEIVED == msgDirect) {
				query.bindValue(":message_type", TYPE_RECEIVED);
			} else {
				query.bindValue(":message_type", TYPE_SENT);
			}
		}
	} else {
		if (isMultiSelect) {
			queryStr += "s.message_type = :message_type";
			queryStr += andToken;
		}

		queryStr += "m.dmID LIKE '%'||:dmId||'%'";

		//qDebug() << queryStr;
		/* prepare query string */
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			query.lastError().text().toUtf8().constData());
			return msgList;
		}

		query.bindValue(":dmId", dmId);

		if (isMultiSelect) {
			if (MSG_RECEIVED == msgDirect) {
				query.bindValue(":message_type", TYPE_RECEIVED);
			} else {
				query.bindValue(":message_type", TYPE_SENT);
			}
		}
	}

	//qDebug() << queryStr;

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		while (query.isValid()) {
			Q_ASSERT(6 == query.record().count());

			SoughtMsg foundMsgData(
			    query.value(0).toLongLong(),
			    dateTimeFromDbFormat(query.value(1).toString()),
			    query.value(5).toInt(),
			    query.value(2).toString(),
			    query.value(3).toString(),
			    query.value(4).toString());

			/*
			 * Cannot use isValid() because the dates may be
			 * missing.
			 */
			Q_ASSERT(foundMsgData.mId.dmId >= 0);

			msgList.append(foundMsgData);
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return msgList;
	}

	return msgList;
}


/* ========================================================================= */
/*
 * Update message envelope delivery information.
 */
bool MessageDb::msgsUpdateMessageState(qint64 dmId,
    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
    int dmMessageStatus)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QString queryStr = "UPDATE messages SET "
	    "dmDeliveryTime = :dmDeliveryTime, "
	    "dmAcceptanceTime = :dmAcceptanceTime, "
	    "dmMessageStatus = :dmMessageStatus "
	    "WHERE dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":dmDeliveryTime", dmDeliveryTime);
	query.bindValue(":dmAcceptanceTime", dmAcceptanceTime);
	query.bindValue(":dmMessageStatus", dmMessageStatus);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Insert/update message file into file table
 */
bool MessageDb::msgsInsertUpdateMessageFile(qint64 dmId,
    const QString &dmFileDescr, const QString &dmUpFileGuid,
    const QString &dmFileGuid, const QString &dmMimeType,
    const QString &dmFormat, const QString &dmFileMetaType,
    const QByteArray &dmEncodedContentBase64)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int dbId = -1;

	QString queryStr = "SELECT id FROM files WHERE "
	    "message_id = :message_id AND _dmFileDescr = :dmFileDescr "
	    "AND _dmMimeType = :dmMimeType";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	query.bindValue(":dmFileDescr", dmFileDescr);
	query.bindValue(":dmMimeType", dmMimeType);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = -1;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (-1 != dbId) {
		queryStr = "UPDATE files SET "
		    " _dmFileDescr = :_dmFileDescr, "
		    "_dmUpFileGuid = :_dmUpFileGuid,"
		    " _dmFileGuid = :_dmFileGuid, _dmMimeType = :_dmMimeType, "
		    "_dmFormat = :_dmFormat, "
		    "_dmFileMetaType = :_dmFileMetaType, "
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
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	query.bindValue(":_dmFileDescr", dmFileDescr);
	query.bindValue(":_dmUpFileGuid", dmUpFileGuid);
	query.bindValue(":_dmFileGuid", dmFileGuid);
	query.bindValue(":_dmMimeType", dmMimeType);
	query.bindValue(":_dmFormat", dmFormat);
	query.bindValue(":_dmFileMetaType", dmFileMetaType);
	query.bindValue(":dmEncodedContent", dmEncodedContentBase64);
	if (-1 != dbId) {
		query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Insert/update message hash into hashes table.
 */
bool MessageDb::msgsInsertUpdateMessageHash(qint64 dmId,
    const QByteArray &valueBase64, const QString &algorithm)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int dbId = -1;

	QString queryStr = "SELECT id FROM hashes WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = -1;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (-1 != dbId) {
		queryStr = "UPDATE hashes SET "
		    "value = :value, _algorithm = :algorithm "
		    "WHERE id = :dbId";
	} else {
		queryStr = "INSERT INTO hashes (message_id, value, _algorithm)"
		    " VALUES (:dmId, :value, :algorithm)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":value", valueBase64);
	query.bindValue(":algorithm", algorithm);
	if (-1 != dbId) {
		query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Insert/update message events into events table.
 */
bool MessageDb::msgsInsertUpdateMessageEvent(qint64 dmId,
    const QString &dmEventTime, const QString &dmEventType,
    const QString &dmEventDescr)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int dbId = -1;

	QString queryStr = "SELECT id FROM events WHERE "
	    "message_id = :message_id AND dmEventTime = :dmEventTime";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	query.bindValue(":dmEventTime", dmEventTime);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toInt();
		} else {
			dbId = -1;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (-1 != dbId) {
		queryStr = "UPDATE events SET "
		"dmEventTime = :dmEventTime, dmEventDescr = :dmEventDescr "
		"WHERE id = :dbId";
	} else {
		queryStr = "INSERT INTO events (message_id, dmEventTime, "
		    "dmEventDescr) VALUES (:dmId, :dmEventTime, "
		    ":dmEventDescr)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":dmEventTime", dmEventTime);
	query.bindValue(":dmEventDescr", dmEventType + dmEventDescr);
	if (-1 != dbId) {
		query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Insert/update raw (DER) message data into raw_message_data
 *     table.
 */
bool MessageDb::msgsInsertUpdateMessageRaw(qint64 dmId, const QByteArray &raw,
    int messageType)
/* ========================================================================= */
{
	/* TODO -- The whole operation must fail or succeed. */

	QSqlQuery query(m_db);
	qint64 dbId = -1;
	struct x509_crt *crt = NULL;

	QString queryStr = "SELECT message_id FROM raw_message_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toLongLong();
		} else {
			dbId = -1;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (-1 != dbId) {
		queryStr = "UPDATE raw_message_data SET "
		"data = :data, message_type = :message_type "
		"WHERE message_id = :dbId";
	} else {
		queryStr = "INSERT INTO raw_message_data "
		"(message_id, message_type, data) "
		"VALUES (:dmId, :message_type, :data)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":data", raw.toBase64());
	/*
	 * The 'message_type' entry in 'raw_message_data' seems not to be used
	 * anywhere.
	 */
	query.bindValue(":message_type", messageType);
	if (-1 != dbId) {
		query.bindValue(":dbId", dbId);
	}
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Get certificate data. */
	crt = raw_cms_signing_cert(raw.data(), raw.size());
	if (NULL != crt) {
		QByteArray crtDer;

		void *der = NULL;
		size_t derSize = 0;
		if (0 == x509_crt_to_der(crt, &der, &derSize)) {
			/* Method setRawData() does not copy the data! */
			crtDer.setRawData((char *) der, derSize);

			msgsInsertUpdateMessageCertBase64(dmId,
			    crtDer.toBase64());

			free(der); der = NULL; derSize = 0;
		}

		x509_crt_destroy(crt); crt = NULL;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Check whether whole message is stored in database.
 */
bool MessageDb::msgsStoredWhole(qint64 dmId) const
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr =
	    "SELECT count(*) FROM raw_message_data WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toInt() != 0;
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Get raw message data from raw_message_data table.
 */
QByteArray MessageDb::msgsMessageBase64(qint64 dmId) const
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr =
	    "SELECT data FROM raw_message_data WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toByteArray();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QByteArray();
}


/* ========================================================================= */
/*
 * Get message data in DER (raw) format.
 */
QByteArray MessageDb::msgsMessageRaw(qint64 dmId) const
/* ========================================================================= */
{
	debugFuncCall();

	return QByteArray::fromBase64(msgsMessageBase64(dmId));
}


/* ========================================================================= */
/*
 * Get base64-encoded delivery info from
 *     raw_delivery_info_data table.
 */
QByteArray MessageDb::msgsGetDeliveryInfoBase64(qint64 dmId) const
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr =
	    "SELECT data FROM raw_delivery_info_data WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toByteArray();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QByteArray();
}


/* ========================================================================= */
/*
 * Return all message ID from database without attachment.
 */
QStringList MessageDb::getAllMessageIDsWithoutAttach(void) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT dmID FROM messages AS m "
	    "LEFT JOIN raw_message_data AS r ON (m.dmID = r.message_id) "
	    "WHERE r.message_id IS null";

	QStringList msgIsList;

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			msgIsList.append(query.value(0).toString());
			query.next();
		}
	}
	return msgIsList;
fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Return all message ID from database.
 */
QList<MessageDb::MsgId> MessageDb::getAllMessageIDsFromDB(void) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT dmID, dmDeliveryTime FROM messages";
	QList<MessageDb::MsgId> msgIdList;

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			msgIdList.append(MessageDb::MsgId(
			    query.value(0).toLongLong(),
			    dateTimeFromDbFormat(
			        query.value(1).toString())));
			query.next();
		}
	}
	return msgIdList;
fail:
	return QList<MessageDb::MsgId>();
}


/* ========================================================================= */
/*
 * Get all unique years from messages db.
 */
QStringList MessageDb::getAllUniqueYearsFormMsgs(void) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT strftime('%Y', dmDeliveryTime) "
	    "FROM messages";
	QStringList yearIsList;

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			if (!query.value(0).toString().isEmpty()) {
				yearIsList.append(query.value(0).toString());
			}
			query.next();
		}
	}
	return yearIsList;
fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Get list of all messages ID correspond with year.
 */
QStringList MessageDb::getAllMsgsIDEqualWithYear(const QString &year) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QStringList msgList;
	QString	queryStr;

	if (year == "inv") {
		queryStr = "SELECT dmID FROM messages WHERE "
		    "ifnull(dmDeliveryTime, '') = ''";
	} else {
		queryStr = "SELECT dmID FROM messages WHERE "
		    "strftime('%Y', dmDeliveryTime) = '"+ year + "'";
	}

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			if (!query.value(0).toString().isEmpty()) {
				msgList.append(query.value(0).toString());
			}
			query.next();
		}
	}
	return msgList;
fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Copy all messages correspond with year and their records
 * from tables into new db.
 */
bool MessageDb::copyRelevantMsgsToNewDb(const QString &newDbFileName,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	bool attached = false;
	bool transaction = false;
	QString queryStr;

	QStringList idList = getAllMsgsIDEqualWithYear(year);

	attached = attachDb2(m_db, newDbFileName);
	if (!attached) {
		goto fail;
	}

	// copy message data from messages table into new db.
	if (year == "inv") {
		queryStr = "INSERT INTO " DB2 ".messages SELECT * FROM messages "
		    "WHERE ifnull(dmDeliveryTime, '') = ''";
	} else {
		queryStr = "INSERT INTO " DB2 ".messages SELECT * FROM messages "
		    "WHERE strftime('%Y', dmDeliveryTime) = '"+ year + "'";
	}

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (query.exec() && query.isActive()) {
		/* TODO */
	}

	transaction = beginTransaction();
	if (!transaction) {
		goto fail;
	}

	// copy other message data from other tables into new db.
	for (int i = 0; i < idList.count(); ++i) {

		queryStr = "INSERT INTO " DB2 ".files SELECT * FROM files WHERE "
		   "message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".hashes SELECT * FROM hashes WHERE "
		   "message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".events SELECT * FROM events WHERE "
		   "message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".raw_message_data SELECT * "
		    "FROM raw_message_data WHERE message_id = "
		    "'"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".raw_delivery_info_data SELECT * "
		    "FROM raw_delivery_info_data WHERE message_id = "
		    "'"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".supplementary_message_data "
		    "SELECT * FROM supplementary_message_data WHERE "
		    "message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".process_state SELECT * FROM "
		    "process_state WHERE message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".certificate_data SELECT * FROM "
		    "certificate_data";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}

		queryStr = "INSERT INTO " DB2 ".message_certificate_data SELECT * "
		    "FROM message_certificate_data WHERE "
		    "message_id = '"+ idList.at(i) + "'";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			/* TODO */
		}
	}

	commitTransaction();
	detachDb2(m_db);

	return true;

fail:
	if (transaction) {
		rollbackTransaction();
	}
	if (attached) {
		detachDb2(m_db);
	}
	return false;
}


/* ========================================================================= */
/*
 * Insert raw (DER) delivery info into raw_delivery_info_data table.
 */
bool MessageDb::msgsInsertUpdateDeliveryInfoRaw(qint64 dmId,
    const QByteArray &raw)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	qint64 dbId = -1;

	QString queryStr = "SELECT message_id FROM raw_delivery_info_data "
	    "WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			dbId = query.value(0).toLongLong();
		} else {
			dbId = -1;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (-1 != dbId) {
		queryStr = "UPDATE raw_delivery_info_data SET "
		    "data = :data WHERE message_id = :dbId";
	} else {
		queryStr = "INSERT INTO raw_delivery_info_data "
		    "(message_id, data) VALUES (:dmId, :data)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":data", raw.toBase64());
	if (-1 != dbId) {
		query.bindValue(":dbId", dbId);
	}

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Update information about author (sender).
 */
bool MessageDb::updateMessageAuthorInfo(qint64 dmId, const QString &senderType,
    const QString &senderName)
/* ========================================================================= */
{
	QSqlQuery query(m_db);

	QJsonObject authorObject;
	authorObject.insert("userType", senderType.isEmpty() ?
	    QJsonValue(QJsonValue::Null) : senderType);
	authorObject.insert("authorName", senderName.isEmpty() ?
	    QJsonValue(QJsonValue::Null) : senderName);
	QJsonObject object;
	object.insert("message_author", authorObject);

	QJsonDocument document;
	document.setObject(object);
	QString json = document.toJson(QJsonDocument::Compact);

	QString queryStr = "UPDATE supplementary_message_data SET "
	    "custom_data = :custom_data WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":custom_data", json);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Get message hash from db
 */
QStringList MessageDb::msgsGetHashFromDb(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QStringList retitem;
	QString queryStr;

	queryStr = "SELECT value, _algorithm FROM hashes WHERE "
	    "message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			retitem.append(query.value(0).toString());
			retitem.append(query.value(1).toString());
			return retitem;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Delete message records from db.
 */
bool MessageDb::msgsDeleteMessageData(qint64 dmId) const
/* ========================================================================= */
{
	/* TODO -- The whole operation must fail or succeed. */

	QSqlQuery query(m_db);
	QString queryStr;

	int certificateId = -1;
	bool deleteCert = false;

	/* Delete hash from hashes table. */
	queryStr = "DELETE FROM hashes WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete file(s) from files table. */
	queryStr = "DELETE FROM files WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete event(s) from events table. */
	queryStr = "DELETE FROM events WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete raw message data from raw_message_data table. */
	queryStr=
	    "DELETE FROM raw_message_data WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete raw info data from raw_delivery_info_data table. */
	queryStr = "DELETE FROM raw_delivery_info_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete supplementary from supplementary_message_data table. */
	queryStr = "DELETE FROM supplementary_message_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Select certificate_id from message_certificate_data table. */
	queryStr = "SELECT certificate_id FROM message_certificate_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			certificateId = query.value(0).toInt();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	//Q_ASSERT(-1 != certificateId);

	/* Delete certificate reference from message_certificate_data table. */
	queryStr = "DELETE FROM message_certificate_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Select certificate id from message_certificate_data table. */
	queryStr = "SELECT count(*) FROM message_certificate_data WHERE "
	    "certificate_id = :certificate_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":certificate_id", certificateId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() > 0) {
				deleteCert = false;
			} else {
				/* No message uses this certificate. */
				deleteCert = true;
			}
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/*
	 * Delete certificate data from certificate_data table if no messages.
	 */
	if (deleteCert) {
		queryStr = "DELETE FROM certificate_data WHERE "
		    "id = :certificate_id";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":certificate_id", certificateId);
		if (!query.exec()) {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}

	/* Delete process state information from process_state table. */
	queryStr = "DELETE FROM process_state WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete message from messages table. */
	queryStr = "DELETE FROM messages WHERE dmID = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Return list of message ids corresponding to given date
 *     interval.
 */
QList<MessageDb::MsgId> MessageDb::msgsDateInterval(const QDate &fromDate,
    const QDate &toDate, enum MessageDirection msgDirect) const
/* ========================================================================= */
{
	/* TODO -- Check whether time is interpreted in correct time zone! */

	QSqlQuery query(m_db);
	QString queryStr;
	QList<MessageDb::MsgId> dmIDs;

	queryStr = "SELECT dmID, dmDeliveryTime "
	    "FROM messages AS m LEFT JOIN supplementary_message_data "
	    "AS s ON (m.dmID = s.message_id) WHERE "
	    "message_type = :message_type AND "
	    "(strftime('%Y-%m-%d', m.dmDeliveryTime) >= :fromDate) AND "
	    "(strftime('%Y-%m-%d', m.dmDeliveryTime) <= :toDate)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (MSG_RECEIVED == msgDirect) {
		query.bindValue(":message_type", TYPE_RECEIVED);
	} else {
		query.bindValue(":message_type", TYPE_SENT);
	}
	query.bindValue(":fromDate", fromDate);
	query.bindValue(":toDate", toDate);
	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			dmIDs.append(MessageDb::MsgId(
			    query.value(0).toLongLong(),
			    dateTimeFromDbFormat(
			        query.value(1).toString())));
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return dmIDs;

fail:
	return QList<MessageDb::MsgId>();
}


/* ========================================================================= */
/*
 * Return some message items in order to export correspondence
 *     to HTML.
 */
QStringList MessageDb::getMsgForHtmlExport(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	QStringList messageItems;

	queryStr =
	    "SELECT dmSender, dmRecipient, dmAnnotation, dmDeliveryTime "
	    "FROM messages WHERE dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		messageItems.append(query.value(0).toString());
		messageItems.append(query.value(1).toString());
		messageItems.append(query.value(2).toString());
		QDateTime dateTime =
		    dateTimeFromDbFormat(query.value(3).toString());
		messageItems.append(dateTime.toString("dd.MM.yyyy"));
		messageItems.append(dateTime.toString("hh:mm:ss"));
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return messageItems;

fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Return some message items for export correspondence to csv.
 */
QStringList MessageDb::getMsgForCsvExport(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	QStringList messageItems;

	queryStr = "SELECT dmMessageStatus, _dmType, dmDeliveryTime, "
	    "dmAcceptanceTime, dmAnnotation, dmSender, dmSenderAddress, "
	    "dmRecipient, dmRecipientAddress, dmSenderOrgUnit, "
	    "dmSenderOrgUnitNum, dmRecipientOrgUnit, dmRecipientOrgUnitNum "
	    "FROM messages WHERE dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		int count = query.record().count();
		Q_ASSERT(13 == count);
		for (int i = 0; i < count; ++i) {
			QString element = query.value(i).toString();

			if (element.contains(',')) {
				element = '"' + element + '"';
			}

			messageItems.append(element);
		}
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return messageItems;

fail:
	return QStringList();
}


/* ========================================================================= */
/*
 * Set the verification result.
 */
bool MessageDb::msgsSetVerified(qint64 dmId, bool verified)
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "UPDATE messages "
	    "SET is_verified = :verified WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":verified", verified);
	query.bindValue(":dmId", dmId);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set process state of received message
 */
bool MessageDb::msgSetProcessState(qint64 dmId, enum MessageProcessState state,
    bool insert)
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	if (insert) {
		queryStr = "INSERT INTO process_state ("
		    "message_id, state) VALUES (:dmId, :state)";
	} else {
		/*
		queryStr = "UPDATE process_state SET state = :state WHERE "
		    "message_id = :dmId";
		*/
		queryStr = "INSERT OR REPLACE INTO process_state ("
		    "message_id, state) VALUES (:dmId, :state)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	query.bindValue(":state", (int) state);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Get process state of received message
 */
int MessageDb::msgGetProcessState(qint64 dmId) const
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT state FROM process_state WHERE message_id = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toInt();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return -1;
}


/* ========================================================================= */
/*
 * Set process state of received messages.
 */
bool MessageDb::msgSetAllReceivedProcessState(enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "INSERT OR REPLACE INTO process_state (message_id, state)"
	    " SELECT s.message_id, :state "
	    "FROM supplementary_message_data AS s "
	    "WHERE s.message_type = :message_type";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":state", (int) state);
	query.bindValue(":message_type", TYPE_RECEIVED);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set process state of received messages in given year.
 */
bool MessageDb::smsgdtSetReceivedYearProcessState(const QString &year,
    enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "INSERT OR REPLACE INTO process_state (message_id, state)"
	    " SELECT s.message_id, :state "
	    "FROM supplementary_message_data AS s "
	    "LEFT JOIN messages AS m ON (s.message_id = m.dmID) "
	    "WHERE (ifnull(strftime('%Y', m.dmDeliveryTime), '" INVALID_YEAR "') = :year) and "
	    "(s.message_type = :message_type)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":state", (int) state);
	query.bindValue(":year", year);
	query.bindValue(":message_type", TYPE_RECEIVED);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Set process state of recently received messages.
 */
bool MessageDb::smsgdtSetWithin90DaysReceivedProcessState(
    enum MessageProcessState state)
/* ========================================================================= */
{
	debugFuncCall();

	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "INSERT OR REPLACE INTO process_state (message_id, state)"
	    " SELECT s.message_id, :state "
	    "FROM supplementary_message_data AS s "
	    "LEFT JOIN messages AS m ON (s.message_id = m.dmID) "
	    "WHERE (m.dmDeliveryTime >= date('now','-90 day')) and "
	    "(s.message_type = :message_type)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":state", (int) state);
	query.bindValue(":message_type", TYPE_RECEIVED);

	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}


/* ========================================================================= */
/*
 * Returns time stamp in raw (DER) format.
 */
QByteArray MessageDb::msgsTimestampRaw(qint64 dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "dmQTimestamp"
	    " FROM messages WHERE "
	    "dmID = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);

	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		QByteArray byteArray = query.value(0).toByteArray();
		if (byteArray.isEmpty()) {
			return QByteArray();
		}

		return QByteArray::fromBase64(byteArray);
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QByteArray();
}


/* ========================================================================= */
/*
 * Adds _dmType column.
 */
bool MessageDb::addDmtypeColumn(void)
/* ========================================================================= */
{
	if (false == m_db.isOpen()) {
		return false;
	}

	/*
	 * Create _dmType column if it does not exist.
	 */
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT _dmType FROM messages LIMIT 1";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (false == query.exec()) {
		query.clear();
		queryStr = "ALTER TABLE messages ADD COLUMN _dmType TEXT";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec()) {
			return true;
		} else {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Open database file.
 */
bool MessageDb::openDb(const QString &fileName, bool createMissing)
/* ========================================================================= */
{
	bool ret;

	if (m_db.isOpen()) {
		m_db.close();
	}

	if (globPref.store_messages_on_disk) {
		m_db.setDatabaseName(QDir::toNativeSeparators(fileName));
	} else {
		m_db.setDatabaseName(memoryLocation);
	}

	ret = m_db.open();

	if (createMissing && ret) {
		/* Ensure database contains all tables. */
		ret = createEmptyMissingTables();
	}

	if (ret) {
		ret = ensurePrimaryKeyInProcessStateTable();
	}

	if (!ret) {
		m_db.close();
	}

	return ret;
}


/* ========================================================================= */
/*
 * Close database file.
 */
void MessageDb::closeDb(void)
/* ========================================================================= */
{
	m_db.close();
}


/* ========================================================================= */
/*
 * Copy db.
 */
bool MessageDb::copyDb(const QString &newFileName)
/* ========================================================================= */
{
	bool copy_ret, open_ret;

	/* Close database. */
	m_db.close();

	/* Backup old file name. */
	QString oldFileName = fileName();
	logInfo("Copying database file '%s' to location '%s'.\n",
	    oldFileName.toUtf8().constData(),
	    newFileName.toUtf8().constData());

	/* Fail if target equals the source. */
	/* TODO -- Perform a more reliable check than string comparison. */
	if (oldFileName == newFileName) {
		logWarning("Copying of database file '%s' aborted. "
		    "Target and source are equal.\n",
		    oldFileName.toUtf8().constData());
		return false;
	}

	/* Erase target if exists. */
	QFile::remove(newFileName);

	/* Copy database file. */
	copy_ret = QFile::copy(oldFileName, newFileName);

	/* Open database. */
	open_ret = openDb(copy_ret ? newFileName : oldFileName);
	if (!open_ret) {
		Q_ASSERT(0);
		logErrorNL("File '%s' could not be opened.",
		    copy_ret ?
		        newFileName.toUtf8().constData() :
		        oldFileName.toUtf8().constData());
		/* TODO -- qFatal() ? */
		return false;
	}

	return copy_ret;
}


/* ========================================================================= */
/*
 * Move db.
 */
bool MessageDb::moveDb(const QString &newFileName)
/* ========================================================================= */
{
	bool move_ret, open_ret;

	/* Close database. */
	m_db.close();

	/* Backup old file name. */
	QString oldFileName = fileName();
	logInfo("Moving database file '%s' to location '%s'.\n",
	    oldFileName.toUtf8().constData(),
	    newFileName.toUtf8().constData());

	/* Fail if target equals the source. */
	/* TODO -- Perform a more reliable check than string comparison. */
	if (oldFileName == newFileName) {
		logWarning("Moving of database file '%s' aborted. "
		    "Target and source are equal.\n",
		    oldFileName.toUtf8().constData());
		return false;
	}

	/* Erase target if exists. */
	QFile::remove(newFileName);

	/* Move database file. */
	move_ret = QFile::rename(oldFileName, newFileName);

	/* Open database. */
	open_ret = openDb(move_ret ? newFileName : oldFileName);
	if (!open_ret) {
		Q_ASSERT(0);
		logErrorNL("File '%s' could not be opened.",
		    move_ret ?
		        newFileName.toUtf8().constData() :
		        oldFileName.toUtf8().constData());
		/* TODO -- qFatal() ? */
		return false;
	}

	return move_ret;
}


/* ========================================================================= */
/*
 * Open a new empty database file.
 */
bool MessageDb::reopenDb(const QString &newFileName)
/* ========================================================================= */
{
	bool reopen_ret, open_ret;

	/* Close database. */
	m_db.close();

	/* Backup old file name. */
	QString oldFileName = fileName();
	logInfo("Closing database file '%s' re-opening file '%s'.\n",
	    oldFileName.toUtf8().constData(),
	    newFileName.toUtf8().constData());

	/* Fail if target equals the source. */
	/* TODO -- Perform a more reliable check than string comparison. */
	if (oldFileName == newFileName) {
		logWarning("Re-opening of database file '%s' aborted. "
		    "Target and source are equal.\n",
		    oldFileName.toUtf8().constData());
		return false;
	}

	/* Erase target if exists. */
	QFile::remove(newFileName);

	/* Open new database file. */
	reopen_ret = openDb(newFileName);

	/* Open database. */
	if (!reopen_ret) {
		open_ret = openDb(oldFileName);
		if (!open_ret) {
			logErrorNL("File '%s' could not be opened.",
			    oldFileName.toUtf8().constData());
			/* TODO -- qFatal() ? */
			return false;
		}
	}

	return reopen_ret;
}


/* ========================================================================= */
/*
 * Perform a db integrity check.
 */
bool MessageDb::checkDb(bool quick)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	bool ret = false;

	QString queryStr;
	if (quick) {
		queryStr = "PRAGMA quick_check";
	} else {
		queryStr = "PRAGMAintegrity_check";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			ret = query.value(0).toBool();
		} else {
			ret = false;
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return ret;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Add/update message certificate in database.
 */
bool MessageDb::msgsInsertUpdateMessageCertBase64(qint64 dmId,
    const QByteArray &crtBase64)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	int certId = -1;
	bool certEntryFound = false;

	/* Search for certificate in 'certificate_data' table. */
	QString queryStr = "SELECT id FROM certificate_data WHERE "
	    "der_data = :der_data";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":der_data", crtBase64);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			certId = query.value(0).toInt(); /* Found. */
		} else {
			certId = -1; /* Not found. */
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* If certificate was not found. */
	if (-1 == certId) {
		/* Create unique certificate identifier. */
		queryStr = "SELECT max(id) FROM certificate_data";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		if (query.exec() && query.isActive()) {
			query.first();
			if (query.isValid()) {
				certId = query.value(0).toInt();
			} else {
				certId = -1;
			}
		} else {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}

		if (-1 == certId) {
			certId = 1; /* First certificate. */
		} else {
			++certId;
		}

		/* Insert missing certificate. */
		queryStr = "INSERT INTO certificate_data "
		    "(id, der_data) VALUES (:id, :der_data)";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":id", certId);
		query.bindValue(":der_data", crtBase64);
		if (!query.exec()) {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}

	/*
	 * Abort operation if there is still no matching certificate available.
	 */
	if (-1 == certId) {
		return false;
	}

	/*
	 * Tie certificate to message. Find whether there is already
	 * an entry for the message.
	 */
	queryStr = "SELECT * FROM message_certificate_data WHERE"
	    " message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		certEntryFound = query.isValid();
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/*
	 * Create or update message entry depending on whether a corresponding
	 * entry was found.
	 */
	if (certEntryFound) {
		queryStr = "UPDATE message_certificate_data SET "
		    "certificate_id = :certificate_id WHERE "
		    "message_id = :message_id";
	} else {
		queryStr = "INSERT INTO message_certificate_data "
		    "(message_id, certificate_id) VALUES "
		    "(:message_id, :certificate_id)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", dmId);
	query.bindValue(":certificate_id", certId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Create empty tables if tables do not already exist.
 */
bool MessageDb::createEmptyMissingTables(void)
/* ========================================================================= */
{
	bool ret;

	if (!msgsTbl.existsInDb(this->m_db)) {
		ret = msgsTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!flsTbl.existsInDb(this->m_db)) {
		ret = flsTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!hshsTbl.existsInDb(this->m_db)) {
		ret = hshsTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!evntsTbl.existsInDb(this->m_db)) {
		ret = evntsTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!rwmsgdtTbl.existsInDb(this->m_db)) {
		ret = rwmsgdtTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!rwdlvrinfdtTbl.existsInDb(this->m_db)) {
		ret = rwdlvrinfdtTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!smsgdtTbl.existsInDb(this->m_db)) {
		ret = smsgdtTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!crtdtTbl.existsInDb(this->m_db)) {
		ret = crtdtTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!msgcrtdtTbl.existsInDb(this->m_db)) {
		ret = msgcrtdtTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	if (!prcstTbl.existsInDb(this->m_db)) {
		ret = prcstTbl.createEmpty(this->m_db);
		if (!ret) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * This method ensures that the process_state table
 *     contains a PRIMARY KEY. This table might be created without any
 *     primary key reference due to a bug in a previous version.
 */
bool MessageDb::ensurePrimaryKeyInProcessStateTable(void)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	QString createTableSql;

	queryStr = "SELECT sql FROM sqlite_master "
	    "WHERE (type = :type) and (name = :name)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":type", QString("table"));
	query.bindValue(":name", QString("process_state"));
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		createTableSql = query.value(0).toString();
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: "
		    "%s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (createTableSql.isEmpty()) {
		goto fail;
	}


	if (createTableSql.contains("PRIMARY", Qt::CaseSensitive)) {
		return true;
	}

	/* Table does not contain primary key. */

	/* Rename existing table. */
	queryStr = "ALTER TABLE process_state RENAME TO _process_state";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (!query.exec()) {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: "
		    "%s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (!prcstTbl.createEmpty(m_db)) {
		goto fail;
	}

	/* Copy table content. */
	queryStr = "INSERT OR REPLACE INTO process_state (message_id, state) "
	    "SELECT message_id, state FROM _process_state";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (!query.exec()) {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: "
		    "%s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* Delete old table. */
	queryStr = "DROP TABLE _process_state";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (!query.exec()) {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: "
		    "%s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}


/* ========================================================================= */
/*
 * Returns verification date (in local time).
 */
QDateTime MessageDb::msgsVerificationDate(qint64 dmId) const
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
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":dmId", dmId);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid()) {
			QDateTime dateTime =
			    dateTimeFromDbFormat(query.value(0).toString());

			if (dateTime.isValid()) {
				return dateTime;
			}
		} else {
			logErrorNL(
			    "Cannot execute SQL query and/or read SQL data: "
			    "%s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
	}

	return QDateTime::currentDateTime();

fail:
	return QDateTime();
}


/* ========================================================================= */
/*
 * Return some additional filename entries as struct:
 * (dmDeliveryTime, dmAcceptanceTime, dmAnnotation, dmSender)
 */
MessageDb::FilenameEntry MessageDb::msgsGetAdditionalFilenameEntry(qint64 dmId)
    const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	MessageDb::FilenameEntry entry;

	queryStr = "SELECT dmDeliveryTime, dmAcceptanceTime, dmAnnotation, "
	    "dmSender FROM messages WHERE dmId = :dmId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmId", dmId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		entry.dmDeliveryTime =
		    dateTimeFromDbFormat(query.value(0).toString());
		entry.dmAcceptanceTime =
		    dateTimeFromDbFormat(query.value(1).toString());
		entry.dmAnnotation = query.value(2).toString();
		entry.dmSender = query.value(3).toString();
		return entry;
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: "
		    "%s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return entry;
}


/* ========================================================================= */
/*
 * Read data from supplementary message data table.
 */
QJsonDocument MessageDb::smsgdCustomData(qint64 msgId) const
/* ========================================================================= */
{
	QJsonDocument jsonDoc;
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "custom_data"
	    " FROM supplementary_message_data WHERE "
	    "message_id = :msgId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":msgId", msgId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			jsonDoc = QJsonDocument::fromJson(
			    query.value(0).toByteArray());
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return jsonDoc;

fail:
	return QJsonDocument();
}


/* ========================================================================= */
/*
 * Check whether message signature was valid at given date
 *     (local time).
 */
bool MessageDb::msgCertValidAtDate(qint64 dmId, const QDateTime &dateTime,
    bool ignoreMissingCrlCheck) const
/* ========================================================================= */
{
	debugFuncCall();

	QByteArray rawBytes = msgsMessageRaw(dmId);
	Q_ASSERT(rawBytes.size() > 0);

	if (ignoreMissingCrlCheck) {
		logWarning("CRL check is not performed for message %d.\n",
		    dmId);
	}
	time_t utcTime = dateTime.toTime_t();

	return 1 == raw_msg_verify_signature_date(
	    rawBytes.data(), rawBytes.size(), utcTime,
	    ignoreMissingCrlCheck ? 0 : 1);
}


/* ========================================================================= */
/*
 * Test if imported message is relevent to account db.
 */
bool MessageDb::isRelevantMsgForImport(qint64 msgId, const QString databoxId)
    const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT dmID FROM messages WHERE dmID = :dmID AND "
	    "(dbIDSender = :dbIDSender OR dbIDRecipient = :dbIDRecipient)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":dmID", msgId);
	query.bindValue(":dbIDSender", databoxId);
	query.bindValue(":dbIDRecipient", databoxId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			return !query.value(0).toString().isEmpty();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return false;
}


/* ========================================================================= */
/*
 * Copy all message data to account database from source database.
 */
bool MessageDb::copyCompleteMsgDataToAccountDb(const QString &sourceDbPath,
    qint64 msgId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QByteArray der_data;
	bool attached = false;
	bool transaction = false;
	QString queryStr;

	attached = attachDb2(m_db, sourceDbPath);
	if (!attached) {
		goto fail;
	}

	transaction = beginTransaction();
	if (!transaction) {
		goto fail;
	}

	// copy message data from messages table into db.
	queryStr = "INSERT INTO messages SELECT * FROM " DB2 ".messages WHERE "
	    "dmID = :dmID";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":dmID", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	// copy other message data from other tables into new db.
	queryStr = "INSERT INTO files SELECT * FROM " DB2 ".files WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO hashes SELECT * FROM " DB2 ".hashes WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO events SELECT * FROM " DB2 ".events WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO raw_message_data SELECT * "
	    "FROM " DB2 ".raw_message_data WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO raw_delivery_info_data SELECT * "
	    "FROM " DB2 ".raw_delivery_info_data WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO supplementary_message_data "
	    "SELECT * FROM " DB2 ".supplementary_message_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "INSERT INTO process_state SELECT * FROM " DB2
	    ".process_state WHERE message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* insert message_certificate_data */
	queryStr = "INSERT INTO message_certificate_data SELECT * "
	    "FROM " DB2 ".message_certificate_data WHERE "
	    "message_id = :message_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	/* get cert der_data based to message_id from source database */
	queryStr = "SELECT der_data FROM " DB2 ".certificate_data WHERE id IN "
	    "(SELECT certificate_id FROM " DB2 ".message_certificate_data "
	    "WHERE message_id = :message_id)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_id", msgId);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		der_data = query.value(0).toByteArray();
	} else {
		logInfoMl("Cannot exec SQL query - "
		    "message cert data missing: %s.",
		    query.lastError().text().toUtf8().constData());
	}

	if (!der_data.isEmpty()) {
		/* check if der_data exists in the target database and update
		* message certificate_id
		*/
		if (!msgsInsertUpdateMessageCertBase64(msgId, der_data)) {
			goto fail;
		}
	}

	commitTransaction();
	detachDb2(m_db);

	return true;

fail:
	if (transaction) {
		rollbackTransaction();
	}
	if (attached) {
		detachDb2(m_db);
	}
	return false;
}
