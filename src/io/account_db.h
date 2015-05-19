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


#ifndef _ACCOUNT_DB_H_
#define _ACCOUNT_DB_H_


#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>


/*!
 * @brief Account information.
 */
class AccountEntry : private QMap<QString, QVariant> {

public:
	AccountEntry(void);
	~AccountEntry(void);

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
class AccountDb : public QObject {

public:
	AccountDb(const QString &connectionName, QObject *parent = 0);
	~AccountDb(void);

	/*!
	 * @brief Open database file.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Return account entry.
	 *
	 * @note Key is in format 'login___True'.
	 */
	AccountEntry accountEntry(const QString &key) const;

	/*!
	 * @brief Return data box identifier.
	 */
	const QString dbId(const QString &key,
	    const QString &defaultValue = QString()) const;

	/*!
	 * @brief Return sender name guess.
	 */
	const QString senderNameGuess(const QString &key,
	    const QString &defaultValue = QString()) const;


	/*!
	 * @brief Return pwd expiration info from db.
	 */
	const QString getPwdExpirFromDb(const QString &key) const;

 	/*!
	 * @brief Set pwd expiration to password_expiration_date table
	 */
	bool setPwdExpirIntoDb(const QString &key, const QString &date);

	/*!
	 * @brief Insert account info into db
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
	 * @brief Insert user info into db
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
	 * @brief delete account info from db
	 */
	bool deleteAccountInfo(const QString &key);

	QList<QString> getUserDataboxInfo(const QString &key) const;

	static
	const QString memoryLocation;

	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

private:
	QSqlDatabase m_db; /*!< Account database. */

	/*!
	 * @brief Create empty tables if tables do not already exist.
	 *
	 * @return True on success.
	 */
	bool createEmptyMissingTables(void);
};


#endif /* _ACCOUNT_DB_H_ */
