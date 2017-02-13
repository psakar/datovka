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
#include "src/worker/task_search_owner_fulltext.h"

TaskSearchOwnerFulltext::TaskSearchOwnerFulltext(const QString &userName,
    const QString &query, const isds_fulltext_target *target,
    const isds_DbType *box_type, long *page, long *pageSize)
    : m_isdsRetError(IE_ERROR),
    m_results(NULL),
    m_userName(userName),
    m_query(query),
    m_target(target),
    m_box_type(box_type),
    m_page(page),
    m_pageSize(pageSize)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(NULL != m_query);
}

TaskSearchOwnerFulltext::~TaskSearchOwnerFulltext(void)
{
	isds_list_free(&m_results);
}

void TaskSearchOwnerFulltext::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (NULL == m_query) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting search owner task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_isdsRetError = isdsSearch2(m_userName, m_query, m_target, m_box_type,
	    m_page, m_pageSize, &m_results);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

int TaskSearchOwnerFulltext::isdsSearch2(const QString &userName,
    const QString &query, const isds_fulltext_target *target,
    const isds_DbType *box_type, long *page, long *pageSize,
    struct isds_list **results)
{
	isds_error ret = IE_ERROR;

	if ((NULL == results) || (NULL == query)) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	ret = isds_find_box_by_fulltext(session, query.toStdString().c_str(),
	    target, box_type, (unsigned long *)pageSize, (unsigned long*)page, NULL, NULL, NULL, NULL, NULL, results);

	logDebugLv1NL("Find databox returned '%d': '%s'.",
	    ret, isdsStrError(ret).toUtf8().constData());
	return ret;
}
