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

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h"

TaskDownloadMessage::TaskDownloadMessage(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect,
    qint64 dmId, const QDateTime &dTime)
    : m_donloadSucceeded(false),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect),
    m_mId(dmId, dTime)
{
	Q_ASSERT(0 != dbSet);
	Q_ASSERT(0 <= dmId);
}

void TaskDownloadMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	if ((MSG_RECEIVED != m_msgDirect) && (MSG_SENT != m_msgDirect)) {
		Q_ASSERT(0);
		return;
	}

	if (0 > m_mId.dmId) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	QString errMsg;
	qdatovka_error res = Q_SUCCESS;

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message '%d' for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent", m_mId.dmId,
	    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	res = Task::downloadMessage(m_userName, m_mId, true, m_msgDirect,
	    *m_dbSet, errMsg, "DownloadMessage");

	m_donloadSucceeded = Q_SUCCESS == res;

	if (Q_SUCCESS == res) {
		logDebugLv1NL("Done downloading message '%d' for account '%s'.",
		    m_mId.dmId,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());

		/* Only on successful download. */
		emit globMsgProcEmitter.downloadSuccess(m_userName, m_mId.dmId);
	} else {
		logErrorNL("Downloading message '%d' for account '%s' failed.",
		    m_mId.dmId,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());

		emit globMsgProcEmitter.downloadFail(m_userName, m_mId.dmId,
		    errMsg);
	}

	emit globMsgProcEmitter.progressChange("Idle", 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}
