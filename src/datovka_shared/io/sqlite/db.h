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

#include <QFlags>
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
	 * @brief Database opening flags.
	 */
	enum OpenFlag {
		NO_OPTIONS = 0x00, /*!< No option specified. */
		CREATE_MISSING = 0x01, /*!< Missing tables and entries are going to be created. */
		FORCE_IN_MEMORY = 0x02 /*!< Database is going to be opened in memory. */
	};
	Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit SQLiteDb(const QString &connectionName);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~SQLiteDb(void);

	/*!
	 * @brief Get file name.
	 *
	 * @return File name holding the database.
	 */
	QString fileName(void) const;

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
	 * @param[in] savePointName Name of the save point.
	 * @return True on success.
	 */
	bool savePoint(const QString &savePointName);

	/*!
	 * @brief End named transaction.
	 *
	 * @param[in] savePointName Name of the save point.
	 * @return True on success.
	 */
	bool releaseSavePoint(const QString &savePointName);

	/*!
	 * @brief Roll back transaction.
	 *
	 * @param[in] savePointName Name of the save point.
	 * @return True on success.
	 *
	 * @note If no save-point name is supplied then a complete roll-back is
	 *     performed.
	 */
	bool rollbackTransaction(const QString &savePointName = QString());

	static
	const QString memoryLocation; /*!< Specifies memory location. */
	static
	const QString dbDriverType; /*!< Database driver name. */

	/*!
	 * @brief Check whether required SQL driver is present.
	 *
	 * @return True if database driver is present.
	 */
	static
	bool dbDriverSupport(void);

protected:
	/*!
	 * @brief Close database file.
	 */
	void closeDb(void);

	/*!
	 * @brief Perform a database integrity check.
	 *
	 * @return False if check fails.
	 */
	bool checkDb(bool quick);

	/*!
	 * @brief Performs database clean-up (VACUUM).
	 *
	 * @return False on error.
	 */
	bool vacuum(void);

	/*!
	 * @brief Returns list of tables encompassed in the database.
	 *
	 * @return List of pointers to tables.
	 *
	 * @note Override this method in derived classes. This method is
	 *     used to create missing database tables.
	 */
	virtual
	QList<class SQLiteTbl *> listOfTables(void) const = 0;

	/*!
	 * @brief This function is used to make database content consistent
	 *     (e.g. adding missing columns or entries).
	 *
	 * @return True on success. If this method returns false, then the
	 *     open procedure must fail. Override this method to implement
	 *     custom checks. This method just returns true.
	 */
	virtual
	bool assureConsistency(void);

	/*!
	 * @brief Copy db.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 *
	 * @note The copy is continued to be used. Original is closed.
	 */
	bool copyDb(const QString &newFileName, enum OpenFlag flag);

	/*!
	 * @brief Open a new empty database file.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 *
	 * @note The old database file is left untouched.
	 */
	bool reopenDb(const QString &newFileName, enum OpenFlag flag);

	/*!
	 * @brief Move db.
	 *
	 * @param[in] newFileName New file path.
	 * @param[in] flag Can be NO_OPTIONS or CREATE_MISSING.
	 * @return True on success.
	 */
	bool moveDb(const QString &newFileName, enum OpenFlag flag);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName File name.
	 * @param[in] flags Database opening flags.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName, OpenFlags flags);

	/*!
	 * @brief Attaches a database file to opened database.
	 *
	 * @param[in,out] query Query to work with.
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
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SQLiteDb::OpenFlags)
