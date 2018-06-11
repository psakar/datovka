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

#include <QDateTime>
#include <QThread>

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/types.h"
#include "src/io/account_db.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_password_info.h"

TaskDownloadPasswordInfo::TaskDownloadPasswordInfo(const QString &userName)
    : m_success(false),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskDownloadPasswordInfo::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download password info task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = downloadPasswordInfoFromISDS(m_userName, m_isdsError,
	    m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download password info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskDownloadPasswordInfo::downloadPasswordInfoFromISDS(
    const QString &userName, QString &error, QString &longError)
{
	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}

	QDateTime expiration;
	Isds::Error err = Isds::Service::getPasswordInfo(session, expiration);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading password information for account '%s' returned '%d': '%s'.",
		    userName.toUtf8().constData(),
		    err.code(), error.toUtf8().constData());
		return false;
	}

	return GlobInstcs::accntDbPtr->setPwdExpirIntoDb(
	    AccountDb::keyFromLogin(userName), expiration);
}
