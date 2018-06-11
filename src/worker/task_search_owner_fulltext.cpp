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

#include <cstdlib>
#include <QThread>

#include "src/datovka_shared/isds/error.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner_fulltext.h"

/* ISDS limits the response size to 100 entries. */
const quint64 TaskSearchOwnerFulltext::maxResponseSize = 100;

TaskSearchOwnerFulltext::TaskSearchOwnerFulltext(const QString &userName,
    const QString &query, enum Isds::Type::FulltextSearchType target,
    enum BoxType type, qint64 pageNumber, bool askAll, bool highlight)
    : m_result(SOF_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_totalMatchingBoxes(0),
    m_foundBoxes(),
    m_gotLastPage(false),
    m_userName(userName),
    m_query(query),
    m_target(target),
    m_boxType(type),
    m_pageNumber(pageNumber),
    m_askAll(askAll),
    m_highlight(highlight)
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
	    m_pageNumber, m_askAll, m_highlight, m_totalMatchingBoxes,
	    m_gotLastPage, m_foundBoxes, m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Convert box type from task to libisds enum.
 *
 * @param[in] type Task enum type representation.
 * @return Isds enum representation.
 */
static
enum Isds::Type::DbType convertType(enum TaskSearchOwnerFulltext::BoxType type)
{
	switch (type) {
	case TaskSearchOwnerFulltext::BT_ALL:
		return Isds::Type::BT_SYSTEM;
		break;
	case TaskSearchOwnerFulltext::BT_OVM:
		return Isds::Type::BT_OVM;
		break;
	case TaskSearchOwnerFulltext::BT_PO:
		return Isds::Type::BT_PO;
		break;
	case TaskSearchOwnerFulltext::BT_PFO:
		return Isds::Type::BT_PFO;
		break;
	case TaskSearchOwnerFulltext::BT_FO:
		return Isds::Type::BT_FO;
		break;
	default:
		Q_ASSERT(0);
		return Isds::Type::BT_SYSTEM;
		break;
	}
}

/*!
 * @brief Converts libisds error code into task error code.
 *
 * @param[in] err Error code.
 * @return Task error code.
 */
static
enum TaskSearchOwnerFulltext::Result convertError(enum Isds::Type::Error err)
{
	switch (err) {
	case Isds::Type::ERR_SUCCESS:
		return TaskSearchOwnerFulltext::SOF_SUCCESS;
		break;
	case Isds::Type::ERR_2BIG:
	case Isds::Type::ERR_NOEXIST:
	case Isds::Type::ERR_INVAL:
	case Isds::Type::ERR_INVALID_CONTEXT:
		return TaskSearchOwnerFulltext::SOF_BAD_DATA;
		break;
	case Isds::Type::ERR_ISDS:
	case Isds::Type::ERR_NOT_LOGGED_IN:
	case Isds::Type::ERR_CONNECTION_CLOSED:
	case Isds::Type::ERR_TIMED_OUT:
	case Isds::Type::ERR_NETWORK:
	case Isds::Type::ERR_HTTP:
	case Isds::Type::ERR_SOAP:
	case Isds::Type::ERR_XML:
		return TaskSearchOwnerFulltext::SOF_COM_ERROR;
		break;
	default:
		return TaskSearchOwnerFulltext::SOF_ERROR;
		break;
	}
}

enum TaskSearchOwnerFulltext::Result TaskSearchOwnerFulltext::isdsSearch2(
    const QString &userName, const QString &query,
    enum Isds::Type::FulltextSearchType target, enum BoxType type,
    quint64 pageSize, quint64 pageNumber, bool highlight,
    quint64 &totalMatchingBoxes, quint64 &currentPageStart,
    quint64 &currentPageSize, bool &gotLastPage,
    QList<Isds::FulltextResult> &foundBoxes, QString &error, QString &longError)
{
	if (query.isEmpty()) {
		Q_ASSERT(0);
		return SOF_ERROR;
	}

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return SOF_ERROR;
	}

	enum Isds::Type::NilBool lastPage = Isds::Type::BOOL_NULL;
	QList<Isds::FulltextResult> boxes;

	Isds::Error err = Isds::Service::isdsSearch2(session, query, target,
	    convertType(type), pageSize, pageNumber,
	    highlight ? Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE,
	    totalMatchingBoxes, currentPageStart, currentPageSize, lastPage,
	    boxes);

	gotLastPage = (lastPage == Isds::Type::BOOL_TRUE);
	foundBoxes.append(boxes);

	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Searching for data box (page %lu) returned status '%d': '%s'.",
		    (unsigned long)pageNumber, err.code(),
		    error.toUtf8().constData());
	} else {
		logDebugLv1NL("Find data box (page %lu) returned '%d': '%s'.",
		    (unsigned long)pageNumber, err.code(),
		    Isds::Description::descrError(err.code()).toUtf8().constData());
	}

	return convertError(err.code());
}

enum TaskSearchOwnerFulltext::Result TaskSearchOwnerFulltext::isdsSearch2All(
    const QString &userName, const QString &query,
    enum Isds::Type::FulltextSearchType target, enum BoxType type,
    quint64 pageNumber, bool askAll, bool highlight,
    quint64 &totalMatchingBoxes, bool &gotLastPage,
    QList<Isds::FulltextResult> &foundBoxes, QString &error, QString &longError)
{
	enum Result res = SOF_SUCCESS;

	quint64 pageSize = maxResponseSize;
	quint64 currentPageStart = 0;
	quint64 currentPageSize = 0;

	if (gotLastPage) {
		return res;
	}

	do {
		res = isdsSearch2(userName, query, target, type, pageSize,
		    pageNumber, highlight, totalMatchingBoxes,currentPageStart,
		    currentPageSize, gotLastPage, foundBoxes, error, longError);

		++pageNumber;
	} while (askAll && (res == SOF_SUCCESS) && (!gotLastPage));

	if ((unsigned)foundBoxes.size() != totalMatchingBoxes) {
		logWarningNL("Got %d instead of %lu declared found data boxes.",
		    foundBoxes.size(), (unsigned long)totalMatchingBoxes);
	}

	return res;
}
