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
#include "src/worker/task_download_owner_info.h"

TaskDownloadOwnerInfo::TaskDownloadOwnerInfo(const QString &userName)
    : m_success(false),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskDownloadOwnerInfo::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download owner info task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = downloadOwnerInfo(m_userName, m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download owner info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskDownloadOwnerInfo::downloadOwnerInfo(const QString &userName,
    QString &error, QString &longError)
{
	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}

	struct isds_DbOwnerInfo *ownerInfo = NULL;
	isds_error status = isds_GetOwnerInfoFromLogin(session, &ownerInfo);
	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading owner information for account '%s' returned '%d': '%s'.",
		    userName.toUtf8().constData(),
		    status, isds_error(status));
		error = isds_error(status);
		longError = isdsLongMessage(session);
		isds_DbOwnerInfo_free(&ownerInfo);
		return false;
	}

	Q_ASSERT(NULL != ownerInfo);

	Isds::DbOwnerInfo dbOwnerInfo;
	bool ok = false;
	dbOwnerInfo = Isds::libisds2dbOwnerInfo(ownerInfo, &ok);
	if (!ok) {
		logErrorNL("%s", "Cannot convert libisds dbOwnerInfo to dbOwnerInfo.");
		isds_DbOwnerInfo_free(&ownerInfo);
		return false;
	}

	isds_DbOwnerInfo_free(&ownerInfo);

	return GlobInstcs::accntDbPtr->insertAccountIntoDb(
	    AccountDb::keyFromLogin(userName), dbOwnerInfo);
}
