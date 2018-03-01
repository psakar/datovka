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

#include <QThread>

#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_credit_info.h"

TaskDownloadCreditInfo::TaskDownloadCreditInfo(const QString &userName,
    const QString &dbId)
    : m_heller(-1),
    m_userName(userName),
    m_dbId(dbId)
{
	Q_ASSERT(!m_userName.isEmpty());
	Q_ASSERT(!m_dbId.isEmpty());
}

void TaskDownloadCreditInfo::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_dbId.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download credit info task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_heller = downloadCreditFromISDS(m_userName, m_dbId);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download credit info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

qint64 TaskDownloadCreditInfo::downloadCreditFromISDS(const QString &userName,
    const QString &dbId)
{
	long credit = 0;

	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName)) {
		return -1;
	}

	isds_error status;
	struct isds_list *history = NULL;

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return -1;
	}

	status = isds_get_commercial_credit(session,
	    dbId.toUtf8().constData(), NULL, NULL, &credit, NULL, &history);

	isds_list_free(&history);

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading credit information returned '%d': '%s'.",
		    status, isdsLongMessage(session).toUtf8().constData());
		return -1;
	}

	return credit;
}
