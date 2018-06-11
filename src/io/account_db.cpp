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

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "src/datovka_shared/isds/type_conversion.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/db_tables.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"

DbEntry::DbEntry(void)
    : QMap<QString, QVariant>()
{
}

bool DbEntry::setValue(const QString &key, const QVariant &value)
{
	/* Don't perform any check against database table. */
	this->insert(key, value);
	return true;
}

bool DbEntry::hasValue(const QString &key) const
{
	return this->contains(key);
}

const QVariant DbEntry::value(const QString &key,
    const QVariant &defaultValue) const
{
	return m_parentType::value(key, defaultValue);
}

DbEntry AccountDb::accountEntry(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	DbEntry aEntry;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (accntinfTbl.knownAttrs.size() - 1); ++i) {
		queryStr += accntinfTbl.knownAttrs[i].first + ", ";
	}
	queryStr += accntinfTbl.knownAttrs.last().first + " ";
	queryStr += "FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() && query.first()) {
		QSqlRecord rec = query.record();
		for (int i = 0; i < accntinfTbl.knownAttrs.size(); ++i) {
			QVariant value = query.value(rec.indexOf(
			    accntinfTbl.knownAttrs[i].first));
			if (!value.isNull() && value.isValid()) {
				aEntry.setValue(accntinfTbl.knownAttrs[i].first,
				    value);
			}
		}
		return aEntry;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return DbEntry();
}

DbEntry AccountDb::userEntry(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	DbEntry uEntry;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (userinfTbl.knownAttrs.size() - 1); ++i) {
		queryStr += userinfTbl.knownAttrs[i].first + ", ";
	}
	queryStr += userinfTbl.knownAttrs.last().first + " ";
	queryStr += "FROM user_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() && query.first()) {
		QSqlRecord rec = query.record();
		for (int i = 0; i < userinfTbl.knownAttrs.size(); ++i) {
			QVariant value = query.value(rec.indexOf(
			    userinfTbl.knownAttrs[i].first));
			if (!value.isNull() && value.isValid()) {
				uEntry.setValue(userinfTbl.knownAttrs[i].first,
				    value);
			}
		}
		return uEntry;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return DbEntry();
}

const QString AccountDb::dbId(const QString &key,
    const QString &defaultValue) const
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT dbID FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toString();
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return defaultValue;
}

const QString AccountDb::senderNameGuess(const QString &key,
    const QString &defaultValue) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	QString name;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT firmName, pnFirstName, pnMiddleName, "
	    "pnLastName FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
	    	/* firmName */
		name = query.value(0).toString();
		if (!(name.isNull() || name.isEmpty())) {
			return name;
		}
		/* pnFirstName */
		name = query.value(1).toString();
		if (!query.value(2).toString().isEmpty()) {
			/* pnMiddleName */
			name += " " + query.value(2).toString();
		}
		/* pnLastName */
		name += " " + query.value(3).toString();
		return name;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return defaultValue;
}

QDateTime AccountDb::getPwdExpirFromDb(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT expDate FROM password_expiration_date "
	    "WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		if (query.first() && query.isValid() &&
		    !query.value(0).toString().isEmpty()) {
			return dateTimeFromDbFormat(query.value(0).toString());
		}
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return QDateTime();
}

int AccountDb::pwdExpiresInDays(const QString &key, int days) const
{
	const QDateTime dbDateTime(getPwdExpirFromDb(key));
	if (dbDateTime.isNull()) {
		return -1;
	}

	const QDate dbDate = dbDateTime.date();
	if (!dbDate.isValid()) {
		return -1;
	}

	qint64 daysTo = QDate::currentDate().daysTo(dbDate);
	if (daysTo > days) {
		return -1;
	}

	return daysTo;
}

bool AccountDb::setPwdExpirIntoDb(const QString &key, const QDateTime &date)
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "INSERT OR REPLACE INTO password_expiration_date "
	    "(key, expDate) VALUES (:key, :expDate)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":expDate", qDateTimeToDbFormat(date));
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return false;
}

bool AccountDb::insertAccountIntoDb(const QString &key,
    const Isds::DbOwnerInfo &dbOwnerInfo)
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "INSERT OR REPLACE INTO account_info "
	    "(key, dbID, dbType, ic, pnFirstName, pnMiddleName, "
	    "pnLastName, pnLastNameAtBirth, firmName, biDate, biCity, "
	    "biCounty, biState, adCity, adStreet, adNumberInStreet, "
	    "adNumberInMunicipality, adZipCode, adState, nationality, "
	    "identifier, registryCode, dbState, dbEffectiveOVM, "
	    "dbOpenAddressing)"
	    " VALUES "
	    "(:key, :dbID, :dbType, :ic, :pnFirstName, :pnMiddleName, "
	    ":pnLastName, :pnLastNameAtBirth, :firmName, :biDate, :biCity, "
	    ":biCounty, :biState, :adCity, :adStreet, :adNumberInStreet, "
	    ":adNumberInMunicipality, :adZipCode, :adState, :nationality, "
	    ":identifier, :registryCode, :dbState, :dbEffectiveOVM, "
	    ":dbOpenAddressing)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":dbID", dbOwnerInfo.dbID());
	query.bindValue(":dbType", Isds::dbType2StrVariant(dbOwnerInfo.dbType()));
	query.bindValue(":ic", dbOwnerInfo.ic());
	query.bindValue(":pnFirstName", dbOwnerInfo.personName().firstName());
	query.bindValue(":pnMiddleName", dbOwnerInfo.personName().middleName());
	query.bindValue(":pnLastName", dbOwnerInfo.personName().lastName());
	query.bindValue(":pnLastNameAtBirth",
	    dbOwnerInfo.personName().lastNameAtBirth());
	query.bindValue(":firmName", dbOwnerInfo.firmName());
	query.bindValue(":biDate", qDateToDbFormat(dbOwnerInfo.birthInfo().date()));
	query.bindValue(":biCity", dbOwnerInfo.birthInfo().city());
	query.bindValue(":biCounty", dbOwnerInfo.birthInfo().county());
	query.bindValue(":biState", dbOwnerInfo.birthInfo().state());
	query.bindValue(":adCity", dbOwnerInfo.address().city());
	query.bindValue(":adStreet", dbOwnerInfo.address().street());
	query.bindValue(":adNumberInStreet",
	    dbOwnerInfo.address().numberInStreet());
	query.bindValue(":adNumberInMunicipality",
	    dbOwnerInfo.address().numberInMunicipality());
	query.bindValue(":adZipCode", dbOwnerInfo.address().zipCode());
	query.bindValue(":adState", dbOwnerInfo.address().state());
	query.bindValue(":nationality", dbOwnerInfo.nationality());
	query.bindValue(":identifier", dbOwnerInfo.identifier());
	query.bindValue(":registryCode", dbOwnerInfo.registryCode());
	query.bindValue(":dbState",
	    Isds::dbState2Variant(dbOwnerInfo.dbState()));
	query.bindValue(":dbEffectiveOVM",
	    Isds::nilBool2Variant(dbOwnerInfo.dbEffectiveOVM()));
	query.bindValue(":dbOpenAddressing",
	    Isds::nilBool2Variant(dbOwnerInfo.dbOpenAddressing()));
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return false;
}

bool AccountDb::insertUserIntoDb(const QString &key,
    const Isds::DbUserInfo &dbUserInfo)
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "INSERT OR REPLACE INTO user_info "
	    "(key, userType, userPrivils, pnFirstName, pnMiddleName, "
	    "pnLastName, pnLastNameAtBirth, adCity, adStreet, "
	    "adNumberInStreet, adNumberInMunicipality, adZipCode, "
	    "adState, biDate, ic, firmName, caStreet, caCity, "
	    "caZipCode, caState)"
	    " VALUES "
	    "(:key, :userType, :userPrivils, :pnFirstName, "
	    ":pnMiddleName, :pnLastName, :pnLastNameAtBirth, :adCity, "
	    ":adStreet, :adNumberInStreet, :adNumberInMunicipality, "
	    ":adZipCode, :adState, :biDate, :ic, :firmName, :caStreet, "
	    ":caCity, :caZipCode, :caState)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":userType", Isds::userType2Str(dbUserInfo.userType()));
	query.bindValue(":userPrivils",
	    Isds::privileges2Variant(dbUserInfo.userPrivils()));
	query.bindValue(":ic", dbUserInfo.ic());
	query.bindValue(":pnFirstName", dbUserInfo.personName().firstName());
	query.bindValue(":pnMiddleName", dbUserInfo.personName().middleName());
	query.bindValue(":pnLastName", dbUserInfo.personName().lastName());
	query.bindValue(":pnLastNameAtBirth",
	    dbUserInfo.personName().lastNameAtBirth());
	query.bindValue(":firmName", dbUserInfo.firmName());
	query.bindValue(":biDate", qDateToDbFormat(dbUserInfo.biDate()));
	query.bindValue(":adCity", dbUserInfo.address().city());
	query.bindValue(":adStreet", dbUserInfo.address().street());
	query.bindValue(":adNumberInStreet",
	    dbUserInfo.address().numberInStreet());
	query.bindValue(":adNumberInMunicipality",
	    dbUserInfo.address().numberInMunicipality());
	query.bindValue(":adZipCode", dbUserInfo.address().zipCode());
	query.bindValue(":adState", dbUserInfo.address().state());
	query.bindValue(":caStreet", dbUserInfo.caStreet());
	query.bindValue(":caCity", dbUserInfo.caCity());
	query.bindValue(":caZipCode", dbUserInfo.caZipCode());
	query.bindValue(":caState", dbUserInfo.caState());
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return false;
}

bool AccountDb::deleteAccountInfo(const QString &key)
{
	QSqlQuery query(m_db);
	QString queryStr;

	/* Delete account info from these tables */
	const QStringList tables = QStringList() << "password_expiration_date"
	    << "user_info" << "account_info";

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	foreach (const QString &table, tables) {
		queryStr = "DELETE FROM " + table + " WHERE key = :key";
		if (!query.prepare(queryStr)) {
			logErrorNL("Cannot prepare SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
			goto fail;
		}
		query.bindValue(":key", key);
		if (query.exec()) {
			return true;
		} else {
			logErrorNL("Cannot execute SQL query: %s.",
			    query.lastError().text().toUtf8().constData());
		}
	}
fail:
	return false;
}

const Isds::DbOwnerInfo AccountDb::getOwnerInfo(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	Isds::Address address;
	Isds::BirthInfo biInfo;
	Isds::DbOwnerInfo dbOwnerInfo;
	Isds::PersonName personName;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT "
	    "dbID, dbType, ic, pnFirstName, pnMiddleName, pnLastName, "
	    "pnLastNameAtBirth, firmName, biDate, biCity, biCounty, biState, "
	    "adCity, adStreet, adNumberInStreet, adNumberInMunicipality, "
	    "adZipCode, adState, nationality, identifier, registryCode, "
	    "dbState, dbEffectiveOVM, dbOpenAddressing "
	    "FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		dbOwnerInfo.setDbID(query.value(0).toString());
		dbOwnerInfo.setDbType(Isds::strVariant2DbType(query.value(1)));
		dbOwnerInfo.setIc(query.value(2).toString());
		personName.setFirstName(query.value(3).toString());
		personName.setMiddleName(query.value(4).toString());
		personName.setLastName(query.value(5).toString());
		personName.setLastNameAtBirth(query.value(6).toString());
		dbOwnerInfo.setPersonName(personName);
		dbOwnerInfo.setFirmName(query.value(7).toString());
		biInfo.setDate(dateFromDbFormat(query.value(8).toString()));
		biInfo.setCity(query.value(9).toString());
		biInfo.setCounty(query.value(10).toString());
		biInfo.setState(query.value(11).toString());
		dbOwnerInfo.setBirthInfo(biInfo);
		address.setCity(query.value(12).toString());
		address.setStreet(query.value(13).toString());
		address.setNumberInStreet(query.value(14).toString());
		address.setNumberInMunicipality(query.value(15).toString());
		address.setZipCode(query.value(16).toString());
		address.setState(query.value(17).toString());
		dbOwnerInfo.setAddress(address);
		dbOwnerInfo.setNationality(query.value(18).toString());
		dbOwnerInfo.setIdentifier(query.value(19).toString());
		dbOwnerInfo.setRegistryCode(query.value(20).toString());
		dbOwnerInfo.setDbState(Isds::variant2DbState(query.value(21)));
		dbOwnerInfo.setDbEffectiveOVM(
		    Isds::variant2NilBool(query.value(22)));
		dbOwnerInfo.setDbOpenAddressing(
		    Isds::variant2NilBool(query.value(23)));
		return dbOwnerInfo;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
	}
fail:
	return Isds::DbOwnerInfo();
}

QString AccountDb::keyFromLogin(const QString &login)
{
	Q_ASSERT(!login.isEmpty());
	return login + QStringLiteral("___True");
}

QList<class SQLiteTbl *> AccountDb::listOfTables(void) const
{
	QList<class SQLiteTbl *> tables;
	tables.append(&accntinfTbl);
	tables.append(&userinfTbl);
	tables.append(&pwdexpdtTbl);
	return tables;
}
