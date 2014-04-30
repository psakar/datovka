

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "account_db.h"


namespace AccntinfTbl {
	const QString tabName("account_info");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"dbID", DB_TEXT},
	{"dbType", DB_TEXT},
	{"ic", DB_INTEGER},
	{"pnFirstName", DB_TEXT},
	{"pnMiddleName", DB_TEXT},
	{"pnLastName", DB_TEXT},
	{"pnLastNameAtBirth", DB_TEXT},
	{"firmName", DB_TEXT},
	{"biDate", DB_DATETIME},
	{"biCity", DB_TEXT},
	{"biCounty", DB_TEXT},
	{"biState", DB_TEXT},
	{"adCity", DB_TEXT},
	{"adStreet", DB_TEXT},
	{"adNumberInStreet", DB_TEXT},
	{"adNumberInMunicipality", DB_TEXT},
	{"adZipCode", DB_TEXT},
	{"adState", DB_TEXT},
	{"nationality", DB_TEXT},
	{"identifier", DB_TEXT},
	{"registryCode", DB_TEXT},
	{"dbState", DB_INTEGER},
	{"dbEffectiveOVM", DB_BOOLEAN},
	{"dbOpenAddressing", DB_BOOLEAN}
	};

	const QMap<QString, AttrProp> attrProps = {
	{"dbID",                   {DB_TEXT, QObject::tr("Data box ID")}},
	{"dbType",                 {DB_TEXT, QObject::tr("Data box type")}},
	{"ic",                     {DB_INTEGER, QObject::tr("IČ")}},
	{"pnFirstName",            {DB_TEXT, QObject::tr("Given name")}},
	{"pnMiddleName",           {DB_TEXT, QObject::tr("Middle name")}},
	{"pnLastName",             {DB_TEXT, QObject::tr("Surname")}},
	{"pnLastNameAtBirth",      {DB_TEXT, QObject::tr("Surname at birth")}},
	{"firmName",               {DB_TEXT, QObject::tr("Firm name")}},
	{"biDate",                 {DB_DATETIME, QObject::tr("Date of birth")}},
	{"biCity",                 {DB_TEXT, QObject::tr("City of birth")}},
	{"biCounty",               {DB_TEXT, QObject::tr("County of birth")}},
	{"biState",                {DB_TEXT, QObject::tr("State of birth")}},
	{"adCity",                 {DB_TEXT, QObject::tr("City of residence")}},
	{"adStreet",               {DB_TEXT, QObject::tr("Street of residence")}},
	{"adNumberInStreet",       {DB_TEXT, QObject::tr("Number in street")}},
	{"adNumberInMunicipality", {DB_TEXT, QObject::tr("Number in municipality")}},
	{"adZipCode",              {DB_TEXT, QObject::tr("Zip code")}},
	{"adState",                {DB_TEXT, QObject::tr("State of residence")}},
	{"nationality",            {DB_TEXT, QObject::tr("Nationality")}},
	{"identifier",             {DB_TEXT, ""}}, //
	{"registryCode",           {DB_TEXT, ""}}, //
	{"dbState",                {DB_INTEGER, ""}}, //
	{"dbEffectiveOVM",         {DB_BOOLEAN, QObject::tr("Effective OVM")}},
	{"dbOpenAddressing",       {DB_BOOLEAN, QObject::tr("Open addressing")}}
	};
} /* namespace AccntinfTbl */
const Tbl accntinfTbl(AccntinfTbl::tabName, AccntinfTbl::knownAttrs,
    AccntinfTbl::attrProps);


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
	queryStr += "FROM account_info WHERE key = '" + key + "'";
//	qDebug() << queryStr;
	if (query.prepare(queryStr) && query.exec() && query.isActive() &&
	    query.first()) {
//		qDebug() << "SQL Ok.";
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
	QString queryStr = "SELECT dbID FROM account_info WHERE key = '" +
	    key + "'";
	if (query.prepare(queryStr) && query.exec() && query.isActive() &&
	    query.first()) {
		return query.value(0).toString();
	} else {
		return defaultValue;
	}
}
