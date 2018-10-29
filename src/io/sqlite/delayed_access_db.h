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

#pragma once

#include <QSqlDatabase>
#include <QString>

#include "src/datovka_shared/io/sqlite/db.h"

class DelayedAccessSQLiteDb : public SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit DelayedAccessSQLiteDb(const QString &connectionName);

	/*!
	 * @brief Get file name.
	 *
	 * @return File name holding the database.
	 */
	QString fileName(void) const;

protected:
	/*!
	 * @brief Copy db.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 *
	 * @note The copy is continued to be used. Original is closed.
	 *     The copy is immediately opened.
	 */
	bool copyDb(const QString &newFileName, enum OpenFlag flag);

	/*!
	 * @brief Open of database file.
	 *
	 * @note It just allows the opening of the database file. The flags
	 *     are file name are stored for later opening. Only memory
	 *     databases are opened immediately.
	 *
	 * @param[in] fileName File name.
	 * @param[in] flags Database opening flags.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName, SQLiteDb::OpenFlags flags);

	/*!
	 * @brief Check whether underlying database is really opened.
	 *
	 * @return True if database connection is opened.
	 */
	bool isOpen(void) const;

	/*!
	 * @brief Access the database connection.
	 *
	 * @return An invalid database connection if the underlying database
	 *     connection cannot be opened.
	 */
	const QSqlDatabase &accessDb(void);

private:
	/*!
	 * @brief Access the database connection.
	 *
	 * @return True if database was opened.
	 */
	bool _accessDb(void);

	/* Hide some inherited content. */
	using SQLiteDb::closeDb;
	using SQLiteDb::reopenDb;
	using SQLiteDb::moveDb;
	using SQLiteDb::m_db;

	QString m_fileName; /*!< File name, if empty, then openDb was not called. */
	SQLiteDb::OpenFlags m_flags; /*!< Opening flags. */
};
