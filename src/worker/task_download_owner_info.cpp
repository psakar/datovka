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

#include <QThread>

#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
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

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download owner info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskDownloadOwnerInfo::downloadOwnerInfo(const QString &userName,
    QString &error, QString &longError)
{
	struct isds_ctx *session = globIsdsSessions.session(userName);
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
		longError = isds_long_message(session);
		isds_DbOwnerInfo_free(&ownerInfo);
		return false;
	}

	Q_ASSERT(NULL != ownerInfo);

	QString birthDate;
	if ((NULL != ownerInfo->birthInfo) &&
	    (NULL != ownerInfo->birthInfo->biDate)) {
		birthDate = tmBirthToDbFormat(ownerInfo->birthInfo->biDate);
	}

	int ic = 0;
	if (NULL != ownerInfo->ic) {
		ic = QString(ownerInfo->ic).toInt();
	}

	QString key = userName + "___True";

	bool ret = globAccountDbPtr->insertAccountIntoDb(
	    key,
	    ownerInfo->dbID,
	    convertDbTypeToString(*ownerInfo->dbType),
	    ic,
	    ownerInfo->personName ?
	        ownerInfo->personName->pnFirstName : NULL,
	    ownerInfo->personName ?
	        ownerInfo->personName->pnMiddleName : NULL,
	    ownerInfo->personName ?
	        ownerInfo->personName->pnLastName : NULL,
	    ownerInfo->personName ?
	        ownerInfo->personName->pnLastNameAtBirth : NULL,
	    ownerInfo->firmName,
	    birthDate,
	    ownerInfo->birthInfo ?
	        ownerInfo->birthInfo->biCity : NULL,
	    ownerInfo->birthInfo ?
	        ownerInfo->birthInfo->biCounty : NULL,
	    ownerInfo->birthInfo ?
	        ownerInfo->birthInfo->biState : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adCity : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adStreet : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adNumberInStreet : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adNumberInMunicipality : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adZipCode : NULL,
	    ownerInfo->address ?
	        ownerInfo->address->adState : NULL,
	    ownerInfo->nationality,
	    ownerInfo->identifier,
	    ownerInfo->registryCode,
	    (int) *ownerInfo->dbState,
	    *ownerInfo->dbEffectiveOVM,
	    *ownerInfo->dbOpenAddressing);

	isds_DbOwnerInfo_free(&ownerInfo);

	return ret;
}
