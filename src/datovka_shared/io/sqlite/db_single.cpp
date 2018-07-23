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

#include "src/datovka_shared/io/sqlite/db_single.h"
#include "src/datovka_shared/log/log.h"

SQLiteDbSingle::SQLiteDbSingle(const QString &connectionName,
    bool allowRelocation)
    : SQLiteDb(connectionName),
    m_relocatable(allowRelocation)
{
}

bool SQLiteDbSingle::copyDb(const QString &newFileName,
    enum SQLiteDb::OpenFlag flag)
{
	if (!m_relocatable) {
		logErrorNL("%s", "Database is not relocatable. Cannot copy.");
		return false;
	}

	return SQLiteDb::copyDb(newFileName, flag);
}

bool SQLiteDbSingle::reopenDb(const QString &newFileName,
    enum SQLiteDb::OpenFlag flag)
{
	if (!m_relocatable) {
		logErrorNL("%s", "Database is not relocatable. Cannot reopen.");
		return false;
	}

	return SQLiteDb::reopenDb(newFileName, flag);
}

bool SQLiteDbSingle::moveDb(const QString &newFileName,
    enum SQLiteDb::OpenFlag flag)
{
	if (!m_relocatable) {
		logErrorNL("%s", "Database is not relocatable. Cannot move.");
		return false;
	}

	return SQLiteDb::moveDb(newFileName, flag);
}

bool SQLiteDbSingle::openDb(const QString &fileName, SQLiteDb::OpenFlags flags)
{
	if (m_db.isOpen() && !m_relocatable) {
		logErrorNL("%s",
		    "Database is not relocatable. Cannot open again.");
		return false;
	}

	return SQLiteDb::openDb(fileName, flags);
}
