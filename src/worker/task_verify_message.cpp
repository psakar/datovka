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

#include <cinttypes> /* PRId64 */
#include <cstdlib>
#include <QThread>

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/types.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_verify_message.h"

TaskVerifyMessage::TaskVerifyMessage(const QString &userName, qint64 dmId,
    const Isds::Hash &hashLocal)
    : m_result(VERIFY_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_dmId(dmId),
    m_hashLocal(hashLocal)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(m_dmId >= 0);
	Q_ASSERT(!m_hashLocal.isNull());
}

void TaskVerifyMessage::run(void)
{
	if (Q_UNLIKELY(m_userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	if (Q_UNLIKELY(0 > m_dmId)) {
		Q_ASSERT(0);
		return;
	}

	if (Q_UNLIKELY(m_hashLocal.isNull())) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting verify message task in thread '%p'",
	    (void *)QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = verifyMessage(m_userName, m_dmId, m_hashLocal, m_isdsError,
	    m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Verify message task finished in thread '%p'",
	    (void *)QThread::currentThreadId());
}

enum TaskVerifyMessage::Result TaskVerifyMessage::verifyMessage(
    const QString &userName, qint64 dmId, const Isds::Hash &hashLocal,
    QString &error, QString &longError)
{
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(dmId >= 0);
	Q_ASSERT(!hashLocal.isNull());

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return VERIFY_ERR;
	}

	/* Get message hash from isds */
	Isds::Hash hashIsds;
	Isds::Error err = Isds::Service::verifyMessage(session, dmId, hashIsds);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL("Error downloading hash of message '%" PRId64 "'.",
		    dmId);
		return VERIFY_ISDS_ERR;
	}
	Q_ASSERT(!hashIsds.isNull());

	if (hashIsds != hashLocal) {
		return VERIFY_NOT_EQUAL;
	}

	return VERIFY_SUCCESS;
}
