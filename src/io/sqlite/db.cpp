/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "src/datovka_shared/io/sqlite/table.h"
#include "src/io/sqlite/db.h"
#include "src/log/log.h"

const QString SQLiteDb::memoryLocation(":memory:");
const QString SQLiteDb::dbDriverType("QSQLITE");

SQLiteDb::SQLiteDb(const QString &connectionName)
{
	m_db = QSqlDatabase::addDatabase(dbDriverType, connectionName);
}

SQLiteDb::~SQLiteDb(void)
{
	m_db.close();
}

QString SQLiteDb::fileName(void) const
{
	return m_db.databaseName();
}

bool SQLiteDb::beginTransaction(void)
{
	QSqlQuery query(m_db);
	QString queryStr("BEGIN DEFERRED TRANSACTION");
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

bool SQLiteDb::commitTransaction(void)
{
	QSqlQuery query(m_db);
	QString queryStr("COMMIT TRANSACTION");
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

bool SQLiteDb::savePoint(const QString &savePointName)
{
	if (savePointName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSqlQuery query(m_db);
	QString queryStr("SAVEPOINT :name");
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":name", savePointName);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool SQLiteDb::releaseSavePoint(const QString &savePointName)
{
	if (savePointName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSqlQuery query(m_db);
	QString queryStr("RELEASE SAVEPOINT :name");
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":name", savePointName);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool SQLiteDb::rollbackTransaction(const QString &savePointName)
{
	QSqlQuery query(m_db);

	if (savePointName.isEmpty()) {

		QString queryStr("ROLLBACK TRANSACTION");
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

	} else {

		QString queryStr("ROLLBACK TRANSACTION TO SAVEPOINT :name");
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			return false;
		}
		query.bindValue(":name", savePointName);
		if (!query.exec()) {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			return false;
		}
		return true;

	}
}

bool SQLiteDb::dbDriverSupport(void)
{
	QStringList driversList = QSqlDatabase::drivers();
	return driversList.contains(dbDriverType, Qt::CaseSensitive);
}

void SQLiteDb::closeDb(void)
{
	m_db.close();
}

bool SQLiteDb::checkDb(bool quick)
{
	QSqlQuery query(m_db);
	bool ret = false;

	QString queryStr;
	if (quick) {
		queryStr = "PRAGMA quick_check";
	} else {
		queryStr = "PRAGMA integrity_check";
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

bool SQLiteDb::vacuum(void)
{
	QSqlQuery query(m_db);

	QString queryStr("VACUUM");
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL command: %s.",
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

bool SQLiteDb::copyDb(const QString &newFileName,
    const QList<class SQLiteTbl *> &tables)
{
	if (fileName() == memoryLocation) {
		logErrorNL("%s",
		    "Trying to copy database that resides i memory.");
		Q_ASSERT(0);
		return false;
	}

	bool copy_ret, open_ret;

	/* Close database. */
	m_db.close();

	/* Backup old file name. */
	QString oldFileName = fileName();
	logInfoNL("Copying database file '%s' to location '%s'.",
	    oldFileName.toUtf8().constData(),
	    newFileName.toUtf8().constData());

	/* Fail if target equals the source. */
	/* TODO -- Perform a more reliable check than string comparison. */
	if (oldFileName == newFileName) {
		logWarningNL(
		    "Copying of database file '%s' aborted. Target and source are equal.",
		    oldFileName.toUtf8().constData());
		return false;
	}

	/* Erase target if exists. */
	QFile::remove(newFileName);

	/* Copy database file. */
	copy_ret = QFile::copy(oldFileName, newFileName);

	/* Open database. */
	open_ret = openDb(copy_ret ? newFileName : oldFileName, false, tables);
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

bool SQLiteDb::moveDb(const QString &newFileName,
    const QList<class SQLiteTbl *> &tables)
{
	if (fileName() == memoryLocation) {
		logErrorNL("%s",
		    "Trying to copy database that resides i memory.");
		Q_ASSERT(0);
		return false;
	}

	bool move_ret, open_ret;

	/* Close database. */
	m_db.close();

	/* Backup old file name. */
	QString oldFileName = fileName();
	logInfoNL("Moving database file '%s' to location '%s'.",
	    oldFileName.toUtf8().constData(),
	    newFileName.toUtf8().constData());

	/* Fail if target equals the source. */
	/* TODO -- Perform a more reliable check than string comparison. */
	if (oldFileName == newFileName) {
		logWarningNL(
		    "Moving of database file '%s' aborted. Target and source are equal.",
		    oldFileName.toUtf8().constData());
		return false;
	}

	/* Erase target if exists. */
	QFile::remove(newFileName);

	/* Move database file. */
	move_ret = QFile::rename(oldFileName, newFileName);

	/* Open database. */
	open_ret = openDb(move_ret ? newFileName : oldFileName, false, tables);
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

bool SQLiteDb::openDb(const QString &fileName, bool forceInMemory,
    const QList<class SQLiteTbl *> &tables)
{
	bool ret;

	if (m_db.isOpen()) {
		m_db.close();
	}

	if (!forceInMemory) {
		m_db.setDatabaseName(QDir::toNativeSeparators(fileName));
	} else {
		m_db.setDatabaseName(memoryLocation);
	}

	ret = m_db.open();

	if (ret) {
		/* Ensure database contains all tables. */
		ret = createEmptyMissingTables(tables);
	}

	if (!ret) {
		m_db.close();
	}

	return ret;
}

bool SQLiteDb::attachDb2(QSqlQuery &query, const QString &attachFileName)
{
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

bool SQLiteDb::detachDb2(QSqlQuery &query)
{
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

bool SQLiteDb::createEmptyMissingTables(const QList<class SQLiteTbl *> &tables)
{
	foreach (const SQLiteTbl *tblPtr, tables) {
		Q_ASSERT(0 != tblPtr);
		if (!tblPtr->createEmpty(m_db)) {
			goto fail; /* TODO -- Proper recovery? */
		}
	}
	return true;

fail:
	return false;
}
