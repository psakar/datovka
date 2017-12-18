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

#pragma once

#include "src/datovka_shared/io/sqlite/db.h"

/*!
 * @brief Database prototype. The database consists only of a single file.
 */
class SQLiteDbSingle : public SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 * @param[in] allowRelocation If true then database can change location
	 *                            (e.g. be moved to a different file).
	 */
	explicit SQLiteDbSingle(const QString &connectionName,
	    bool allowRelocation);

	/*!
	 * @brief Copy db.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 *
	 * @note The copy is continued to be used. Original is closed.
	 */
	bool copyDb(const QString &newFileName, enum SQLiteDb::OpenFlag flag);

	/*!
	 * @brief Open a new empty database file.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 *
	 * @note The old database file is left untouched.
	 */
	bool reopenDb(const QString &newFileName, enum SQLiteDb::OpenFlag flag);

	/*!
	 * @brief Move db.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 */
	bool moveDb(const QString &newFileName, enum SQLiteDb::OpenFlag flag);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName File name.
	 * @param[in] flags Database opening flags.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName, SQLiteDb::OpenFlags flags);

	/* Make some inherited methods public. */
	using SQLiteDb::checkDb;
	using SQLiteDb::vacuum;

private:
	const bool m_relocatable; /*!<
	                           * If set to true, then database can be
	                           * moved to a different file.
	                           */
};
