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
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_owner_info_mojeid.h"
#include "src/web/json.h"

TaskDownloadOwnerInfoMojeId::TaskDownloadOwnerInfoMojeId(int id)
    : m_success(false),
    m_isdsError(),
    m_id(id)
{
}

void TaskDownloadOwnerInfoMojeId::run(void)
{

	logDebugLv0NL("Starting download owner info task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = downloadOwnerInfo(m_id, m_isdsError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download owner info task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskDownloadOwnerInfoMojeId::downloadOwnerInfo(int id, QString &error)
{
	JsonLayer::AccountInfo aInfo;
	JsonLayer lJsonlayer;

	if (lJsonlayer.getAccountInfo(id, aInfo, error)) {
		return globAccountDbPtr->insertAccountIntoDb(
		    aInfo.key + "___True",
		    aInfo.dbID,
		    aInfo.dbType,
		    aInfo.ic.toInt(),
		    aInfo.pnFirstName,
		    aInfo.pnMiddleName,
		    aInfo.pnLastName,
		    aInfo.pnLastNameAtBirth,
		    aInfo.firmName,
		    aInfo.biDate,
		    aInfo.biCity,
		    aInfo.biCounty,
		    aInfo.biState,
		    aInfo.adCity,
		    aInfo.adStreet,
		    aInfo.adNumberInStreet,
		    aInfo.adNumberInMunicipality,
		    aInfo.adZipCode,
		    aInfo.adState,
		    aInfo.nationality,
		    aInfo.identifier,
		    aInfo.registryCode,
		    aInfo.dbState,
		    aInfo.dbEffectiveOVM,
		    aInfo.dbOpenAddressing
		);
	} else {
		return false;
	}
}
