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

#ifndef _MESSAGE_DB_SINGLE_H_
#define _MESSAGE_DB_SINGLE_H_

#include <QString>

#include "src/io/message_db.h"

/*!
 * Encapsulates only a single database file.
 */
class MessageDbSingle {

public:
	/* Constructor is private. */

	/*!
	 * @brief Destructor.
	 */
	~MessageDbSingle(void);

	/*!
	 * @brief Creates new object.
	 *
	 * @param[in] filePath         Full path to the database file.
	 * @param[in] connectionPrefix Prefix of the connection name.
	 * @return Pointer to new container or zero pointer on error.
	 */
	static
	MessageDbSingle *createNew(const QString &filePath,
	    const QString &connectionPrefix);

	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

	/*!
	 * @brief Check whether required SQL driver is present.
	 *
	 * @return True if database driver is present.
	 */
	static
	bool dbDriverSupport(void);

private:
	/*!
	 * @brief Constructor.
	 *
	 * @note This container ignores file naming conventions.
	 */
	MessageDbSingle(void);

	MessageDb *m_db; /*!< The encapsulated database. */

public:
	/*!
	 * @brief Return all message ID from database.
	 *
	 * @return message id list.
	 */
	QList<MessageDb::MsgId> getAllMessageIDsFromDB(void) const;
};

#endif /* _MESSAGE_DB_SINGLE_H_ */
