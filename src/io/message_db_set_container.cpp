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
DbContainer::DbContainer(void)
/* ========================================================================= */
    : QMap<QString, MessageDb *>()
{
}


/* ========================================================================= */
DbContainer::~DbContainer(void)
/* ========================================================================= */
{
	QMap<QString, MessageDb *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}


/* ========================================================================= */
/*
 * Access/create+open message database related to item.
 */
MessageDb * DbContainer::accessMessageDb(const QString &primaryKey,
    const QString &locDir, bool testing, bool create)
/* ========================================================================= */
{
	MessageDb *db = NULL;
	bool open_ret;

	/* Already opened. */
	if (this->find(primaryKey) != this->end()) {
		return (*this)[primaryKey];
	}

	db = new(std::nothrow) MessageDb(dbDriverType, primaryKey);
	if (NULL == db) {
		Q_ASSERT(0);
		return NULL;
	}

	qDebug() << "XXXXXXXXXXXXXX organisation" << MessageDbSet::dbOrganisation(locDir, primaryKey, testing);

	/* TODO -- Handle file name deviations! */
	/*
	 * Test accounts have ___1 in their names, ___0 relates to standard
	 * accounts.
	 */
	QString dbFileName = constructDbFileName(primaryKey, locDir, testing);
	QFileInfo fileInfo(dbFileName);

	if (!create && !fileInfo.isFile()) {
		delete db;
		return NULL;
	} else if (!fileInfo.isFile()) {
		/* Create missing directory. */
		QDir dir = fileInfo.absoluteDir().absolutePath();
		if (!dir.exists()) {
			/* Empty file will be created automatically. */
			if (!dir.mkpath(dir.absolutePath())) {
				/* Cannot create directory. */
				delete db;
				return NULL;
			}
		}
	}

	open_ret = db->openDb(dbFileName);
	if (!open_ret) {
		delete db;
		return NULL;
	}

	this->insert(primaryKey, db);
	return db;
}


/* ========================================================================= */
/*
 * Creates a copy of the current data base into a given new
 *     directory.
 */
bool DbContainer::copyMessageDb(MessageDb *db, const QString &newLocDir)
/* ========================================================================= */
{
	if (0 == db) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDb *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != db)) {
		++it;
	}
	/* Must exist. */
	if (this->end() == it) {
		Q_ASSERT(0);
		return false;
	}

	/* Get old and new file name. */
	QString oldFileName = db->fileName();
	QFileInfo fileInfo(oldFileName);
	QString newFileName =
	    newLocDir + QDir::separator() + fileInfo.fileName();

	/* Copy database. */
	return db->copyDb(newFileName);
}


/* ========================================================================= */
/*
 * Move message database into a new directory.
 */
bool DbContainer::moveMessageDb(MessageDb *db, const QString &newLocDir)
/* ========================================================================= */
{
	if (0 == db) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDb *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != db)) {
		++it;
	}
	/* Must exist. */
	if (this->end() == it) {
		Q_ASSERT(0);
		return false;
	}

	/* Get old and new file name. */
	QString oldFileName = db->fileName();
	QFileInfo fileInfo(oldFileName);
	QString newFileName =
	    newLocDir + QDir::separator() + fileInfo.fileName();

	/* Move database. */
	return db->moveDb(newFileName);
}


/* ========================================================================= */
/*
 * Re-open a new empty database file. The old file is left
 *     untouched.
 */
bool DbContainer::reopenMessageDb(MessageDb *db, const QString &newLocDir)
/* ========================================================================= */
{
	if (0 == db) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDb *>::iterator it = this->begin();
	while ((it != this->end()) && (it.value() != db)) {
		++it;
	}
	/* Must exist. */
	if (this->end() == it) {
		Q_ASSERT(0);
		return false;
	}

	/* Get old and new file name. */
	QString oldFileName = db->fileName();
	QFileInfo fileInfo(oldFileName);
	QString newFileName =
	    newLocDir + QDir::separator() + fileInfo.fileName();

	/* Move database. */
	return db->reopenDb(newFileName);
}


/* ========================================================================= */
/*
 * Delete message db.
 */
bool DbContainer::deleteMessageDb(MessageDb *db)
/* ========================================================================= */
{
	if (0 == db) {
		Q_ASSERT(0);
		return false;
	}

	/* Find entry. */
	QMap<QString, MessageDb *>::iterator it = this->begin();
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

	/* Delete file. */
	logInfo("Deleting database file '%s'.\n",
	    fileName.toUtf8().constData());

	if (!QFile::remove(fileName)) {
		logErrorNL("Failed deleting database file '%s'.",
		    fileName.toUtf8().constData());
		return false;
	}

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
