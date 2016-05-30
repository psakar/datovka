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
#include "src/worker/task_get_account_list_mojeid.h"
#include "src/web/json.h"

TaskGetAccountListMojeId::TaskGetAccountListMojeId(void)
    : m_success(false),
    m_isdsError()
{
}

void TaskGetAccountListMojeId::run(void)
{

	logDebugLv0NL("Starting download account list task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = getAccountList(m_isdsError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download account list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskGetAccountListMojeId::getAccountList(QString &error)
{
	QList<JsonLayer::AccountData> accountList;
	QNetworkCookie sessionid;

	emit globMsgProcEmitter.progressChange(PL_GET_ACCOUNT_LIST, -1);

	jsonlayer.getAccountList(sessionid, accountList, error);

	if (!error.isEmpty()) {
		qDebug() << "ERROR:" << error;
		return false;
	}

	if (accountList.isEmpty()) {
		return false;
	}

	return true;
}
