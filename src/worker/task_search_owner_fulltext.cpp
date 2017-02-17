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
    const QString &query, enum FulltextTarget target, enum BoxType type)
    : m_result(SOF_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_totalMatchingBoxes(0),
    m_foundBoxes(),
    m_userName(userName),
    m_query(query),
    m_target(target),
    m_boxType(type)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(!m_query.isEmpty());
}

void TaskSearchOwnerFulltext::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_query.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting search owner task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = isdsSearch2All(m_userName, m_query, m_target, m_boxType,
	    m_totalMatchingBoxes, m_foundBoxes, m_isdsError, m_isdsLongError);

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

/*!
 * @brief Converts libisds error code into task error code.
 *
 * @param[in] status Libisds error status.
 * @return Task error code.
 */
static
enum TaskSearchOwnerFulltext::Result convertError(int status)
{
	switch (status) {
	case IE_SUCCESS:
		return TaskSearchOwnerFulltext::SOF_SUCCESS;
		break;
	case IE_2BIG:
	case IE_NOEXIST:
	case IE_INVAL:
	case IE_INVALID_CONTEXT:
		return TaskSearchOwnerFulltext::SOF_BAD_DATA;
		break;
	case IE_ISDS:
	case IE_NOT_LOGGED_IN:
	case IE_CONNECTION_CLOSED:
	case IE_TIMED_OUT:
	case IE_NETWORK:
	case IE_HTTP:
	case IE_SOAP:
	case IE_XML:
		return TaskSearchOwnerFulltext::SOF_COM_ERROR;
		break;
	default:
		return TaskSearchOwnerFulltext::SOF_ERROR;
		break;
	}
}

enum TaskSearchOwnerFulltext::Result TaskSearchOwnerFulltext::isdsSearch2(
    const QString &userName, const QString &query, enum FulltextTarget target,
    enum BoxType type, quint64 pageSize, quint64 pageNumber,
    quint64 &totalMatchingBoxes, quint64 &currentPageStart,
    quint64 &currentPageSize, bool &isLastPage, QList<BoxEntry> &foundBoxes,
    QString &error, QString &longError)
{
	isds_error status = IE_ERROR;

	if (NULL == query) {
		Q_ASSERT(0);
		return SOF_ERROR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return SOF_ERROR;
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

	status = isds_find_box_by_fulltext(session, query.toUtf8().constData(),
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

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Searching for data box returned status %d: '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
		error = isds_error(status);
		longError = isdsLongMessage(session);
	} else {
		logDebugLv1NL("Find databox returned '%d': '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
	}

	return convertError(status);
}

enum TaskSearchOwnerFulltext::Result TaskSearchOwnerFulltext::isdsSearch2All(
    const QString &userName, const QString &query, enum FulltextTarget target,
    enum BoxType type, quint64 &totalMatchingBoxes, QList<BoxEntry> &foundBoxes,
    QString &error, QString &longError)
{
	enum Result res = SOF_SUCCESS;

	quint64 pageSize = maxResponseSize;
	quint64 pageNumber = 0;
	quint64 currentPageStart = 0;
	quint64 currentPageSize = 0;
	bool isLastPage = false;

	while ((res == SOF_SUCCESS) && (!isLastPage)) {
		res = isdsSearch2(userName, query, target, type,
		    pageSize, pageNumber, totalMatchingBoxes, currentPageStart,
		    currentPageSize, isLastPage, foundBoxes, error, longError);

		++pageNumber;
	}

	if ((unsigned)foundBoxes.size() != totalMatchingBoxes) {
		logWarningNL("Got %d instead og %lu declated found data boxes.",
		    foundBoxes.size(), (unsigned long)totalMatchingBoxes);
	}

	return res;
}
