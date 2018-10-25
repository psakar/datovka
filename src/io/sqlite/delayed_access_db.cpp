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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/datovka_shared/log/log.h"
#include "src/io/sqlite/delayed_access_db.h"

/* Null objects - for convenience. */
static const QSqlDatabase nullSqlDatabase;

DelayedAccessSQLiteDb::DelayedAccessSQLiteDb(const QString &connectionName)
    : SQLiteDb(connectionName),
    m_fileName(),
    m_flags(SQLiteDb::NO_OPTIONS)
{
}

bool DelayedAccessSQLiteDb::copyDb(const QString &newFileName,
    enum OpenFlag flag)
{
	if (Q_UNLIKELY(!_accessDb())) {
		return false;
	}

	if (SQLiteDb::copyDb(newFileName, flag)) {
		m_fileName = newFileName;
		m_flags = flag;
		return true;
	}

	return false;
}

bool DelayedAccessSQLiteDb::openDb(const QString &fileName,
     SQLiteDb::OpenFlags flags)
{
	if (Q_UNLIKELY((!(flags & SQLiteDb::FORCE_IN_MEMORY)) && fileName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY((!(flags & SQLiteDb::FORCE_IN_MEMORY)) && (fileName == memoryLocation))) {
		Q_ASSERT(0);
		return false;
	}

	/* A database in memory is opened immediately. */
	if (flags & SQLiteDb::FORCE_IN_MEMORY) {
		logInfoNL("Opening connection '%s' to database file '%s'.",
		    m_db.connectionName().toUtf8().constData(),
		    fileName.toUtf8().constData());
		const bool ret = SQLiteDb::openDb(fileName, flags);
		if (ret) {
			m_fileName = fileName;
			m_flags = flags;
		} else {
			m_fileName.clear();
			m_flags = SQLiteDb::NO_OPTIONS;
		}
		return ret;
	}

	/* Delay the opening of the database file. */
	logInfoNL(
	    "Delaying the opening of connection '%s' to database file '%s'.",
	    m_db.connectionName().toUtf8().constData(),
	    fileName.toUtf8().constData());
	m_fileName = fileName;
	m_flags = flags;
	return true;
}

bool DelayedAccessSQLiteDb::isOpen(void) const
{
	return m_db.isOpen();
}

const QSqlDatabase &DelayedAccessSQLiteDb::accessDb(void)
{
	if (Q_UNLIKELY(!_accessDb())) {
		return nullSqlDatabase;
	}

	return m_db;
}

bool DelayedAccessSQLiteDb::_accessDb(void)
{
	if (Q_UNLIKELY(!isOpen())) {
		const char *cCName = m_db.connectionName().toUtf8().constData();
		if (Q_UNLIKELY(m_fileName.isEmpty())) {
			logErrorNL(
			    "Missing file name to unopened database '%s'.",
			    cCName);
			Q_ASSERT(0);
			return false;
		}
		const char *cFileName = m_fileName.toUtf8().constData();
		logInfoNL("Opening connection '%s' to database file '%s'.",
		    cCName, cFileName);
		if (Q_UNLIKELY(!SQLiteDb::openDb(m_fileName, m_flags))) {
			logErrorNL("Cannot open database '%s'.", cFileName);
			return false;
		}
	}

	return true;
}
