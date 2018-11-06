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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/datovka_shared/log/log.h"
#include "src/io/message_db_set_container.h"

DbContainer::DbContainer(const QString &connectionPrefix)
    : QObject(Q_NULLPTR),
    QMap<QString, MessageDbSet *>(),
    m_connectionPrefix(connectionPrefix)
{
}

DbContainer::~DbContainer(void)
{
	QMap<QString, MessageDbSet *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		MessageDbSet *dbSet = i.value();
		/* No need to call disconnect here. */
		//dbSet->disconnect(SLOT(opened(QString)),
		//    this, SIGNAL(watchOpened(QString)));
		delete dbSet;
	}
}

MessageDbSet *DbContainer::accessDbSet(const QString &locDir,
    const QString &primaryKey, bool testing,
    MessageDbSet::Organisation organisation,
    enum MessageDbSet::CreationManner manner)
{
	MessageDbSet *dbSet = Q_NULLPTR;

	/* Already opened. */
	if (this->find(primaryKey) != this->end()) {
		return (*this)[primaryKey];
	}

	dbSet = MessageDbSet::createNew(locDir, primaryKey, testing,
	    organisation, m_connectionPrefix, manner);
	if (Q_NULLPTR == dbSet) {
		return Q_NULLPTR;
	}

	connect(dbSet, SIGNAL(opened(QString)),
	    this, SLOT(watchOpened(QString)));

	this->insert(primaryKey, dbSet);
	return dbSet;
}

bool DbContainer::deleteDbSet(MessageDbSet *dbSet)
{
	if (Q_NULLPTR == dbSet) {
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

	dbSet->disconnect(SLOT(opened(QString)),
	    this, SIGNAL(watchOpened(QString)));

	/* Close database. */
	delete dbSet;

	return true;
}

void DbContainer::watchOpened(const QString &primaryKey)
{
	emit opened(primaryKey);
}

const QString DbContainer::dbDriverType("QSQLITE");
