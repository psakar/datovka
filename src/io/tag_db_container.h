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

#ifndef _TAG_DB_CONTAINER_H_
#define _TAG_DB_CONTAINER_H_


#include <QMap>
#include <QString>

#include "src/io/tag_db.h"

/*!
 * @brief Database container.
 *
 * TODO -- Should there be a single globally accessible instance?
 *     (Actually no singleton.)
 */
class TagDbContainer : private QMap<QString, TagDb *> {
public:
	explicit TagDbContainer(const QString &connectionName);
	~TagDbContainer(void);

	/*!
	 * @brief Access/create+open message database set related to item.
	 *
	 * @param[in] primaryKey   Part of database file name, usually the login.
	 * @return Pointer to database, zero pointer on error.
	 */
	TagDb *accessTagDb(const QString &key);

	/*!
	 * @brief Delete tag database.
	 *
	 * @param db Deleted database.
	 * @return True on success.
	 */
	bool deleteDb(TagDb *db);

	/*!
	 * @brief Creates the database name from supplied information.
	 *
	 * @param[in] key     user name.
	 * @return Path to database file.
	 */
	static
	QString constructDbTagName(const QString &key);

	const QString m_connectionName;
};

#endif /* _TAG_DB_CONTAINER_H_ */
