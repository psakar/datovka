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
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/box_conversion.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_user_info.h"

TaskDownloadUserInfo::TaskDownloadUserInfo(const QString &userName)
    : m_success(false),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskDownloadUserInfo::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download user info task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = downloadUserInfo(m_userName, m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download user info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskDownloadUserInfo::downloadUserInfo(const QString &userName,
    QString &error, QString &longError)
{
	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}

	struct isds_DbUserInfo *userInfo = NULL;
	isds_error status = isds_GetUserInfoFromLogin(session, &userInfo);
	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading user information for account '%s' returned '%d': '%s'.",
		    userName.toUtf8().constData(),
		    status, isds_strerror(status));
		error = isds_strerror(status);
		longError = isdsLongMessage(session);
		isds_DbUserInfo_free(&userInfo);
		return false;
	}

	Q_ASSERT(NULL != userInfo);

	bool ok = false;
	Isds::DbUserInfo dbUserInfo(Isds::libisds2dbUserInfo(userInfo, &ok));
	if (!ok) {
		logErrorNL("%s", "Cannot convert libisds dbUserInfo to dbUserInfo.");
		isds_DbUserInfo_free(&userInfo);
		return false;
	}

	isds_DbUserInfo_free(&userInfo);

	return GlobInstcs::accntDbPtr->insertUserIntoDb(
	    AccountDb::keyFromLogin(userName), dbUserInfo);
}
