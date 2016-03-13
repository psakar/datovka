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

	static
	const QString memoryLocation; /*!< Specifies memory location. */

protected:
	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName File name or memory location.
	 * @param[in] tables   List of table prototypes that should be created
	 *                     if missing.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName,
	    const QList<class SQLiteTbl *> &tables);

	QSqlDatabase m_db; /*!< Database. */

private:
	static
	const QString dbDriverType; /*!< Database driver name. */
};

#endif /* _SQLITE_DB_H_ */
