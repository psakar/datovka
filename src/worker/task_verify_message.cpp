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

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
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

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Verify message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Returns hash of local message.
 *
 * @param[in] messageDb Message database.
 * @param[in] dmId      Message identifier.
 * @return Pointer to hash structure or NULL on error.
 */
static
struct isds_hash *localMessageHash(MessageDb *messageDb, qint64 dmId)
{
	Q_ASSERT(0 != messageDb);

	struct isds_hash *hashLocal = NULL;
	hashLocal = (struct isds_hash *) malloc(sizeof(struct isds_hash));
	if (NULL == hashLocal) {
		return NULL;
	}
	memset(hashLocal, 0, sizeof(struct isds_hash));

	MessageDb::MessageHash hashLocaldata(
	    messageDb->msgsGetHashFromDb(dmId));

	if (!hashLocaldata.isValid()) {
		isds_hash_free(&hashLocal);
		return NULL;
	}

	QByteArray rawHash = QByteArray::fromBase64(hashLocaldata.valueBase64);
	hashLocal->length = (size_t) rawHash.size();
	hashLocal->algorithm = (isds_hash_algorithm) convertHashAlg2(
	    hashLocaldata.alg);
	hashLocal->value = malloc(hashLocal->length);
	if (NULL == hashLocal->value) {
		isds_hash_free(&hashLocal);
		return NULL;
	}
	memcpy(hashLocal->value, rawHash.data(), hashLocal->length);

	return hashLocal;
}

enum TaskVerifyMessage::Result TaskVerifyMessage::verifyMessage(
    const QString &userName, MessageDbSet *dbSet, const MessageDb::MsgId &msgId,
    QString &error, QString &longError)
{
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(0 != dbSet);
	Q_ASSERT(msgId.dmId >= 0);
	Q_ASSERT(msgId.deliveryTime.isValid());

	MessageDb *messageDb = dbSet->accessMessageDb(msgId.deliveryTime,
	    false);
	if (0 == messageDb) {
		Q_ASSERT(0);
		return VERIFY_ERR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return VERIFY_ERR;
	}

	struct isds_hash *hashIsds = NULL;
	isds_error status = isds_download_message_hash(session,
	    QString::number(msgId.dmId).toUtf8().constData(), &hashIsds);
	if (IE_SUCCESS != status) {
		logErrorNL("Error downloading hash of message '%" PRId64 "'.",
		    msgId.dmId);
		error = isds_error(status);
		longError = isdsLongMessage(session);
		isds_hash_free(&hashIsds);
		return VERIFY_ISDS_ERR;
	}

	Q_ASSERT(NULL != hashIsds);

	struct isds_hash *hashLocal = localMessageHash(messageDb, msgId.dmId);
	if (NULL == hashLocal) {
		logErrorNL(
		    "Error obtaining hash of message '%" PRId64 "' from local database.",
		    msgId.dmId);
		isds_hash_free(&hashIsds);
		return VERIFY_SQL_ERR;
	}

	status = isds_hash_cmp(hashIsds, hashLocal);

	isds_hash_free(&hashIsds);
	isds_hash_free(&hashLocal);

	if (IE_NOTEQUAL == status) {
		return VERIFY_NOT_EQUAL;
	}

	if (IE_SUCCESS != status) {
		error = isds_error(status);
		return VERIFY_ISDS_ERR;
	}

	return VERIFY_SUCCESS;
}
