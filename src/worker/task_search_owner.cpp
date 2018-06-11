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
#include <cstring>
#include <QThread>

#include "src/datovka_shared/isds/box_interface.h"
#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/types.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner.h"

TaskSearchOwner::TaskSearchOwner(const QString &userName,
    const Isds::DbOwnerInfo &dbOwnerInfo)
    : m_result(SO_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_foundBoxes(),
    m_userName(userName),
    m_dbOwnerInfo(dbOwnerInfo)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskSearchOwner::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting search owner task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = isdsSearch(m_userName, m_dbOwnerInfo, m_foundBoxes,
	    m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Converts libisds error code into task error code.
 *
 * @param[in] code Libisds error status.
 * @return Task error code.
 */
static
enum TaskSearchOwner::Result convertError(enum Isds::Type::Error code)
{
	switch (code) {
	case Isds::Type::ERR_SUCCESS:
		return TaskSearchOwner::SO_SUCCESS;
		break;
	case Isds::Type::ERR_2BIG:
	case Isds::Type::ERR_NOEXIST:
	case Isds::Type::ERR_INVAL:
	case Isds::Type::ERR_INVALID_CONTEXT:
		return TaskSearchOwner::SO_BAD_DATA;
		break;
	case Isds::Type::ERR_ISDS:
	case Isds::Type::ERR_NOT_LOGGED_IN:
	case Isds::Type::ERR_CONNECTION_CLOSED:
	case Isds::Type::ERR_TIMED_OUT:
	case Isds::Type::ERR_NETWORK:
	case Isds::Type::ERR_HTTP:
	case Isds::Type::ERR_SOAP:
	case Isds::Type::ERR_XML:
		return TaskSearchOwner::SO_COM_ERROR;
		break;
	default:
		return TaskSearchOwner::SO_ERROR;
		break;
	}
}

enum TaskSearchOwner::Result TaskSearchOwner::isdsSearch(const QString &userName,
    const Isds::DbOwnerInfo &dbOwnerInfo, QList<Isds::DbOwnerInfo> &foundBoxes,
    QString &error, QString &longError)
{
	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return SO_ERROR;
	}

	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return SO_ERROR;
	}

	Isds::Error err = Isds::Service::findDataBox(session, dbOwnerInfo,
	    foundBoxes);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Searching for data box returned status %d: '%s'.",
		    err.code(), error.toUtf8().constData());
	} else {
		logDebugLv1NL("Find databox returned '%d': '%s'.",
		    err.code(), error.toUtf8().constData());
	}

	return convertError(err.code());
}
