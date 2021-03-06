/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QMap>
#include <QObject>
#include <QString>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"

/*!
 * @brief Database container.
 *
 * TODO -- Should there be a single globally accessible instance?
 *     (Actually no singleton.)
 */
class DbContainer : public QObject, private QMap<QString, MessageDbSet *> {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionPrefix Database connection prefix.
	 */
	explicit DbContainer(const QString &connectionPrefix = QString());

	/*!
	 * @brief Destructor.
	 */
	~DbContainer(void);

	/*!
	 * @brief Access/create+open message database set related to item.
	 *
	 * @param[in] locDir       Directory where to search for the file.
	 * @param[in] primaryKey   Part of database file name, usually the login.
	 * @param[in] testing      True for testing accounts.
	 * @param[in] organisation Way how the database is organised.
	 * @param[in] manner       How to treat files when opening database.
	 * @return Pointer to database, Q_NULLPTR on error.
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

signals:
	/*!
	 * @brief Emitted when a database file is opened.
	 *
	 * @param[in] primaryKey Primary key, usually the username.
	 */
	void opened(const QString &primaryKey);

private slots:
	/*!
	 * @brief This slot must be connected to every database set created
	 *     inside this container.
	 *
	 * @param[in] primaryKey Primary key, usually the username.
	 */
	void watchOpened(const QString &primaryKey);

private:
	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

	const QString m_connectionPrefix; /*!< Database connection prefix. */
};
