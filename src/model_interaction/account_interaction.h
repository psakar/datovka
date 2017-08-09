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

#ifndef _ACCOUNT_INTERACTION_H_
#define _ACCOUNT_INTERACTION_H_

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QString>

class MessageDbSet; /* Forward declaration. */

/*!
 * @brief Provides a namespace for convenience functions dealing with the
 *     account container.
 *
 * @note The account container is considered to be part of the account model.
 *     The methods are not part of the model because the code has to be
 *     separated from GUI dependencies.
 */
class AccountInteraction {
	Q_DECLARE_TR_FUNCTIONS(AccountInteraction)

private:
	/*!
	 * @brief Private constructor.
	 */
	AccountInteraction(void);

public:
	enum AccessStatus {
		AS_OK = 0, /*!< Database successfully opened. */
		AS_DB_PRESENT, /*!< Database already present even though it should not exist. */
		AS_DB_NOT_PRESENT, /*!< Database is not present even though it should exist. */
		AS_DB_NOT_FILES, /*!< Some of the database locations are not files. */
		AS_DB_FILES_INACCESSIBLE, /*!< Some files cannot be accessed. */
		AS_DB_FILES_CORRUPT, /*!< Some files do not contain valid database. */
		AS_DB_CONFUSING_ORGANISATION, /*!< Multiple organisation formats within same location. */
		AS_ERR /*!< Generic error. */
	};

	/*!
	 * @brief Accesses database set for supplied user name.
	 *
	 * @note The returned pointer must not be freed.
	 *
	 * @param[in]  userName User name identifying the account.
	 * @param[out] status Return status while accessing the databases.
	 * @param[out] dbDir Database location directory.
	 * @param[out] namesStr Database file names.
	 * @return Pointer to database set or Q_NULLPTR on error.
	 */
	static
	MessageDbSet *accessDbSet(const QString &userName,
	    enum AccessStatus &status, QString &dbDir, QString &namesStr);
};

#endif /* _ACCOUNT_INTERACTION_H_ */
