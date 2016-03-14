/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "src/io/sqlite/table.h"
#include "src/log/log.h"

SQLiteTbl::SQLiteTbl(const QString &name,
    const QVector< QPair<QString, enum EntryType> > &attrs,
    const QMap<QString, SQLiteTbl::AttrProp> &props,
    const QMap<QString, QString> &colCons,
    const QString &tblCons)
    : tabName(name),
    knownAttrs(attrs),
    attrPropsSources(props),
    attrProps(props),
    colConstraints(colCons),
    tblConstraint(tblCons)
{
}

bool SQLiteTbl::existsInDb(const QSqlDatabase &db) const
{
	if (!db.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	if (!db.isOpen()) {
		Q_ASSERT(0);
		logErrorNL("%s", "Database seems not to be open.");
		return false;
	}

	QSqlQuery query(db);
	QString queryStr = "SELECT "
	    "name"
	    " FROM sqlite_master WHERE "
	    "type = 'table'"
	    " and "
	    "name = :tabName";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":tabName", tabName);
	if (query.exec() && query.isActive()) {
		return query.first() && query.isValid();
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return false;
}

bool SQLiteTbl::createEmpty(QSqlDatabase &db) const
{
	if (!db.isValid()) {
		Q_ASSERT(0);
		return false;
	}

	if (!db.isOpen()) {
		Q_ASSERT(0);
		logErrorNL("%s", "Database seems not to be open.");
		return false;
	}

	logInfoNL("Creating non-existent new table '%s'.",
	    tabName.toUtf8().constData());

	QSqlQuery query(db);
	QString queryStr = "CREATE TABLE IF NOT EXISTS " + tabName + " (\n";
	for (int i = 0; i < knownAttrs.size(); ++i) {
		queryStr += "        " + knownAttrs[i].first + " " +
		    entryType2SQLiteDataType(knownAttrs[i].second);
		if (colConstraints.end() !=
		    colConstraints.find(knownAttrs[i].first)) {
			queryStr += " " +
			    colConstraints.value(knownAttrs[i].first);
		}
		if ((knownAttrs.size() - 1) != i) {
			queryStr += ",\n";
		}
	}
	queryStr += tblConstraint;
	queryStr += "\n)";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

void SQLiteTbl::reloadLocalisedDescription(void)
{
	attrProps.clear();

	QMap<QString, AttrProp>::const_iterator it = attrPropsSources.begin();

	AttrProp prop;
	for (; it != attrPropsSources.end(); ++it) {
		prop.type = it.value().type;
		prop.desc = QObject::tr(it.value().desc.toUtf8().constData());
		attrProps.insert(it.key(), prop);
	}
}

const QString &SQLiteTbl::entryType2SQLiteDataType(enum EntryType entryType)
{
	static const QString integer("INTEGER");
	static const QString text("TEXT");
	static const QString boolean("BOOLEAN");
	static const QString datetime("DATETIME");
	static const QString date("DATE");
	static const QString invalid;

	switch (entryType) {
	case DB_INTEGER:
	case DB_INT_PROCESSING_STATE:
		return integer;
		break;
	case DB_TEXT:
		return text;
		break;
	case DB_BOOLEAN:
	case DB_BOOL_READ_LOCALLY:
	case DB_BOOL_ATTACHMENT_DOWNLOADED:
		return boolean;
		break;
	case DB_DATETIME:
		return datetime;
		break;
	case DB_DATE:
		return date;
		break;
	default:
		Q_ASSERT(0);
		return invalid;
		break;
	}
}

const QMap<QString, QString> SQLiteTbl::emptyColConstraints;

const QString SQLiteTbl::emptyTblConstraint;
