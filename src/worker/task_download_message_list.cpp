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
#include "src/worker/task_download_message_list.h"

TaskDownloadMessageList::TaskDownloadMessageList(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect)
    : m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect)
{
	Q_ASSERT(0 != m_dbSet);
}

void TaskDownloadMessageList::run(void)
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

	logDebugLv0NL("Starting download message list task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	int rt = 0; /*!< Received total. */
	int rn = 0; /*!< Received new. */
	int st = 0; /*!< Sent total. */
	int sn = 0; /*!< Sent new. */
        QString errMsg;
	QStringList newMsgIdList;
	qdatovka_error res = Q_SUCCESS;

	/* dmStatusFilter
	 *
	 * MESSAGESTATE_SENT |
	 * MESSAGESTATE_STAMPED |
	 * MESSAGESTATE_DELIVERED |
	 * MESSAGESTATE_RECEIVED |
	 * MESSAGESTATE_READ |
	 * MESSAGESTATE_REMOVED |
	 * MESSAGESTATE_IN_SAFE |
	 * MESSAGESTATE_ANY
	 */

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message list for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent",
	    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	unsigned long dmLimit = MESSAGE_LIST_LIMIT;

	if (MSG_RECEIVED == m_msgDirect) {
		res = Task::downloadMessageList(m_userName, MSG_RECEIVED,
		    *m_dbSet, errMsg, "GetListOfReceivedMessages",
		    rt, rn, newMsgIdList, &dmLimit, MESSAGESTATE_ANY);
	} else {
		res = Task::downloadMessageList(m_userName, MSG_SENT,
		    *m_dbSet, errMsg, "GetListOfSentMessages",
		    st, sn, newMsgIdList, &dmLimit, MESSAGESTATE_ANY);
	}

	/*
	 * -1 means list of received messages.
	 * -2 means list of sent messages.
	 */
	emit globMsgProcEmitter.downloadSuccess(m_userName,
	    (MSG_RECEIVED == m_msgDirect) ? -1 : -2);
	emit globMsgProcEmitter.downloadListSummary(true, rt, rn , st, sn);

	if (Q_SUCCESS == res) {
		logDebugLv1NL("Done downloading message list for account '%s'.",
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	} else {
		logErrorNL("Downloading message list for account '%s' failed.",
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());

		emit globMsgProcEmitter.downloadFail(m_userName,
		    (MSG_RECEIVED == m_msgDirect) ? -1 : -2, errMsg);
	}

	emit globMsgProcEmitter.progressChange("Idle", 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}
