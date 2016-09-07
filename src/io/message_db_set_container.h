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
class DbContainer : private QMap<QString, MessageDbSet *> {

public:
	DbContainer(const QString &connectionPrefix = QString());
	~DbContainer(void);

	/*!
	 * @brief Access/create+open message database set related to item.
	 *
	 * @param[in] locDir       Directory where to search for the file.
	 * @param[in] primaryKey   Part of database file name, usually the login.
	 * @param[in] testing      True for testing accounts.
	 * @param[in] organisation Way how the database is organised.
	 * @param[in] manner       How to treat files when opening database.
	 * @return Pointer to database, zero pointer on error.
	 */
	MessageDbSet *accessDbSet(const QString &locDir,
	    const QString &primaryKey, bool testing,
	    MessageDbSet::Organisation organisation,
	    enum MessageDbSet::CreationManner manner);

	/*!
	 * @brief Delete all files related to dbset.
	 *
	 * @param dbSet Deleted database set.
	 * @return True on success.
	 */
	bool deleteDbSet(MessageDbSet *dbSet);

private:
	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

	const QString m_connectionPrefix; /*!< Database connection prefix. */
};


/*!
 * @brief Global database container.
 */
extern DbContainer *globMessageDbsPtr;


#endif /* _MESSAGE_DB_SET_CONTAINER_H_ */
