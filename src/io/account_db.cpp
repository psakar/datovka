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

	queryStr = "SELECT ";
	for (int i = 0; i < (accntinfTbl.knownAttrs.size() - 1); ++i) {
		queryStr += accntinfTbl.knownAttrs[i].first + ", ";
	}
	queryStr += accntinfTbl.knownAttrs.last().first + " ";
	queryStr += "FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return DbEntry();
}

DbEntry AccountDb::userEntry(const QString &key) const
{
	QSqlQuery query(m_db);
	QString queryStr;
	DbEntry uEntry;

	queryStr = "SELECT ";
	for (int i = 0; i < (userinfTbl.knownAttrs.size() - 1); ++i) {
		queryStr += userinfTbl.knownAttrs[i].first + ", ";
	}
	queryStr += userinfTbl.knownAttrs.last().first + " ";
	queryStr += "FROM user_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return DbEntry();
}

const QString AccountDb::dbId(const QString &key,
    const QString &defaultValue) const
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT dbID FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toString();
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return defaultValue;
}

const QString AccountDb::senderNameGuess(const QString &key,
    const QString &defaultValue) const
{
	QSqlQuery query(m_db);
	QString name;

	QString queryStr = "SELECT firmName, pnFirstName, pnMiddleName, "
	    "pnLastName FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return defaultValue;
}

const QString AccountDb::getPwdExpirFromDb(const QString &key) const
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT expDate FROM password_expiration_date "
	    "WHERE key = :key";

	if (!query.prepare(queryStr)) {
		goto fail;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive()) {
		if (query.first() && query.isValid() &&
		    !query.value(0).toString().isEmpty()) {
			return query.value(0).toString();
		}
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
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

	QString queryStr = "INSERT OR REPLACE INTO password_expiration_date "
	    "(key, expDate) VALUES (:key, :expDate)";

	if (!query.prepare(queryStr)) {
		goto fail;
	}
	query.bindValue(":key", key);
	query.bindValue(":expDate", date);
	if (query.exec()) {
		return true;
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
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

	QString queryStr = "INSERT OR REPLACE INTO account_info "
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
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

	QString queryStr = "INSERT OR REPLACE INTO user_info "
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return false;
}

bool AccountDb::deleteAccountInfo(const QString &key)
{
	QSqlQuery query(m_db);
	QString queryStr;

	/* Delete account info from these tables */
	const QStringList tables = QStringList() << "password_expiration_date"
	    << "user_info" << "account_info";

	foreach (const QString &table, tables) {
		queryStr = "DELETE FROM " + table + " WHERE key = :key";
		if (!query.prepare(queryStr)) {
			goto fail;
		}
		query.bindValue(":key", key);
		if (!query.exec()) {
			goto fail;
		}
	}
	return true;
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return false;
}

QStringList AccountDb::getUserDataboxInfo(const QString &key) const
{
	QSqlQuery query(m_db);
	QStringList dataList;

	QString queryStr = "SELECT dbType, dbEffectiveOVM, dbOpenAddressing "
	    "FROM account_info WHERE key = :key";

	if (!query.prepare(queryStr)) {
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
	}
fail:
	logErrorNL("Cannot prepare or execute SQL query: %s.",
	    query.lastError().text().toUtf8().constData());
	return QStringList();
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
