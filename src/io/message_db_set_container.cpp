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


#include <QFileInfo>

#include "message_db_set_container.h"
#include "src/log/log.h"


/* ========================================================================= */
DbContainer::DbContainer(const QString &connectionPrefix)
/* ========================================================================= */
    : QMap<QString, MessageDbSet *>(),
    m_connectionPrefix(connectionPrefix)
{
}


/* ========================================================================= */
DbContainer::~DbContainer(void)
/* ========================================================================= */
{
	QMap<QString, MessageDbSet *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}


/* ========================================================================= */
/*
 * Access/create+open message database related to item.
 */
MessageDbSet *DbContainer::accessDbSet(const QString &locDir,
    const QString &primaryKey, bool testing,
    MessageDbSet::Organisation organisation, bool create)
/* ========================================================================= */
{
	MessageDbSet *dbSet = NULL;

	/* Already opened. */
	if (this->find(primaryKey) != this->end()) {
		return (*this)[primaryKey];
	}

	dbSet = MessageDbSet::createNew(locDir, primaryKey, testing,
	    organisation, m_connectionPrefix, !create);
	if (NULL == dbSet) {
		return NULL;
	}

	this->insert(primaryKey, dbSet);
	return dbSet;
}


/* ========================================================================= */
/*
 * Delete message db.
 */
bool DbContainer::deleteDbSet(MessageDbSet *dbSet)
/* ========================================================================= */
{
	if (0 == dbSet) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDbSet *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != dbSet)) {
		++it;
	}
	/* Must exist. */
	if (this->end() == it) {
		Q_ASSERT(0);
		return false;
	}

	/* Remove from container. */
	this->erase(it);

	/* Delete files. */
	dbSet->deleteLocation();

	/* Close database. */
	delete dbSet;

	return true;
}


const QString DbContainer::dbDriverType("QSQLITE");


/* ========================================================================= */
/*
 * Creates the database name from supplied information.
 */
QString DbContainer::constructDbFileName(const QString &primaryKey,
    const QString &locDir, bool testing)
/* ========================================================================= */
{
	return locDir + QDir::separator() +
	    primaryKey + "___" + (testing ? "1" : "0") + ".db";
}


DbContainer *globMessageDbsPtr;
