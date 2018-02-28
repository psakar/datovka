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

#include <cinttypes>
#include <QThread>

#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_erase_message.h"

TaskEraseMessage::TaskEraseMessage(const QString &userName, MessageDbSet *dbSet,
    const MessageDb::MsgId &msgId, enum MessageDirection msgDirect,
    bool delFromIsds)
    : m_result(NOT_DELETED),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgId(msgId),
    m_msgDirect(msgDirect),
    m_delFromIsds(delFromIsds)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(0 != m_dbSet);
	Q_ASSERT(m_msgId.dmId >= 0);
}

void TaskEraseMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	if (0 > m_msgId.dmId) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting erase message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = eraseMessage(m_userName, m_dbSet, m_msgId, m_msgDirect,
	    m_delFromIsds, m_isdsError, m_isdsLongError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Erase message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskEraseMessage::Result TaskEraseMessage::eraseMessage(
    const QString &userName, MessageDbSet *dbSet, const MessageDb::MsgId &msgId,
    enum MessageDirection msgDirect, bool delFromIsds, QString &error,
    QString &longError)
{
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(0 != dbSet);
	Q_ASSERT(msgId.dmId >= 0);

	isds_error status = IE_SUCCESS;

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return NOT_DELETED;
	}

	/* First delete message in ISDS. */
	if (delFromIsds) {
		struct isds_ctx *session =
		    GlobInstcs::isdsSessionsPtr->session(userName);
		if (NULL == session) {
			Q_ASSERT(0);
			return NOT_DELETED;
		}

		status = isds_delete_message_from_storage(session,
		    QString::number(msgId.dmId).toUtf8().constData(),
		    msgDirect == MSG_RECEIVED);

		if (IE_SUCCESS == status) {
			logDebugLv1NL(
			    "Message '%" PRId64 "' was deleted from ISDS.",
			    msgId.dmId);
		} else {
			error = isdsStrError(status);
			longError = isdsLongMessage(session);

			logErrorNL(
			    "Erasing message '%" PRId64 "'from ISDS returned status '%d': '%s'",
			    msgId.dmId, status, error.toUtf8().constData());
		}
	}

	if ((IE_SUCCESS != status) && (IE_INVAL != status)) {
		return NOT_DELETED;
	}

	if (messageDb->msgsDeleteMessageData(msgId.dmId)) {
		logDebugLv1NL(
		    "Message '%" PRId64 "' was deleted from local database.",
		    msgId.dmId);
		return (delFromIsds && (IE_SUCCESS == status)) ? DELETED_ISDS_LOCAL : DELETED_LOCAL;
	} else {
		logErrorNL(
		    "Could not delete message '%" PRId64 "' from local database.",
		    msgId.dmId);
		return (delFromIsds && (IE_SUCCESS == status)) ? DELETED_ISDS : NOT_DELETED;
	}
}
