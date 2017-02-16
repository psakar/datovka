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
#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner_fulltext.h"

/* ISDS limits the response size to 100 entries. */
const quint64 TaskSearchOwnerFulltext::maxResponseSize = 100;

TaskSearchOwnerFulltext::TaskSearchOwnerFulltext(const QString &userName,
    const QString &query, const isds_fulltext_target *target,
    const isds_DbType *box_type, quint64 pageSize, quint64 pageNumber)
    : m_isdsRetError(IE_ERROR),
    m_pageSize((pageSize <= maxResponseSize) ? pageSize: maxResponseSize),
    m_pageNumber(pageNumber),
    m_totalMatchingBoxes(0),
    m_currentPageStart(0),
    m_currentPageSize(0),
    m_isLastPage(false),
    m_results(NULL),
    m_userName(userName),
    m_query(query),
    m_target(target),
    m_box_type(box_type)
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
	    m_pageSize, m_pageNumber, m_totalMatchingBoxes, m_currentPageStart,
	    m_currentPageSize, m_isLastPage, &m_results);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

int TaskSearchOwnerFulltext::isdsSearch2(const QString &userName,
    const QString &query, const isds_fulltext_target *target,
    const isds_DbType *box_type, quint64 pageSize, quint64 pageNumber,
    quint64 &totalMatchingBoxes, quint64 &currentPageStart,
    quint64 &currentPageSize, bool &isLastPage, struct isds_list **results)
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

	/* For conversion purposes. */
	unsigned long int iPageSize = pageSize;
	unsigned long int iPageNumber = pageNumber;
	unsigned long int *iTotalMatchingBoxes = NULL;
	unsigned long int *iCurrentPageStart = NULL;
	unsigned long int *iCurentPageSize = NULL;
	_Bool *iIsLastPage = NULL;

	ret = isds_find_box_by_fulltext(session, query.toStdString().c_str(),
	    target, box_type, &iPageSize, &iPageNumber,
	    NULL, &iTotalMatchingBoxes, &iCurrentPageStart, &iCurentPageSize,
	    &iIsLastPage, results);

	/* Free allocated stuff. */
	if (iTotalMatchingBoxes != NULL) {
		totalMatchingBoxes = *iTotalMatchingBoxes;
		free(iTotalMatchingBoxes); iTotalMatchingBoxes = NULL;
	}
	if (iCurrentPageStart != NULL) {
		currentPageStart = *iCurrentPageStart;
		free(iCurrentPageStart); iCurrentPageStart = NULL;
	}
	if (iCurentPageSize != NULL) {
		currentPageSize = *iCurentPageSize;
		free(iCurentPageSize); iCurentPageSize = NULL;
	}
	if (iIsLastPage != NULL) {
		isLastPage = *iIsLastPage;
		free(iIsLastPage); iIsLastPage = NULL;
	}

	logDebugLv1NL("Find databox returned '%d': '%s'.",
	    ret, isdsStrError(ret).toUtf8().constData());
	return ret;
}
