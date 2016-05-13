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

#include <cinttypes>
#include <QThread>

#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message_mojeid.h"


#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/io/isds_sessions.h"
#include "src/models/accounts_model.h"
#include "src/worker/message_emitter.h"
#include "src/gui/dlg_import_zfo.h" /* TODO -- Remove this dependency. */
#include "src/web/json.h"

TaskDownloadMessageMojeId::TaskDownloadMessageMojeId(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect, int id)
    : m_result(DM_ERR),
    m_error(),
    m_id(id),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect)
{
	Q_ASSERT(0 != dbSet);
}

void TaskDownloadMessageMojeId::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	if ((MSG_RECEIVED != m_msgDirect) && (MSG_SENT != m_msgDirect)) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message '%" PRId64 "' for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent", m_id,
	    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	m_result = downloadMessage(m_id, m_msgDirect, *m_dbSet,
	    m_error, PL_DOWNLOAD_MESSAGE);

	if (DM_SUCCESS == m_result) {
		logDebugLv1NL(
		    "Done downloading message '%" PRId64 "' for account '%s'.",
		    m_id,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	} else {
		logErrorNL("Downloading message '%" PRId64 "' for account '%s' failed.",
		    m_id,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	}

	//emit globMsgProcEmitter.downloadMessageFinished(m_userName, m_id,
	//    m_msgDirect, m_result, m_error, m_listScheduled);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskDownloadMessageMojeId::Result TaskDownloadMessageMojeId::downloadMessage(
    int id, enum MessageDirection msgDirect, MessageDbSet &dbSet, QString &error,
    const QString &progressLabel)
{
	debugFuncCall();

	logDebugLv0NL("Trying to download complete message '%" PRId64 "'", id);

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	JsonLayer lJsonlayer;
	QByteArray zfoData = lJsonlayer.downloadMessage(id, error);

	emit globMsgProcEmitter.progressChange(progressLabel, 30);

	if (zfoData.isEmpty()) {
		return DM_ERR;
	}

	struct isds_ctx *dummy_session = isds_ctx_create();
	if (NULL == dummy_session) {
		logError("%s\n", "Cannot create dummy ISDS session.");
		return DM_ERR;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 40);

	struct isds_message *message;
	message = loadZfoData(dummy_session, zfoData,
	    ImportZFODialog::IMPORT_MESSAGE_ZFO);
	if (NULL == message) {
		logError("%s\n", "Cannot parse message data.");
		return DM_ERR;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 60);

	Task::storeEnvelope(msgDirect, dbSet, message->envelope);
	emit globMsgProcEmitter.progressChange(progressLabel, 80);

	Task::storeMessage(true, msgDirect, dbSet, message, progressLabel);
	emit globMsgProcEmitter.progressChange(progressLabel, 100);

	logDebugLv0NL("Done with %s().", __func__);

	return DM_SUCCESS;
}
