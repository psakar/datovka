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

#include "src/datovka_shared/isds/box_interface.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/error.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/isds/types.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_credit_info.h"

TaskDownloadCreditInfo::TaskDownloadCreditInfo(const QString &userName,
    const QString &dbId)
    : m_heller(-1),
    m_isdsError(),
    m_isdsLongError(),
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

	m_heller = downloadCreditFromISDS(m_userName, m_dbId,
	    m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download credit info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

qint64 TaskDownloadCreditInfo::downloadCreditFromISDS(const QString &userName,
    const QString &dbId, QString &error, QString &longError)
{
	if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(userName)) {
		return -1;
	}

	qint64 credit = 0;
	QString email;
	QList<Isds::CreditEvent> history;

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return -1;
	}

	Isds::Error err = Isds::Service::dataBoxCreditInfo(session,
	    dbId, QDate(), QDate(), credit, email, history);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading credit information returned '%d': '%s'.",
		    err.code(), longError.toUtf8().constData());
		return -1;
	}

	return credit;
}
