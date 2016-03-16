/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include "src/io/tag_db.h"
#include "src/io/db_tables.h"
#include "src/log/log.h"

TagDb::TagDb(const QString &connectionName)
    : SQLiteDb(connectionName)
{
}

bool TagDb::openDb(const QString &fileName)
{
	return SQLiteDb::openDb(fileName, false, listOfTables());
}


QList<class SQLiteTbl *> TagDb::listOfTables(void)
{
	static QList<class SQLiteTbl *> tables;
	if (tables.isEmpty()) {
		tables.append(&tagTbl);
		tables.append(&msgtagsTbl);
	}
	return tables;
}

TagDb *globTagDbPtr = 0;
