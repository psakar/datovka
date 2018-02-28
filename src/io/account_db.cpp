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

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "src/io/account_db.h"
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
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return aEntry;

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
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return uEntry;

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
		goto fail;
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

	queryStr = "SELECT firmName FROM account_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		name = query.value(0).toString();
		if (!(name.isNull() || name.isEmpty())) {
			return name;
		}
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
		/* goto fail; */ /* Try a different query. */
	}

	queryStr = "SELECT pnFirstName, pnMiddleName, pnLastName "
	    "FROM account_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		name = query.value(0).toString();
		if (!query.value(1).toString().isEmpty()) {
			name += " " + query.value(1).toString();
		}
		name += " " + query.value(2).toString();
		return name;
	} else {
		logErrorNL("Cannot execute SQL query and/or read SQL data: "
		    "%s.", query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return defaultValue;
}

const QString AccountDb::getPwdExpirFromDb(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT expDate "
	    "FROM password_expiration_date WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		if (query.first() && query.isValid() &&
		    !query.value(0).toString().isEmpty()) {
			return query.value(0).toString();
		} else {
			return QString();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QString();
}

int AccountDb::pwdExpiresInDays(const QString &key, int days) const
{
	const QString dbDateTimeString(getPwdExpirFromDb(key));
	if (dbDateTimeString.isNull()) {
		return -1;
	}

	const QDateTime dbDateTime = QDateTime::fromString(
	    dbDateTimeString, "yyyy-MM-dd HH:mm:ss.000000");
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

bool AccountDb::setPwdExpirIntoDb(const QString &key, const QString &date)
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool update = true;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT count(*) "
	    "FROM password_expiration_date WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() != 0) {
				update = true;
			} else {
				update = false;
			}
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (update) {
		queryStr = "UPDATE password_expiration_date "
		    "SET expDate = :expDate WHERE key = :key";
	} else {
		queryStr = "INSERT INTO password_expiration_date "
		    "(key, expDate) VALUES (:key, :expDate)";
	}
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":expDate", date);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}

bool AccountDb::insertAccountIntoDb(const QString &key, const QString &dbID,
    const QString &dbType, int ic, const QString &pnFirstName,
    const QString &pnMiddleName, const QString &pnLastName,
    const QString &pnLastNameAtBirth, const QString &firmName,
    const QString &biDate, const QString &biCity, const QString &biCounty,
    const QString &biState, const QString &adCity, const QString &adStreet,
    const QString &adNumberInStreet, const QString &adNumberInMunicipality,
    const QString &adZipCode,const QString &adState,const QString &nationality,
    const QString &identifier, const QString &registryCode,
    int dbState, bool dbEffectiveOVM, bool dbOpenAddressing)
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool update = true;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT count(*) "
	    "FROM account_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() != 0) {
				update = true;
			} else {
				update = false;
			}
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (update) {
		queryStr = "UPDATE account_info "
		    "SET dbID = :dbID, dbType = :dbType, ic = :ic, "
		    "pnFirstName = :pnFirstName, pnMiddleName = :pnMiddleName, "
		    "pnLastName = :pnLastName, "
		    "pnLastNameAtBirth = :pnLastNameAtBirth, "
		    "firmName = :firmName, biDate = :biDate, "
		    "biCity = :biCity, biCounty = :biCounty, "
		    "biState = :biState, "
		    "adCity = :adCity, adStreet = :adStreet, "
		    "adNumberInStreet = :adNumberInStreet, "
		    "adNumberInMunicipality = :adNumberInMunicipality, "
		    "adZipCode = :adZipCode, "
		    "adState = :adState, nationality = :nationality, "
		    "identifier = :identifier, registryCode = :registryCode, "
		    "dbState = :dbState, dbEffectiveOVM = :dbEffectiveOVM, "
		    "dbOpenAddressing = :dbOpenAddressing WHERE key = :key";
	} else {
		queryStr = "INSERT INTO account_info ("
		    "key, dbID, dbType, ic, pnFirstName, pnMiddleName, "
		    "pnLastName, pnLastNameAtBirth, firmName, biDate, biCity, "
		    "biCounty, biState, adCity, adStreet, adNumberInStreet, "
		    "adNumberInMunicipality, adZipCode, adState, nationality, "
		    "identifier, registryCode, dbState, dbEffectiveOVM, "
		    "dbOpenAddressing"
		    ") VALUES ("
		    ":key, :dbID, :dbType, :ic, :pnFirstName, :pnMiddleName, "
		    ":pnLastName, :pnLastNameAtBirth, :firmName, :biDate, :biCity, "
		    ":biCounty, :biState, :adCity, :adStreet, :adNumberInStreet, "
		    ":adNumberInMunicipality, :adZipCode, :adState, :nationality, "
		    ":identifier, :registryCode, :dbState, :dbEffectiveOVM, "
		    ":dbOpenAddressing"
		    ")";
	}

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":dbID", dbID);
	query.bindValue(":dbType", dbType);
	query.bindValue(":ic", ic);
	query.bindValue(":pnFirstName", pnFirstName);
	query.bindValue(":pnMiddleName", pnMiddleName);
	query.bindValue(":pnLastName", pnLastName);
	query.bindValue(":pnLastNameAtBirth", pnLastNameAtBirth);
	query.bindValue(":firmName", firmName);
	query.bindValue(":biDate", biDate);
	query.bindValue(":biCity", biCity);
	query.bindValue(":biCounty", biCounty);
	query.bindValue(":biState", biState);
	query.bindValue(":adCity", adCity);
	query.bindValue(":adStreet", adStreet);
	query.bindValue(":adNumberInStreet", adNumberInStreet);
	query.bindValue(":adNumberInMunicipality", adNumberInMunicipality);
	query.bindValue(":adZipCode", adZipCode);
	query.bindValue(":adState", adState);
	query.bindValue(":nationality", nationality);
	query.bindValue(":identifier", identifier);
	query.bindValue(":registryCode", registryCode);
	query.bindValue(":dbState", dbState);
	query.bindValue(":dbEffectiveOVM", dbEffectiveOVM);
	query.bindValue(":dbOpenAddressing", dbOpenAddressing);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}

bool AccountDb::insertUserIntoDb(const QString &key,
    const QString &userType, int userPrivils,
    const QString &pnFirstName, const QString &pnMiddleName,
    const QString &pnLastName, const QString &pnLastNameAtBirth,
    const QString &adCity, const QString &adStreet,
    const QString &adNumberInStreet, const QString &adNumberInMunicipality,
    const QString &adZipCode,const QString &adState,
    const QString &biDate,
    int ic, const QString &firmName, const QString &caStreet,
    const QString &caCity, const QString &caZipCode, const QString &caState)
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool update = true;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT count(*) "
	    "FROM user_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() != 0) {
				update = true;
			} else {
				update = false;
			}
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (update) {
		queryStr = "UPDATE user_info "
		    "SET userType = :userType, userPrivils = :userPrivils, "
		    "pnFirstName = :pnFirstName, pnMiddleName = :pnMiddleName, "
		    "pnLastName = :pnLastName, "
		    "pnLastNameAtBirth = :pnLastNameAtBirth, "
		    "adCity = :adCity, adStreet = :adStreet, "
		    "adNumberInStreet = :adNumberInStreet, "
		    "adNumberInMunicipality = :adNumberInMunicipality, "
		    "adZipCode = :adZipCode, adState = :adState, "
		    "biDate = :biDate, ic = :ic, "
		    "firmName = :firmName, caStreet = :caStreet, "
		    "caCity = :caCity, caZipCode = :caZipCode, "
		    "caState = :caState WHERE key = :key";
	} else {
		queryStr = "INSERT INTO user_info ("
		    "key, userType, userPrivils, pnFirstName, pnMiddleName, "
		    "pnLastName, pnLastNameAtBirth, adCity, adStreet, "
		    "adNumberInStreet, adNumberInMunicipality, adZipCode, "
		    "adState, biDate, ic, firmName, caStreet, caCity, "
		    "caZipCode, caState"
		    ") VALUES ("
		    ":key, :userType, :userPrivils, :pnFirstName, "
		    ":pnMiddleName, :pnLastName, :pnLastNameAtBirth, :adCity, "
		    ":adStreet, :adNumberInStreet, :adNumberInMunicipality, "
		    ":adZipCode, :adState, :biDate, :ic, :firmName, :caStreet, "
		    ":caCity, :caZipCode, :caState)";
	}

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":key", key);
	query.bindValue(":userType", userType);
	query.bindValue(":userPrivils", userPrivils);
	query.bindValue(":ic", ic);
	query.bindValue(":pnFirstName", pnFirstName);
	query.bindValue(":pnMiddleName", pnMiddleName);
	query.bindValue(":pnLastName", pnLastName);
	query.bindValue(":pnLastNameAtBirth", pnLastNameAtBirth);
	query.bindValue(":firmName", firmName);
	query.bindValue(":biDate", biDate);
	query.bindValue(":adCity", adCity);
	query.bindValue(":adStreet", adStreet);
	query.bindValue(":adNumberInStreet", adNumberInStreet);
	query.bindValue(":adNumberInMunicipality", adNumberInMunicipality);
	query.bindValue(":adZipCode", adZipCode);
	query.bindValue(":adState", adState);
	query.bindValue(":caStreet", caStreet);
	query.bindValue(":caCity", caCity);
	query.bindValue(":caZipCode", caZipCode);
	query.bindValue(":caState", caState);
	if (query.exec()) {
		return true;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return false;
}

bool AccountDb::deleteAccountInfo(const QString &key)
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "DELETE FROM password_expiration_date WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	queryStr = "DELETE FROM account_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}

QList<QString> AccountDb::getUserDataboxInfo(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	QList<QString> dataList;

	if (!m_db.isOpen()) {
		logErrorNL("%s", "Account database seems not to be open.");
		goto fail;
	}

	queryStr = "SELECT dbType, dbEffectiveOVM, dbOpenAddressing "
	    "FROM account_info WHERE key = :key";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		/* dbType */
		dataList.append(query.value(0).toString());
		/* dbEffectiveOVM */
		dataList.append(query.value(1).toString());
		/* dbOpenAddressing */
		dataList.append(query.value(2).toString());

		return dataList;
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return dataList;

fail:
	return QList<QString>();
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
