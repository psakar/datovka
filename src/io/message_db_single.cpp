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

#include "message_db_single.h"

MessageDbSingle::MessageDbSingle(void)
    : m_db(NULL)
{

}

MessageDbSingle::~MessageDbSingle(void)
{
	delete m_db;
}

MessageDbSingle *MessageDbSingle::createNew(const QString &filePath,
    const QString &connectionPrefix)
{
	MessageDb *db = NULL;
	MessageDbSingle *dbSingle = NULL;

	if (filePath.isEmpty()) {
		return NULL;
	}

	if (!QFileInfo::exists(filePath)) {
		return NULL;
	}

	QString connectionName(connectionPrefix + "_SINGLE_FILE");

	db = new(std::nothrow) MessageDb(dbDriverType, connectionName);
	if (NULL == db) {
		Q_ASSERT(0);
		return NULL;
	}

	if (!db->openDb(filePath)) {
		delete db;
		return NULL;
	}

	dbSingle = new(std::nothrow) MessageDbSingle();
	if (NULL == dbSingle) {
		delete db;
		return NULL;
	}

	/* Assign database. */
	dbSingle->m_db = db;

	return dbSingle;
}

const QString MessageDbSingle::dbDriverType("QSQLITE");

bool MessageDbSingle::dbDriverSupport(void)
{
	QStringList driversList = QSqlDatabase::drivers();

	return driversList.contains(dbDriverType, Qt::CaseSensitive);
}

/* Methods delegated from MessageDb and made public via this class. */

QList<MessageDb::MsgId> MessageDbSingle::getAllMessageIDsFromDB(void) const
{
	return m_db->getAllMessageIDsFromDB();
}

bool MessageDbSingle::isRelevantMsgForImport(qint64 msgId,
    const QString databoxId) const
{
	return m_db->isRelevantMsgForImport(msgId, databoxId);
}
