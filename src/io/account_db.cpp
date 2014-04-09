

#include <QDir>
#include <QSqlQuery>

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
		QString queryStr;

		query.clear();
		query.prepare("ALTER TABLE messages ADD COLUMN _dmType TEXT");
		query.exec();
	}

	return entry;
}
