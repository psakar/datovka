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

#ifndef _ACCOUNT_DB_H_
#define _ACCOUNT_DB_H_

#include <QMap>
#include <QString>
#include <QVariant>

#include "src/datovka_shared/io/sqlite/db.h"

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
	 * @param[in] key   Key string.
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
	 * @param[in] key          Key string.
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
class AccountDb : public SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit AccountDb(const QString &connectionName);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName      File name.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName);

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
	 * @param[in] key          Key value.
	 * @param[in] defaultValue Value to be returned when nothing found.
	 * @return Data-box identifier.
	 */
	const QString dbId(const QString &key,
	    const QString &defaultValue = QString()) const;

	/*!
	 * @brief Return sender name guess.
	 *
	 * @param[in] key          Key value.
	 * @param[in] defaultValue Value to be returned when nothing found.
	 * @return Sender name.
	 */
	const QString senderNameGuess(const QString &key,
	    const QString &defaultValue = QString()) const;

	/*!
	 * @brief Return pwd expiration info from db.
	 *
	 * @param[in] key Key value.
	 * @return Expiration information.
	 */
	const QString getPwdExpirFromDb(const QString &key) const;

	/*!
	 * @brief Checks whether password expires in given period.
	 *
	 * @param[in] key Key value.
	 * @param[in] days Amount of days to check the expiration.
	 * @return Non-negative value if password expires within given amount
	 *     of days. Negative value else.
	 */
	int pwdExpiresInDays(const QString &key, int days) const;

	/*!
	 * @brief Set password expiration information.
	 *
	 * @param[in] key  Key value.
	 * @param[in] date Expiration date.
	 * @return True on success.
	 */
	bool setPwdExpirIntoDb(const QString &key, const QString &date);

	/*!
	 * @brief Insert account info into database.
	 *
	 * @return True on success.
	 */
	bool insertAccountIntoDb(const QString &key, const QString &dbID,
	    const QString &dbType, int ic, const QString &pnFirstName,
	    const QString &pnMiddleName, const QString &pnLastName,
	    const QString &pnLastNameAtBirth, const QString &firmName,
	    const QString &biDate, const QString &biCity,
	    const QString &biCounty, const QString &biState,
	    const QString &adCity, const QString &adStreet,
	    const QString &adNumberInStreet,
	    const QString &adNumberInMunicipality, const QString &adZipCode,
	    const QString &adState, const QString &nationality,
	    const QString &identifier, const QString &registryCode,
	    int dbState, bool dbEffectiveOVM, bool dbOpenAddressing);

	/*!
	 * @brief Insert user info into database.
	 *
	 * @return True on success.
	 */
	bool insertUserIntoDb(const QString &key,
	    const QString &userType, int userPrivils,
	    const QString &pnFirstName, const QString &pnMiddleName,
	    const QString &pnLastName, const QString &pnLastNameAtBirth,
	    const QString &adCity, const QString &adStreet,
	    const QString &adNumberInStreet,
	    const QString &adNumberInMunicipality, const QString &adZipCode,
	    const QString &adState,
	    const QString &biDate,
	    int ic, const QString &firmName, const QString &caStreet,
	    const QString &caCity, const QString &caZipCode,
	    const QString &caState);

	/*!
	 * @brief delete account info from database.
	 *
	 * @param[in] key Key value.
	 * @return True on success.
	 */
	bool deleteAccountInfo(const QString &key);

	/*!
	 * @brief Get data box information.
	 *
	 * @param[in] key Key value.
	 */
	QList<QString> getUserDataboxInfo(const QString &key) const;

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

/*!
 * @brief Global account database.
 */
extern AccountDb *globAccountDbPtr;

#endif /* _ACCOUNT_DB_H_ */
