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

#ifndef _MESSAGE_DB_SET_H_
#define _MESSAGE_DB_SET_H_

#include <QMap>
#include <QString>
#include <QStringList>

#include "src/io/message_db.h"

/*!
 * Organises database files according to secondary keys.
 */
class MessageDbSet : private QMap<QString, MessageDb *> {

public:
	/*!
	 * Style of database organisation.
	 */
	enum Organisation {
		DO_UNKNOWN = 0, /*!< Unsupported type. */
		DO_SINGLE_FILE, /*!< Content is stored within a single file. */
		DO_YEARLY /*!< Content is split according to delivery year. */
	};

	/*!
	 * @brief Creates a database set.
	 *
	 * @param[in] primaryKey   Primary key, usually the user name.
	 * @param[in] locDir       Directory that holds the database files.
	 * @param[in] testing      True if this is a testing account.
	 * @param[in] create       Whether to create missing file.
	 * @param[in] organisation How to organise the database files.
	 */
	MessageDbSet(const QString &primaryKey, const QString &locDir,
	    bool testing, bool create, Organisation organisation);

	/*!
	 * @brief Destructor.
	 */
	~MessageDbSet(void);

	/*!
	 * @brief Creates a copy of the current database into a given new
	 *     directory.
	 *
	 * @param[in] newLocDir  New location directory.
	 * @return True if database was copied and re-opened.
	 */
	bool copyToLocation(const QString &newLocDir);

	/*!
	 * @brief Move message database into a new directory.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was moved and re-opened.
	 */
	bool moveToLocation(const QString &newLocDir);

	/*!
	 * @brief Re-open a new empty database file. The old file is left
	 *     untouched.
	 *
	 * @param[in] newLocDir New location directory.
	 * @paran[on] organisation Organisation type.
	 * @return True if database was re-opened.
	 */
	bool reopenLocation(const QString &newLocDir,
	    Organisation organisation);

	/*!
	 * @brief Delete associated message db files in location.
	 *
	 * @return True on success.
	 */
	bool deleteLocation(void);

	/*!
	 * @brief Get the organisation type of the database.
	 *
	 * @param[in] locDir     Directory where the database resides.
	 * @param[in] primaryKey Primary key.
	 * @param[in] testing    True if a testing account.
	 * @return    Organisation type.
	 */
	static
	Organisation dbOrganisation(const QString &locDir,
	    const QString &primaryKey, bool testing);

	/*!
	 * Brief returns the database file names stored in the location.
	 *
	 * @param[in] locDir       Directory where the database should reside.
	 * @param[in] primaryKey   Primary key.
	 * @param[in] testing      True if a testing account.
	 * @paran[on] organisation Organisation type.
	 * @return List of database files in the location.
	 */
	static
	QStringList existingDbFilesInLocation(const QString &locDir,
	    const QString &primaryKey, bool testing, Organisation organisation);

private:
	const QString m_primaryKey; /*!< Used for accessing the database. */
	const bool m_testing; /*!< Whether those are a testing databases. */
	QString m_locDir; /*!< Directory where the files reside. */
	Organisation m_organisation; /*!< How the database is organised. */
};

#endif /* _MESSAGE_DB_SET_H_ */
