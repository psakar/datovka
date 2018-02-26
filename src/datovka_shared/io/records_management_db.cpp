/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "src/io/db_tables.h"
#include "src/datovka_shared/io/records_management_db.h"
#include "src/log/log.h"

/*!
 * @brief Delete all entries from table.
 *
 * @param[in,out] db SQL database.
 * @param[in]     tblName Name of table whose content should be erased.
 * @return True on success.
 */
static
bool deleteTableContent(QSqlDatabase &db, const QString &tblName)
{
	if (tblName.isEmpty()) {
		return false;
	}

	QSqlQuery query(db);

	QString queryStr = "DELETE FROM " + tblName;
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool RecordsManagementDb::deleteAllEntries(void)
{
	deleteTableContent(m_db, QStringLiteral("service_info"));
	deleteTableContent(m_db, QStringLiteral("stored_files_messages"));

	return true;
}

/*!
 * @brief Insert a new service info record into service info table.
 *
 * @param[in,out] db SQL database.
 * @param[in]     entry Service info entry.
 * @return True on success.
 */
static
bool insertServiceInfo(QSqlDatabase &db,
    const RecordsManagementDb::ServiceInfoEntry &entry)
{
	if (!entry.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	QSqlQuery query(db);

	QString queryStr = "INSERT INTO service_info "
	    "(url, name, token_name, logo_svg) VALUES "
	    "(:url, :name, :tokenName, :logoSvg)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":url", entry.url);
	query.bindValue(":name", entry.name);
	query.bindValue(":tokenName", entry.tokenName);
	query.bindValue(":logoSvg", entry.logoSvg.toBase64());

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool RecordsManagementDb::updateServiceInfo(const ServiceInfoEntry &entry)
{
	if (!entry.isValid()) {
		return false;
	}

	if (!beginTransaction()) {
		return false;
	}

	if (!deleteTableContent(m_db, QStringLiteral("service_info"))) {
		goto rollback;
	}

	if (!insertServiceInfo(m_db, entry)) {
		goto rollback;
	}

	return commitTransaction();

rollback:
	rollbackTransaction();
	return false;
}

RecordsManagementDb::ServiceInfoEntry RecordsManagementDb::serviceInfo(void) const
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT url, name, token_name, logo_svg "
	    "FROM service_info";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return ServiceInfoEntry();
	}

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			ServiceInfoEntry entry;

			entry.url = query.value(0).toString();
			entry.name = query.value(1).toString();
			entry.tokenName = query.value(2).toString();
			entry.logoSvg = QByteArray::fromBase64(
			    query.value(3).toByteArray());

			Q_ASSERT(entry.isValid());
			return entry;
		} else {
			return ServiceInfoEntry();
		}
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		return ServiceInfoEntry();
	}
}

bool RecordsManagementDb::deleteAllStoredMsg(void)
{
	return deleteTableContent(m_db,
	    QStringLiteral("stored_files_messages"));
}

bool RecordsManagementDb::deleteStoredMsg(qint64 dmId)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM stored_files_messages "
	    "WHERE dm_id = :dm_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":dm_id", dmId);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool RecordsManagementDb::updateStoredMsg(qint64 dmId,
    const QStringList &locations)
{
#define LIST_SEPARATOR QLatin1String("^")

	QSqlQuery query(m_db);

	QString queryStr = "INSERT OR REPLACE INTO stored_files_messages "
	    "(dm_id, separator, joined_locations) VALUES "
	    "(:dm_id, :separator, :joined_locations)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":dm_id", dmId);
	query.bindValue(":separator", LIST_SEPARATOR);
	query.bindValue(":joined_locations", locations.join(LIST_SEPARATOR));

	if (!query.exec()) {
		logErrorNL("Cannot exec SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;

#undef LIST_SEPARATOR
}

QList<qint64> RecordsManagementDb::getAllDmIds(void) const
{
	QSqlQuery query(m_db);

	QList<qint64> dmIdList;

	QString queryStr = "SELECT dm_id FROM stored_files_messages";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return dmIdList;
	}
	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			dmIdList.append(query.value(0).toLongLong());
			query.next();
		}
	}
	return dmIdList;
}

QStringList RecordsManagementDb::storedMsgLocations(qint64 dmId) const
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT separator, joined_locations "
	    "FROM stored_files_messages WHERE dm_id = :dm_id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return QStringList();
	}
	query.bindValue(":dm_id", dmId);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			QString separator(query.value(0).toString());
			return query.value(1).toString().split(separator);
		} else {
			return QStringList();
		}
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		return QStringList();
	}
}

QList<class SQLiteTbl *> RecordsManagementDb::listOfTables(void) const
{
	QList<class SQLiteTbl *> tables;
	tables.append(&srvcInfTbl);
	tables.append(&strdFlsMsgsTbl);
	return tables;
}
