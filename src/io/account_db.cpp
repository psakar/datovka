

#include <QDebug>
#include <QDir>
#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QVariant>

#include "account_db.h"
#include "src/io/db_tables.h"


/* ========================================================================= */
AccountEntry::AccountEntry(void)
/* ========================================================================= */
    : QMap<QString, QVariant>()
{
}


/* ========================================================================= */
AccountEntry::~AccountEntry(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Set value.
 */
bool AccountEntry::setValue(const QString &key, const QVariant &value)
/* ========================================================================= */
{
	/* Don't insert if key is not known. */
	if (accntinfTbl.attrProps.find(key) ==
	    accntinfTbl.attrProps.end()) {
		return false;
	}

	this->insert(key, value);

	return true;
}


/* ========================================================================= */
/*
 * Check whether value is stored.
 */
bool AccountEntry::hasValue(const QString &key) const
/* ========================================================================= */
{
	return this->find(key) != this->end();
}


/* ========================================================================= */
/*
 * Return stored value.
 */
const QVariant AccountEntry::value(const QString &key,
    const QVariant &defaultValue) const
/* ========================================================================= */
{
	return m_parentType::value(key, defaultValue);
}


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
	bool ret;
	//qDebug() << "Opening account db" << fileName;

	m_db.setDatabaseName(QDir::toNativeSeparators(fileName));

	ret = m_db.open();

	if (ret) {
		/* Check whether database contains account table. */
		if (!accntinfTbl.existsInDb(m_db)) {
			//qDebug() << accntinfTbl.tabName << "does not exist.";
			return accntinfTbl.createEmpty(m_db);
		}
	}

	return ret;
}


/* ========================================================================= */
AccountEntry AccountDb::accountEntry(const QString &key) const
/* ========================================================================= */
{
	AccountEntry entry;

	if (false == m_db.isOpen()) {
		return entry;
	}

	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (accntinfTbl.knownAttrs.size() - 1); ++i) {
		queryStr += accntinfTbl.knownAttrs[i].first + ", ";
	}
	queryStr += accntinfTbl.knownAttrs.last().first + " ";
	queryStr += "FROM account_info WHERE key = :key";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		return entry;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() && query.first()) {
		//qDebug() << "SQL Ok.";
		QSqlRecord rec = query.record();
		for (int i = 0; i < accntinfTbl.knownAttrs.size(); ++i) {
			QVariant value = query.value(rec.indexOf(
			    accntinfTbl.knownAttrs[i].first));
			if (!value.isNull() && value.isValid()) {
				entry.setValue(
				    accntinfTbl.knownAttrs[i].first,
				    value);
			}
		}
	}
	query.finish();

	return entry;
}


/* ========================================================================= */
/*
 * Return data box identifier.
 */
const QString AccountDb::dbId(const QString &key,
    const QString defaultValue) const
/* ========================================================================= */
{
	if (false == m_db.isOpen()) {
		return defaultValue;
	}

	QSqlQuery query(m_db);
	QString queryStr = "SELECT dbID FROM account_info WHERE key = :key";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		return defaultValue;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() && query.first()) {
		return query.value(0).toString();
	} else {
		return defaultValue;
	}
}


/* ========================================================================= */
/*
 * Get pwd expiration info from password_expiration_date table
 */
const QString AccountDb::getPwdExpirFromDb(const QString &key) const
/* ========================================================================= */
{
	QString ret = tr("unknown or without expiration");

	if (false == m_db.isOpen()) {
		return ret;
	}

	QSqlQuery query(m_db);
	QString queryStr =
	    "SELECT expDate FROM password_expiration_date WHERE key = :key";
	if (!query.prepare(queryStr)) {
		return ret;
	}
	query.bindValue(":key", key);
	if (query.exec() && query.isActive() && query.first()) {
		if (query.value(0).toString().isEmpty()) {
			return ret;
		} else {
			return query.value(0).toString();
		}
	} else {
		return ret;
	}
}


/* ========================================================================= */
/*
 * Set pwd expiration to password_expiration_date table
 */
bool AccountDb::setPwdExpirIntoDb(const QString &key, QString &date)
    const
/* ========================================================================= */
{
	if (false == m_db.isOpen()) {
		return false;
	}

	bool update = true;
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT count(*) FROM password_expiration_date WHERE "
	    "key = :key";
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":key", key);

	if (!query.exec()) {
	 	return false;
	} else {
		query.first();
		if (query.isValid()) {
			if (query.value(0).toInt() == 0) {
				update = false;
			}
		}
	}

	if (update) {
		queryStr = "UPDATE password_expiration_date SET "
		"expDate = :expDate WHERE key = :key";
	} else {
		queryStr = "INSERT INTO password_expiration_date ("
		    "key, expDate) VALUES (:key, :expDate)";
	}

	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":key", key);
	query.bindValue(":expDate", date);
	if (!query.exec()) {
		return false;
	}
	return true;
}


