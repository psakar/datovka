/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include <QRegExp>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include "src/io/db_tables.h"
#include "src/io/tag_db.h"
#include "src/log/log.h"

#define DFLT_COLOUR "ffffff"

TagDb::TagEntry::TagEntry(void)
    : id(-1),
    name(),
    colour(DFLT_COLOUR)
{
}

TagDb::TagEntry::TagEntry(int i, const QString &n, const QString &c)
    : id(i),
    name(n),
    colour(isValidColourStr(c) ? c : DFLT_COLOUR)
{
}

bool TagDb::TagEntry::isValid(void) const
{
	return (id >= 0) && !name.isEmpty() && (6 == colour.size());
}

bool TagDb::TagEntry::isValidColourStr(const QString &colourStr)
{
	QRegExp re("^[a-f0-9]{6,6}$");

	return re.exactMatch(colourStr);
}

TagDb::TagDb(const QString &connectionName)
    : SQLiteDb(connectionName)
{
}

bool TagDb::openDb(const QString &fileName)
{
	return SQLiteDb::openDb(fileName, false, listOfTables());
}

bool TagDb::insertTag(const QString &tagName, const QString &tagColor)
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT id FROM tag WHERE tag_name = :tag_name";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":tag_name", tagName);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			return false;
		}
	}

	queryStr = "INSERT INTO tag (tag_name, tag_color) "
	    "VALUES (:tag_name, :tag_color)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":tag_name", tagName);
	query.bindValue(":tag_color", tagColor);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool TagDb::updateTag(int id, const QString &tagName, const QString &tagColor)
{
	QSqlQuery query(m_db);

	QString queryStr = "UPDATE tag SET tag_name = :tag_name, "
	    "tag_color = :tag_color WHERE id = :id";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":tag_name", tagName);
	query.bindValue(":tag_color", tagColor);
	query.bindValue(":id", id);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool TagDb::deleteTag(int id)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM message_tags WHERE tag_id = :id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":id", id);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	queryStr = "DELETE FROM tag WHERE id = :id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	query.bindValue(":id", id);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}


bool TagDb::deleteAllTags(void)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM tag";
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


TagDb::TagEntry TagDb::getTagData(int id) const
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT tag_name, tag_color FROM tag WHERE id = :id";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":id", id);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return TagEntry(id, query.value(0).toString(),
		    query.value(1).toString());
	} else {
		logErrorNL(
		    "Cannot execute SQL query and/or read SQL data: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return TagEntry();
}

QList<TagDb::TagEntry> TagDb::getAllTags(void) const
{
	QSqlQuery query(m_db);
	QList<TagEntry> tagList;

	QString queryStr = "SELECT * FROM tag ORDER BY tag_name ASC";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			tagList.append(TagEntry(query.value(0).toInt(),
			    query.value(1).toString(),
			    query.value(2).toString()));
			query.next();
		}
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return tagList;
fail:
	return QList<TagEntry>();
}

QList<TagDb::TagEntry> TagDb::getMessageTags(const QString &userName,
    quint64 msgId) const
{
	QSqlQuery query(m_db);
	QList<TagEntry> tagList;

	QString queryStr = "SELECT t.id, t.tag_name, t.tag_color "
	    "FROM tag AS t "
	    "LEFT JOIN message_tags AS m "
	    "ON (t.id = m.tag_id) "
	    "WHERE m.message_id  = :msgId AND m.user_name = :userName "
	    "ORDER BY t.tag_name ASC";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":msgId", msgId);
	query.bindValue(":userName", userName);

	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			tagList.append(TagEntry(query.value(0).toInt(),
			    query.value(1).toString(),
			    query.value(2).toString()));
			query.next();
		}
		return tagList;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

fail:
	return QList<TagEntry>();
}

bool TagDb::removeAllTagsFromMsg(const QString &userName, qint64 msgId)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM message_tags WHERE "
	    "user_name = :userName AND message_id = :msgId";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":msgId", msgId);
	query.bindValue(":userName", userName);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	return true;
}

bool TagDb::assignTagToMsg(const QString &userName, int tagId, qint64 msgId)
{
	QSqlQuery query(m_db);

	QString queryStr = "SELECT id FROM message_tags WHERE "
	    "message_id = :msgId AND tag_id = :tagId AND user_name = :userName";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":msgId", msgId);
	query.bindValue(":tagId", tagId);
	query.bindValue(":userName", userName);

	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			return false;
		}
	}

	queryStr = "INSERT INTO message_tags (user_name, message_id, tag_id) "
	    "VALUES (:userName, :msgId, :tagId)";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":msgId", msgId);
	query.bindValue(":tagId", tagId);
	query.bindValue(":userName", userName);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	return true;
}

bool TagDb::removeTagFromMsg(const QString &userName, int tagId, qint64 msgId)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM message_tags WHERE "
	    "message_id = :msgId AND tag_id = :tagId AND user_name = :userName";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":msgId", msgId);
	query.bindValue(":tagId", tagId);
	query.bindValue(":userName", userName);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	return true;
}

bool TagDb::removeAllMsgTagsFromAccount(const QString &userName)
{
	QSqlQuery query(m_db);

	QString queryStr = "DELETE FROM message_tags WHERE "
	    "user_name = :userName";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}

	query.bindValue(":userName", userName);

	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		return false;
	}
	return true;
}

QList<qint64> TagDb::getMsgIdsContainSearchTagText(const QString &text) const
{
	QSqlQuery query(m_db);
	QList<qint64> msgIdList;

	/* TODO - remove duplication if (account1.sent == account2.received) */

	QString queryStr = "SELECT m.message_id FROM message_tags AS m "
	    "LEFT JOIN tag AS t ON (m.tag_id = t.id) "
	    "WHERE t.tag_name LIKE '%'||:text||'%'";

	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	query.bindValue(":text", text);
	if (query.exec() && query.isActive()) {
		query.first();
		while (query.isValid()) {
			msgIdList.append(query.value(0).toLongLong());
			query.next();
		}
		return msgIdList;
	} else {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
fail:
	return QList<qint64>();
}


QList<class SQLiteTbl *> TagDb::listOfTables(void)
{
	static QList<class SQLiteTbl *> tables;
	if (tables.isEmpty()) {
		tables.append(&tagTbl);
		tables.append(&msgtagsTbl);
	}
	return tables;
}

TagDb *globTagDbPtr = 0;
