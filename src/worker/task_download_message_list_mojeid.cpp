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
#include "src/io/isds_sessions.h"
#include "src/models/accounts_model.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_download_message_list_mojeid.h"
#include "src/worker/task_download_message.h"
#include "src/gui/dlg_import_zfo.h" /* TODO -- Remove this dependency. */
#include "src/web/json.h"

TaskDownloadMessageListMojeID::TaskDownloadMessageListMojeID(
    const QString &userName, MessageDbSet *dbSet,
    enum MessageDirection msgDirect, bool downloadWhole,
    int dmLimit, int accountID, int dmOffset)
    : m_result(DL_ERR),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect),
    m_downloadWhole(downloadWhole),
    m_dmLimit(dmLimit),
    m_accountID(accountID),
    m_dmOffset(dmOffset)
{
	Q_ASSERT(0 != m_dbSet);
}

void TaskDownloadMessageListMojeID::run(void)
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

	logDebugLv0NL("Starting download message list task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	int rt = 0; /*!< Received total. */
	int rn = 0; /*!< Received new. */
	int st = 0; /*!< Sent total. */
	int sn = 0; /*!< Sent new. */

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message list for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent",
	    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	if (MSG_RECEIVED == m_msgDirect) {
		m_result = downloadMessageList(m_userName, MSG_RECEIVED,
		    *m_dbSet, m_downloadWhole, m_error,
		    PL_DOWNLOAD_RECEIVED_LIST, rt, rn, m_dmLimit,
		    m_accountID, m_dmOffset);
	} else {
		m_result = downloadMessageList(m_userName, MSG_SENT,
		    *m_dbSet, m_downloadWhole, m_error,
		    PL_DOWNLOAD_SENT_LIST, st, sn, m_dmLimit,
		    m_accountID, m_dmOffset);
	}

	if (DL_SUCCESS == m_result) {
		logDebugLv1NL("Done downloading message list for account '%s'.",
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	} else {
		logErrorNL("Downloading message list for account '%s' failed.",
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	}

	emit globMsgProcEmitter.downloadMessageListFinished(m_userName,
	    m_msgDirect, m_result, m_error, true, rt, rn , st, sn);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskDownloadMessageListMojeID::Result
TaskDownloadMessageListMojeID::downloadMessageList(
    const QString &userName, enum MessageDirection msgDirect,
    MessageDbSet &dbSet, bool downloadWhole, QString &error,
    const QString &progressLabel, int &total, int &news,
    int dmLimit, int accountID, int dmOffset)
{
	debugFuncCall();

	Q_UNUSED(downloadWhole);

	#define USE_TRANSACTIONS 1

	int newcnt = 0;
	int allcnt = 0;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	emit globMsgProcEmitter.progressChange(progressLabel, 10);

	QList<JsonLayer::MsgEnvelope> messageList;

	/* webdatovka message types */
	int msgType = 1;
	if (MSG_SENT == msgDirect) {
		msgType = -1;
	}

	JsonLayer lJsonlayer;
	lJsonlayer.getMessageList(accountID, msgType, dmLimit, dmOffset,
	    messageList, error);
	if (!error.isEmpty()) {
		qDebug() << "ERROR:" << error;
		return DL_NET_ERROR;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 20);

	float delta = 0.0;
	float diff = 0.0;
	allcnt = messageList.count();

	if (allcnt == 0) {
		emit globMsgProcEmitter.progressChange(progressLabel, 50);
	} else {
		delta = 80.0 / allcnt;
	}

	QByteArray zfoData;
	for (int i = 0; i < allcnt; ++i) {

		newcnt++;
		diff += delta;

		emit globMsgProcEmitter.progressChange(progressLabel,
		    (int) (20 + diff));

		zfoData = lJsonlayer.downloadMessage(messageList.at(i).id,
		    error);

		if (zfoData.isEmpty()) {
			continue;
		}

		struct isds_ctx *dummy_session = isds_ctx_create();
		if (NULL == dummy_session) {
			logError("%s\n", "Cannot create dummy ISDS session.");
			return DL_ERR;
		}

		struct isds_message *message;
		message = loadZfoData(dummy_session, zfoData,
		    ImportZFODialog::IMPORT_MESSAGE_ZFO);
		if (NULL == message) {
			logError("%s\n", "Cannot parse message data.");
			return DL_ERR;
		}

		Task::storeEnvelope(msgDirect, dbSet, message->envelope);
		Task::storeMessage(true, msgDirect, dbSet, message,
		    progressLabel);
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 100);

	if (MSG_RECEIVED == msgDirect) {
		logDebugLv0NL("#Received total: %d #Received new: %d",
		    allcnt, newcnt);
	} else {
		logDebugLv0NL("#Sent total: %d #Sent new: %d", allcnt, newcnt);
	}

	total = allcnt;
	news =  newcnt;

	return DL_SUCCESS;

#undef USE_TRANSACTIONS
}
