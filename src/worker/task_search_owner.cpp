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
    : m_result(SO_ERROR),
    m_isdsError(),
    m_isdsLongError(),
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

	m_result = isdsSearch(m_userName, m_info, &m_results,
	    m_isdsError, m_isdsLongError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Converts libisds error code into task error code.
 *
 * @param[in] status Libisds error status.
 * @return Task error code.
 */
static
enum TaskSearchOwner::Result convertError(int status)
{
	switch (status) {
	case IE_SUCCESS:
		return TaskSearchOwner::SO_SUCCESS;
		break;
	case IE_2BIG:
	case IE_NOEXIST:
	case IE_INVAL:
	case IE_INVALID_CONTEXT:
		return TaskSearchOwner::SO_BAD_DATA;
		break;
	case IE_ISDS:
	case IE_NOT_LOGGED_IN:
	case IE_CONNECTION_CLOSED:
	case IE_TIMED_OUT:
	case IE_NETWORK:
	case IE_HTTP:
	case IE_SOAP:
	case IE_XML:
		return TaskSearchOwner::SO_COM_ERROR;
		break;
	default:
		return TaskSearchOwner::SO_ERROR;
		break;
	}
}

enum TaskSearchOwner::Result TaskSearchOwner::isdsSearch(
    const QString &userName, const struct isds_DbOwnerInfo *info,
    struct isds_list **results, QString &error, QString &longError)
{
	isds_error status = IE_ERROR;

	if ((NULL == results) || (NULL == info)) {
		Q_ASSERT(0);
		return SO_ERROR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return SO_ERROR;
	}

	status = isds_FindDataBox(session, info, results);

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading delivery information returned status %d: '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
		error = isds_error(status);
		longError = isdsLongMessage(session);
	} else {
		logDebugLv1NL("Find databox returned '%d': '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
	}

	return convertError(status);
}
