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

TaskSearchOwnerFulltext::BoxEntry::BoxEntry(const QString &i, int t,
    const QString &n, const QString &ad, bool ovm, bool ac, bool ps, bool cs)
    : id(i),
    type(t),
    name(n),
    address(ad),
    effectiveOVM(ovm),
    active(ac),
    publicSending(ps),
    commercialSending(cs)
{
}

TaskSearchOwnerFulltext::TaskSearchOwnerFulltext(const QString &userName,
    const QString &query, enum FulltextTarget target, enum BoxType type,
    quint64 pageSize, quint64 pageNumber)
    : m_isdsRetError(IE_ERROR),
    m_pageSize((pageSize <= maxResponseSize) ? pageSize: maxResponseSize),
    m_pageNumber(pageNumber),
    m_totalMatchingBoxes(0),
    m_currentPageStart(0),
    m_currentPageSize(0),
    m_isLastPage(false),
    m_foundBoxes(),
    m_userName(userName),
    m_query(query),
    m_target(target),
    m_boxType(type)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(NULL != m_query);
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

	m_isdsRetError = isdsSearch2(m_userName, m_query, m_target, m_boxType,
	    m_pageSize, m_pageNumber, m_totalMatchingBoxes, m_currentPageStart,
	    m_currentPageSize, m_isLastPage, m_foundBoxes);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Convert target from task enum to libisds enum.
 *
 * @param[in] target Task enum target representation.
 * @return Libisds enum representation.
 */
static
isds_fulltext_target convertTarget(
    enum TaskSearchOwnerFulltext::FulltextTarget target)
{
	switch (target) {
	case TaskSearchOwnerFulltext::FT_ALL:
		return FULLTEXT_ALL;
		break;
	case TaskSearchOwnerFulltext::FT_ADDRESS:
		return FULLTEXT_ADDRESS;
		break;
	case TaskSearchOwnerFulltext::FT_IC:
		return FULLTEXT_IC;
		break;
	case TaskSearchOwnerFulltext::FT_BOX_ID:
		return FULLTEXT_BOX_ID;
		break;
	default:
		Q_ASSERT(0);
		return FULLTEXT_ALL;
		break;
	}
}

/*!
 * @brief Convert box type from task to libisds enum.
 *
 * @param[in] type Task enum type representation.
 * @return Libisds enum representation.
 */
static
isds_DbType convertType(enum TaskSearchOwnerFulltext::BoxType type)
{
	switch (type) {
	case TaskSearchOwnerFulltext::BT_ALL:
		return DBTYPE_SYSTEM;
		break;
	case TaskSearchOwnerFulltext::BT_OVM:
		return DBTYPE_OVM;
		break;
	case TaskSearchOwnerFulltext::BT_PO:
		return DBTYPE_PO;
		break;
	case TaskSearchOwnerFulltext::BT_PFO:
		return DBTYPE_PFO;
		break;
	case TaskSearchOwnerFulltext::BT_FO:
		return DBTYPE_FO;
		break;
	default:
		Q_ASSERT(0);
		return DBTYPE_SYSTEM;
		break;
	}
}

/*!
 * @brief Adds entries from libisds list into antry list.
 *
 * @param[out] boxList List of boxes to append data to.
 * @param[in]  boxes List of boxes to copy data from.
 */
static
void appendBoxEntries(QList<TaskSearchOwnerFulltext::BoxEntry> &foundBoxes,
    const struct isds_list *boxes)
{
	while (boxes != NULL) {
		const struct isds_fulltext_result *result =
		    (struct isds_fulltext_result *)boxes->data;
		if (result == NULL) {
			Q_ASSERT(0);
			return;
		}

		if (result->dbID == NULL) {
			Q_ASSERT(0);
			return;
		}
		foundBoxes.append(
		    TaskSearchOwnerFulltext::BoxEntry(result->dbID,
		        result->dbType, result->name, result->address,
		        result->dbEffectiveOVM, result->active,
		        result->public_sending, result->commercial_sending));

		boxes = boxes->next;
	}
}

int TaskSearchOwnerFulltext::isdsSearch2(const QString &userName,
    const QString &query, enum FulltextTarget target, enum BoxType type,
    quint64 pageSize, quint64 pageNumber, quint64 &totalMatchingBoxes,
    quint64 &currentPageStart, quint64 &currentPageSize, bool &isLastPage,
    QList<BoxEntry> &foundBoxes)
{
	isds_error ret = IE_ERROR;

	if (NULL == query) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	/* For conversion purposes. */
	isds_fulltext_target iTarget = convertTarget(target);
	isds_DbType iType = convertType(type);
	unsigned long int iPageSize = pageSize;
	unsigned long int iPageNumber = pageNumber;
	unsigned long int *iTotalMatchingBoxes = NULL;
	unsigned long int *iCurrentPageStart = NULL;
	unsigned long int *iCurentPageSize = NULL;
	_Bool *iIsLastPage = NULL;
	struct isds_list *iFoundBoxes = NULL;

	ret = isds_find_box_by_fulltext(session, query.toUtf8().constData(),
	    &iTarget, &iType, &iPageSize, &iPageNumber,
	    NULL, &iTotalMatchingBoxes, &iCurrentPageStart, &iCurentPageSize,
	    &iIsLastPage, &iFoundBoxes);

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
	if (iFoundBoxes != NULL) {
		appendBoxEntries(foundBoxes, iFoundBoxes);
		isds_list_free(&iFoundBoxes);
	}

	logDebugLv1NL("Find databox returned '%d': '%s'.",
	    ret, isdsStrError(ret).toUtf8().constData());
	return ret;
}
