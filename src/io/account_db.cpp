

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "account_db.h"


/* ========================================================================= */
AccountDb::AccountDb(const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent)
{
	m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
}


/* ========================================================================= */
AccountDb::~AccountDb(void)
/* ========================================================================= */
{
	m_db.close();
}


/* ========================================================================= */
/*
 * Open database file.
 */
bool AccountDb::openDb(const QString &fileName)
/* ========================================================================= */
{
//	qDebug() << "Opening db" << fileName;

	m_db.setDatabaseName(QDir::toNativeSeparators(fileName));

	m_db.setDatabaseName(fileName);

	return m_db.open();
}


/* ========================================================================= */
AccountEntry AccountDb::accountEntry(const QString &key)
/* ========================================================================= */
{
	AccountEntry entry;

	if (false == m_db.isOpen()) {
		return entry;
	}

	QSqlQuery query(m_db);
	query.prepare("SELECT _dmType FROM messages LIMIT 1");
	if (false == query.exec()) {
		QString queryStr = "SELECT "
		    "dbId "
		    "dbID, dbType, ic, pnFirstName, pnMiddleName, pnLastName, "
		    "pnLastNameAtBirth, firmName, biDate, biCity, biCounty, "
		    "biState, adCity, adStreet, adNumberInStreet, "
		    "adNumberInMunicipality, adZipCode, adState, nationality, "
		    "identifier, registryCode, dbState, dbEffectiveOVM, "
		    "dbOpenAddressing "
		    "FROM account_info WHERE key = '" + key + "'";
//		qDebug() << queryStr;
		if (query.prepare(queryStr) && query.exec() &&
		    query.isActive() && query.first()) {
//			qDebug() << "SQL Ok.";
			QSqlRecord rec = query.record();
			entry.dbID = query.value(
			    rec.indexOf("dbID")).toString();
			entry.dbType = query.value(
			    rec.indexOf("dbType")).toString();
			entry.ic = query.value(rec.indexOf("ic")).toInt();
			entry.pnFirstName = query.value(
			    rec.indexOf("pnFirstName")).toString();
			entry.pnMiddleName = query.value(
			    rec.indexOf("pnMiddleName")).toString();
			entry.pnLastName = query.value(
			    rec.indexOf("pnLastName")).toString();
			entry.pnLastNameAtBirth = query.value(
			    rec.indexOf("pnLastNameAtBirth")).toString();
			entry.firmName = query.value(
			    rec.indexOf("firmName")).toString();
			entry.biDate = query.value(
			    rec.indexOf("biDate")).toString();
			entry.biCity = query.value(
			    rec.indexOf("biCity")).toString();
			entry.biCounty = query.value(
			    rec.indexOf("biCounty")).toString();
			entry.biState = query.value(
			    rec.indexOf("biState")).toString();
			entry.adCity = query.value(
			    rec.indexOf("adCity")).toString();
			entry.adStreet = query.value(
			    rec.indexOf("adStreet")).toString();
			entry.adNumberInStreet = query.value(
			    rec.indexOf("adNumberInStreet")).toString();
			entry.adNumberInMunicipality = query.value(
			    rec.indexOf("adNumberInMunicipality")).toString();
			entry.adZipCode = query.value(
			    rec.indexOf("adZipCode")).toString();
			entry.adState = query.value(
			    rec.indexOf("adState")).toString();
			entry.nationality = query.value(
			    rec.indexOf("nationality")).toString();
			entry.identifier = query.value(
			    rec.indexOf("identifier")).toString();
			entry.registryCode = query.value(
			    rec.indexOf("registryCode")).toString();
			entry.dbState = query.value(
			    rec.indexOf("dbState")).toInt();
			entry.dbEffectiveOVM = query.value(
			    rec.indexOf("dbEffectiveOVM")).toBool();
			entry.dbOpenAddressing = query.value(
			    rec.indexOf("dbOpenAddressing")).toBool();
		}
		query.finish();
	}

	return entry;
}
