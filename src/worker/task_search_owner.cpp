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

#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner.h"

TaskSearchOwner::TaskSearchOwner(const QString &userName,
    const struct isds_DbOwnerInfo *info)
    : m_isdsRetError(IE_ERROR),
    m_results(NULL),
    m_userName(userName),
    m_info(info)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(NULL != m_info);
}

TaskSearchOwner::~TaskSearchOwner(void)
{
	isds_list_free(&m_results);
}

void TaskSearchOwner::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (NULL == m_info) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting search owner task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_isdsRetError = isdsSearch(m_userName, m_info, &m_results);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

int TaskSearchOwner::isdsSearch(const QString &userName,
    const struct isds_DbOwnerInfo *info, struct isds_list **results)
{
	isds_error ret = IE_ERROR;

	if ((NULL == results) || (NULL == info)) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	ret = isds_FindDataBox(session, info, results);

	logDebugLv1NL("Find databox returned '%d': '%s'.",
	    ret, isds_strerror(ret));
	return ret;
}
