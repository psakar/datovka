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

#ifndef _DOCUMENT_SERVICE_DB_H_
#define _DOCUMENT_SERVICE_DB_H_

#include <QList>
#include <QString>

#include "src/io/sqlite/db.h"

/*!
 * @brief Encapsulates document service database.
 */
class DocumentServiceDb : public SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit DocumentServiceDb(const QString &connectionName);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName      File name.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName);

private:
	/*!
	 * @brief Returns list of tables.
	 *
	 * @return List of pointers to tables.
	 */
	static
	QList<class SQLiteTbl *> listOfTables(void);
};

/*!
 * @brief Global document service database.
 */
extern DocumentServiceDb *globDocumentServiceDbPtr;

#endif /* _DOCUMENT_SERVICE_DB_H_ */
