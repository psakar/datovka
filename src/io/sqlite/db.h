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

#ifndef _SQLITE_DB_H_
#define _SQLITE_DB_H_

#include <QList>
#include <QSqlDatabase>
#include <QString>

#define DB2 "db2"

/*!
 * @brief Database prototype.
 */
class SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit SQLiteDb(const QString &connectionName);

	/*!
	 * @brief Destructor.
	 */
	~SQLiteDb(void);

	/*!
	 * @brief Get file name.
	 *
	 * @return File name holding the database.
	 */
	QString fileName(void) const;

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName      File name.
	 * @param[in] forceInMemory True if the message should be stored in
	 *                          memory only.
	 * @param[in] tables        List of table prototypes that should be
	 *                          created if missing.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName, bool forceInMemory,
	    const QList<class SQLiteTbl *> &tables);

	/*!
	 * @brief Close database file.
	 */
	void closeDb(void);

	/*!
	 * @brief Begin a transaction.
	 *
	 * @return True on success.
	 */
	bool beginTransaction(void);

	/*!
	 * @brief End transaction.
	 */
	bool commitTransaction(void);

	/*!
	 * @brief Begin named transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 */
	bool savePoint(const QString &savePointName);

	/*!
	 * @brief End named transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 */
	bool releaseSavePoint(const QString &savePointName);

	/*!
	 * @brief Roll back transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 *
	 * @note If no save-point name is supplied then a complete roll-back is
	 *     performed.
	 */
	bool rollbackTransaction(const QString &savePointName = QString());

	static
	const QString memoryLocation; /*!< Specifies memory location. */

protected:
	/*!
	 * @brief Perform a db integrity check.
	 *
	 * @return False if check fails.
	 */
	bool checkDb(bool quick);

	/*!
	 * @brief Attaches a database file to opened database.
	 *
	 * @param[in,out] query          Query to work with.
	 * @param[in]     attachFileName File containing database to be
	 *                               attached.
	 * @return False on error.
	 */
	static
	bool attachDb2(class QSqlQuery &query, const QString &attachFileName);

	/*!
	 * @brief Detaches attached database file from opened database.
	 *
	 * @param[in,out] query Query to work with.
	 * @return False on error.
	 */
	static
	bool detachDb2(class QSqlQuery &query);

	QSqlDatabase m_db; /*!< Database. */

private:
	/*!
	 * @brief Create empty tables if tables do not already exist.
	 *
	 * @param[in] tables List of table prototypes that should be created
	 *                   if missing.
	 * @return True on success.
	 */
	bool createEmptyMissingTables(const QList<class SQLiteTbl *> &tables);

	static
	const QString dbDriverType; /*!< Database driver name. */
};

#endif /* _SQLITE_DB_H_ */
