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

#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QVariant>

#include "src/datovka_shared/io/sqlite/db_single.h"
#include "src/isds/box_interface.h"

/*!
 * @brief Information obtained from database. It is structured only by the
 *      queried keys.
 */
class DbEntry : private QMap<QString, QVariant> {

public:
	/*!
	 * @brief Constructor.
	 */
	DbEntry(void);

	/*!
	 * @brief Set value.
	 *
	 * @param[in] key Key string.
	 * @param[in] value Value to be stored.
	 */
	bool setValue(const QString &key, const QVariant &value);

	/*!
	 * @brief Check whether value is stored.
	 *
	 * @param[in] key Key string.
	 * @return True if key found, False else.
	 */
	bool hasValue(const QString &key) const;

	/*!
	 * @brief Return stored value.
	 *
	 * @param[in] key  Key string.
	 * @param[in] defaultValue Value to be returned if key not found.
	 * @return Found value associated to key or defaultValue if such entry
	 *     found.
	 */
	const QVariant value(const QString &key,
	    const QVariant &defaultValue = QVariant()) const;

private:
	typedef QMap<QString, QVariant> m_parentType;
};

/*!
 * @brief Encapsulates account database.
 */
class AccountDb : public SQLiteDbSingle {

public:
	/* Use parent class constructor. */
	using SQLiteDbSingle::SQLiteDbSingle;

	/*!
	 * @brief Return account entry.
	 *
	 * @param[in] key Key value in format 'login___True'.
	 * @return Associated entries or empty container on error.
	 */
	DbEntry accountEntry(const QString &key) const;

	/*!
	 * @brief Return user entry.
	 *
	 * @param[in] key Key value in format 'login___True'.
	 * @return Associated entries or empty container on error.
	 */
	DbEntry userEntry(const QString &key) const;

	/*!
	 * @brief Return data box identifier.
	 *
	 * @param[in] key Key value.
	 * @param[in] defaultValue Value to be returned when nothing found.
	 * @return Databox identifier.
	 */
	const QString dbId(const QString &key,
	    const QString &defaultValue = QString()) const;

	/*!
	 * @brief Return sender name guess.
	 *
	 * @param[in] key Key value.
	 * @param[in] defaultValue Value to be returned when nothing found.
	 * @return Sender name.
	 */
	const QString senderNameGuess(const QString &key,
	    const QString &defaultValue = QString()) const;

	/*!
	 * @brief Return pwd expiration info from db.
	 *
	 * @param[in] key Key value.
	 * @return Expiration information, null value if password does not expire.
	 */
	QDateTime getPwdExpirFromDb(const QString &key) const;

	/*!
	 * @brief Checks whether password expires in given period.
	 *
	 * @param[in] key Key value.
	 * @param[in] days Amount of days to check the expiration.
	 * @return Non-negative value if password expires within given amount
	 *                      of days. Negative value else.
	 */
	int pwdExpiresInDays(const QString &key, int days) const;

	/*!
	 * @brief Insert or update password expiration information in db.
	 *
	 * @param[in] key Key value.
	 * @param[in] date Expiration date.
	 * @return True on success.
	 */
	bool setPwdExpirIntoDb(const QString &key, const QDateTime &date);

	/*!
	 * @brief Insert account info into database.
	 *
	 * @param[in] key Key value.
	 * @param[in] dbOwnerInfo Owner info structure.
	 * @return True on success.
	 */
	bool insertAccountIntoDb(const QString &key,
	    const Isds::DbOwnerInfo &dbOwnerInfo);

	/*!
	 * @brief Insert user info into database.
	 *
	 * @param[in] key Key value.
	 * @param[in] dbUserInfo User info structure.
	 * @return True on success.
	 */
	bool insertUserIntoDb(const QString &key,
	    const Isds::DbUserInfo &dbUserInfo);

	/*!
	 * @brief Delete account info from database.
	 *
	 * @param[in] key Key value.
	 * @return True on success.
	 */
	bool deleteAccountInfo(const QString &key);

	/*!
	 * @brief Get account/owner info from database.
	 *
	 * @param[in] key Key value.
	 * @return DbOwnerInfo structure - account info.
	 */
	const Isds::DbOwnerInfo getOwnerInfo(const QString &key) const;

	/*!
	 * @brief Return key used to access user entries in account database.
	 *
	 * @param[in] userName User name to construct key from.
	 * @return Key value in format 'login___True'
	 */
	static
	QString keyFromLogin(const QString &login);

protected:

	/*!
	 * @brief Returns list of tables.
	 *
	 * @return List of pointers to tables.
	 */
	virtual
	QList<class SQLiteTbl *> listOfTables(void) const Q_DECL_OVERRIDE;
};
