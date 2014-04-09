

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "account_db.h"


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
	if (AccountEntry::entryNameMap.find(key) ==
	    AccountEntry::entryNameMap.end()) {
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


const QVector< QPair<QString, AccountEntry::m_dbEntryType> >
    AccountEntry::entryNames = {
	{"dbID", STRING},
	{"dbType", STRING},
	{"ic", INTEGER},
	{"pnFirstName", STRING},
	{"pnMiddleName", STRING},
	{"pnLastName", STRING},
	{"pnLastNameAtBirth", STRING},
	{"firmName", STRING},
	{"biDate", STRING},
	{"biCity", STRING},
	{"biCounty", STRING},
	{"biState", STRING},
	{"adCity", STRING},
	{"adStreet", STRING},
	{"adNumberInStreet", STRING},
	{"adNumberInMunicipality", STRING},
	{"adZipCode", STRING},
	{"adState", STRING},
	{"nationality", STRING},
	{"identifier", STRING},
	{"registryCode", STRING},
	{"dbState", INTEGER},
	{"dbEffectiveOVM", BOOL},
	{"dbOpenAddressing", BOOL}
};


const QMap<QString, QString> AccountEntry::entryNameMap = {
	{"dbID", QObject::tr("Data box ID")},
	{"dbType", QObject::tr("Data box type")},
	{"ic", QObject::tr("IČ")},
	{"pnFirstName", QObject::tr("Given name")},
	{"pnMiddleName", QObject::tr("Middle name")},
	{"pnLastName", QObject::tr("Surname")},
	{"pnLastNameAtBirth", QObject::tr("Surname at birth")},
	{"firmName", QObject::tr("Firm name")},
	{"biDate", QObject::tr("Date of birth")},
	{"biCity", QObject::tr("City of birth")},
	{"biCounty", QObject::tr("County of birth")},
	{"biState", QObject::tr("State of birth")},
	{"adCity", QObject::tr("City of residence")},
	{"adStreet", QObject::tr("Street of residence")},
	{"adNumberInStreet", QObject::tr("Number in street")},
	{"adNumberInMunicipality", QObject::tr("Number in municipality")},
	{"adZipCode", QObject::tr("Zip code")},
	{"adState", QObject::tr("State of residence")},
	{"nationality", QObject::tr("Nationality")},
	{"identifier", ""}, //
	{"registryCode", ""}, //
	{"dbState", ""}, //
	{"dbEffectiveOVM", QObject::tr("Effective OVM")},
	{"dbOpenAddressing", QObject::tr("Open addressing")}
};


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
		QString queryStr = "SELECT ";
		for (int i = 0; i < (AccountEntry::entryNames.size() - 1);
		     ++i) {
			queryStr += AccountEntry::entryNames[i].first + ", ";
		}
		queryStr += AccountEntry::entryNames.last().first + " ";
		queryStr += "FROM account_info WHERE key = '" + key + "'";
//		qDebug() << queryStr;
		if (query.prepare(queryStr) && query.exec() &&
		    query.isActive() && query.first()) {
//			qDebug() << "SQL Ok.";
			QSqlRecord rec = query.record();
			for (int i = 0; i < AccountEntry::entryNames.size();
			     ++i) {
				QVariant value = query.value(rec.indexOf(
				    AccountEntry::entryNames[i].first));
				if (!value.isNull() && value.isValid()) {
					entry.setValue(
					    AccountEntry::entryNames[i].first,
					    value);
				}
			}
		}
		query.finish();
	}

	return entry;
}
