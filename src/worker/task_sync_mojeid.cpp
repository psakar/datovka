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

#include <QThread>

#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_sync_mojeid.h"
#include "src/web/json.h"

TaskSyncAccount::TaskSyncAccount(int id)
    : m_success(false),
    m_isdsError(),
    m_id(id)
{
}

void TaskSyncAccount::run(void)
{
	logDebugLv0NL("Starting account sync in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = syncAccount(m_id, m_isdsError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Account sync task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskSyncAccount::syncAccount(int id, QString &error)
{
	emit globMsgProcEmitter.progressChange(PL_SYNC_ACCOUNT, 10);

	JsonLayer lJsonlayer;
	return lJsonlayer.syncAccount(id, error);
}
