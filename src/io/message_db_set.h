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

#include <QDateTime>
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

	/* Constructor is private. */

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
	 * @brief Computes the secondary key from supplied date.
	 */
	QString secondaryKey(const QDateTime &time) const;

	/*!
	 * @brief Accesses message database file matching the secondary key.
	 *
	 * @param secondaryKey Secondary key.
	 * @param write        Whether the database should be written.
	 * @return Message database or zero pointer on error.
	 */
	MessageDb *accessMessageDb(const QDateTime &deliveryTime, bool write);

	/*!
	 * @brief Creates a new object.
	 *
	 * @param[in] locDir        Directory that holds the database files.
	 * @param[in] primaryKey    Primary key, usually the user name.
	 * @param[in] testing       True if this is a testing account.
	 * @param[in] organisation How to organise the database files.
	 */
	static
	MessageDbSet *createNew(const QString &locDir,
	    const QString &primaryKey, bool testing, Organisation organisation,
	    bool mustExist);

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
	 * @brief Returns the secondary key from a file name.
	 *
	 * @param[in] fileName     File name (without path).
	 * @param[in] organisation Organisation type.
	 * @return Secondary key, null string on error.
	 */
	static
	QString secondaryKeyFromFileName(const QString &fileName,
	    Organisation organisation);

	/*!
	 * @brief Returns the database file names stored in the location.
	 *
	 * @note If DO_UNKNOWN is passed via `organisation` then all possible
	 *     matching file names are returned.
	 *
	 * @param[in] locDir       Directory where the database should reside.
	 * @param[in] primaryKey   Primary key.
	 * @param[in] testing      True if a testing account.
	 * @paran[on] organisation Organisation type.
	 * @return List of database files in the location.
	 */
	static
	QStringList existingDbFileNamesInLocation(const QString &locDir,
	    const QString &primaryKey, bool testing, Organisation organisation);

	/*!
	 * @brief Construct key from primary and secondary key.
	 *
	 * @param[in] primaryKey   Primary key.
	 * @param[in] secondaryKey Secondary key.
	 * @paran[on] organisation Organisation type.
	 * @return Key or null string on error.
	 */
	static
	QString constructKey(const QString &primaryKey,
	    const QString &secondaryKey, Organisation organisation);

	/*!
	 * @brief Returns file name according to primary and secondary key.
	 *
	 * @param[in] locDir       Directory where the database should reside.
	 * @param[in] primaryKey   Primary key.
	 * @param[in] secondaryKey Secondary key.
	 * @param[in] testing      True if a testing account.
	 * @paran[on] organisation Organisation type.
	 * @return Full file path or empty string on error.
	 */
	static
	QString constructDbFileName(const QString &locDir,
	    const QString &primaryKey, const QString &secondaryKey,
	    bool testing, Organisation organisation);

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
	 * @brief Creates a database set.
	 *
	 * @param[in] locDir     Directory that holds the database files.
	 * @param[in] primaryKey Primary key, usually the user name.
	 * @param[in] testing    True if this is a testing account.
	 * @param[in] organisation How to organise the database files.
	 */
	MessageDbSet(const QString &locDir, const QString &primaryKey,
	    bool testing, Organisation organisation);

	/*!
	 * @brief Accesses message database file matching the secondary key.
	 *
	 * @param[in] secondaryKey Secondary key.
	 * @param[in] create       Whether to create the file.
	 * @return Message database or zero pointer on error.
	 */
	MessageDb *_accessMessageDb(const QString &secondaryKey, bool create);

	const QString m_primaryKey; /*!< Used for accessing the database. */
	const bool m_testing; /*!< Whether those are a testing databases. */
	QString m_locDir; /*!< Directory where the files reside. */
	Organisation m_organisation; /*!< How the database is organised. */
};

#endif /* _MESSAGE_DB_SET_H_ */
