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
#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/isds/message_conversion.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_verify_message.h"

TaskVerifyMessage::TaskVerifyMessage(const QString &userName,
    MessageDbSet *dbSet, const MessageDb::MsgId &msgId)
    : m_result(VERIFY_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgId(msgId)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(0 != m_dbSet);
	Q_ASSERT(m_msgId.dmId >= 0);
	Q_ASSERT(m_msgId.deliveryTime.isValid());
}

void TaskVerifyMessage::run(void)
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

	if (!m_msgId.deliveryTime.isValid()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting verify message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = verifyMessage(m_userName, m_dbSet, m_msgId, m_isdsError,
	    m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Verify message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskVerifyMessage::Result TaskVerifyMessage::verifyMessage(
    const QString &userName, MessageDbSet *dbSet, const MessageDb::MsgId &msgId,
    QString &error, QString &longError)
{
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(Q_NULLPTR != dbSet);
	Q_ASSERT(msgId.dmId >= 0);
	Q_ASSERT(msgId.deliveryTime.isValid());

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return VERIFY_ERR;
	}

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return VERIFY_ERR;
	}

	/* Get message hash from isds */
	struct isds_hash *hashIsds = NULL;
	isds_error status = isds_download_message_hash(session,
	    QString::number(msgId.dmId).toUtf8().constData(), &hashIsds);
	if (IE_SUCCESS != status) {
		logErrorNL("Error downloading hash of message '%" PRId64 "'.",
		    msgId.dmId);
		error = isds_strerror(status);
		longError = isdsLongMessage(session);
		isds_hash_free(&hashIsds);
		return VERIFY_ISDS_ERR;
	}
	Q_ASSERT(NULL != hashIsds);

	/* Get message hash from local database */
	const Isds::Hash hashDb = messageDb->getMessageHash(msgId.dmId);
	if (hashDb.isNull()) {
		logErrorNL(
		    "Error obtaining hash of message '%" PRId64 "' from local database.",
		    msgId.dmId);
		isds_hash_free(&hashIsds);
		return VERIFY_SQL_ERR;
	}

	bool ok = false;
	struct isds_hash *hashLocal = Isds::hash2libisds(hashDb, &ok);
	if (!ok) {
		logErrorNL("%s", "Cannot convert hash to libisds hash.");
		isds_hash_free(&hashIsds);
		isds_hash_free(&hashLocal);
		return VERIFY_SQL_ERR;
	}

	/* Compare both hashes */
	status = isds_hash_cmp(hashIsds, hashLocal);

	isds_hash_free(&hashIsds);
	isds_hash_free(&hashLocal);

	if (IE_NOTEQUAL == status) {
		return VERIFY_NOT_EQUAL;
	}

	if (IE_SUCCESS != status) {
		error = isds_strerror(status);
		return VERIFY_ISDS_ERR;
	}

	return VERIFY_SUCCESS;
}
