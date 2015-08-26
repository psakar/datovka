/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#ifndef _MESSAGE_DB_SET_CONTAINER_H_
#define _MESSAGE_DB_SET_CONTAINER_H_


#include <QMap>
#include <QString>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"

/*!
 * @brief Database container.
 *
 * TODO -- Should there be a single globally accessible instance?
 *     (Actually no singleton.)
 */
class DbContainer : private QMap<QString, MessageDb *> {

public:
	DbContainer(void);
	~DbContainer(void);

	/*!
	 * @brief Access/create+open message database related to item.
	 *
	 * @param[in] primaryKey Part of database file name, usually the login.
	 * @param[in] locDir     Directory where to search for the file.
	 * @param[in] testing    True for testing accounts.
	 * @param[in] create     Whether to create non-existing file.
	 * @return Pointer to database, zero pointer on error.
	 */
	MessageDb * accessMessageDb(const QString &primaryKey,
	    const QString &locDir, bool testing, bool create);

	/*!
	 * @brief Creates a copy of the current database into a given new
	 *     directory.
	 *
	 * @param[in] newLocDir  New location directory.
	 * @return True if database was copied and re-opened.
	 */
	bool copyMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Move message database into a new directory.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was moved and re-opened.
	 */
	bool moveMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Re-open a new empty database file. The old file is left
	 *     untouched.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was re-opened.
	 */
	bool reopenMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Delete message db file.
	 *
	 * @param db Deleted database.
	 * @return True on success.
	 */
	bool deleteMessageDb(MessageDb *db);

private:
	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

	/*!
	 * @brief Creates the database name from supplied information.
	 *
	 * @param[in] primaryKey ISDS user name.
	 * @param[in] locDir     Directory where to store the file.
	 * @param[in] testing    Whether it is a testing account.
	 * @return Path to database file.
	 */
	static
	QString constructDbFileName(const QString &primaryKey,
	    const QString &locDir, bool testing);
};


/*!
 * @brief Global database container.
 */
extern DbContainer *globMessageDbsPtr;


#endif /* _MESSAGE_DB_SET_CONTAINER_H_ */
