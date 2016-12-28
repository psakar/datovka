/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <QDebug>
#include <QFileInfo>
#include <QDir>

#include "src/io/tag_db_container.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"

/* ========================================================================= */
TagDbContainer::TagDbContainer(const QString &connectionName)
/* ========================================================================= */
    : QMap<QString, TagDb *>(),
    m_connectionName(connectionName)
{
}


/* ========================================================================= */
TagDbContainer::~TagDbContainer(void)
/* ========================================================================= */
{
	QMap<QString, TagDb *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}

/* ========================================================================= */
/*
 * Access/create+open message database related to item.
 */
TagDb *TagDbContainer::accessTagDb(const QString &key)
/* ========================================================================= */
{
	TagDb *db = NULL;
	bool open_ret;

	QString dbTagName = constructDbTagName(key);


	/* Already opened. */
	if (this->find(dbTagName) != this->end()) {
		return (*this)[dbTagName];
	}

	db = new(std::nothrow) TagDb(dbTagName);
	if (NULL == db) {
		Q_ASSERT(0);
		return NULL;
	}

	open_ret = db->openDb(dbTagName);
	if (!open_ret) {
		delete db;
		return NULL;
	}

	this->insert(dbTagName, db);

	return db;
}


/* ========================================================================= */
/*
 * Delete message db.
 */
bool TagDbContainer::deleteDb(TagDb *db)
/* ========================================================================= */
{
	if (0 == db) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, TagDb *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != db)) {
		++it;
	}
	/* Must exist. */
	if (this->end() == it) {
		Q_ASSERT(0);
		return false;
	}

	/* Remove from container. */
	this->erase(it);

	/* Get file name. */
	QString fileName = db->fileName();

	/* Close database. */
	delete db;

	if (fileName == TagDb::memoryLocation) {
		return true;
	}

	/* Delete file. */
	if (!QFile::remove(fileName)) {
		return false;
	}

	return true;
}


/* ========================================================================= */
/*
 * Creates the database name from supplied information.
 */
QString TagDbContainer::constructDbTagName(const QString &primaryKey)
/* ========================================================================= */
{
	/* get current db file location */
	AcntSettings &itemSettings(AccountModel::globAccounts[primaryKey]);
	QString dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		dbDir = globPref.confDir();
	}

	return dbDir + QDir::separator() + primaryKey + "-tag.db";
}


TagDbContainer * globWebDatovkaTagDbPtr = 0;
