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

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/message_conversion.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_send_message.h"

TaskSendMessage::ResultData::ResultData(void)
    : result(SM_ERR),
    errInfo(),
    dbIDRecipient(),
    recipientName(),
    isPDZ(false),
    dmId(-1)
{
}

TaskSendMessage::ResultData::ResultData(enum Result res, const QString &eInfo,
    const QString &recId, const QString &recName, bool pdz, qint64 mId)
    : result(res),
    errInfo(eInfo),
    dbIDRecipient(recId),
    recipientName(recName),
    isPDZ(pdz),
    dmId(mId)
{
}

TaskSendMessage::TaskSendMessage(const QString &userName,
    MessageDbSet *dbSet, const QString &transactId, const Isds::Message &message,
    const QString &recipientName, const QString &recipientAddress, bool isPDZ)
    : m_resultData(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_transactId(transactId),
    m_message(message),
    m_recipientName(recipientName),
    m_recipientAddress(recipientAddress),
    m_isPDZ(isPDZ)
{
	Q_ASSERT(0 != m_dbSet);
}

void TaskSendMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting send message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	sendMessage(m_userName, *m_dbSet, m_message, m_recipientName,
	    m_recipientAddress, m_isPDZ, PL_SEND_MESSAGE, &m_resultData);

	emit GlobInstcs::msgProcEmitterPtr->sendMessageFinished(m_userName,
	    m_transactId, m_resultData.result, m_resultData.errInfo,
	    m_resultData.dbIDRecipient, m_resultData.recipientName,
	    m_isPDZ, m_resultData.dmId);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Send message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskSendMessage::Result TaskSendMessage::sendMessage(
    const QString &userName, MessageDbSet &dbSet, const Isds::Message &message,
    const QString &recipientName, const QString &recipientAddress, bool isPDZ,
    const QString &progressLabel, TaskSendMessage::ResultData *resultData)
{
	Q_ASSERT(!userName.isEmpty());

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 0);

	enum TaskSendMessage::Result ret = SM_ERR;
	qint64 dmId = -1;
	struct isds_message *isdsMessage = NULL;
	isds_error status;
	struct isds_ctx *session = NULL;
	QString isdsError, isdsLongError;
	bool ok = false;

	session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		logErrorNL("%s", "Missing ISDS session.");
		ret = SM_ERR;
		goto fail;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 40);

	isdsMessage = Isds::message2libisds(message, &ok);
	if (!ok) {
		logErrorNL("%s", "Cannot convert lo libisds message.");
		ret = SM_ERR;
		goto fail;
	}

	logInfo("Sending message from user '%s'.\n",
	    userName.toUtf8().constData());

	status = isds_send_message(session, isdsMessage);
	if (IE_SUCCESS != status) {
		isdsError = isds_error(status);
		isdsLongError = isdsLongMessage(session);
		logErrorNL("Sending message returned status %d: '%s' '%s'.",
		    status, isdsError.toUtf8().constData(),
		    isdsLongError.toUtf8().constData());
		ret = SM_ISDS_ERROR;
		goto fail;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 70);

	{
		/*
		 * Some values have to be retrieved from libisds structure
		 * after the message has been sent.
		 */
		{
			bool ok = false;
			dmId = QString(isdsMessage->envelope->dmID).toLongLong(&ok);
			if (!ok) {
				Q_ASSERT(0);
				dmId = -1;
			}
		}

		QDateTime deliveryTime(
		    timevalToDateTime(isdsMessage->envelope->dmDeliveryTime));

		MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime,
		    true);
		if (Q_NULLPTR == messageDb) {
			Q_ASSERT(0);
			ret = SM_DB_INS_ERR;
			goto fail;
		}

		const QString acntDbKey(AccountDb::keyFromLogin(userName));
		const QString dbId(GlobInstcs::accntDbPtr->dbId(acntDbKey));
		const QString senderName(
		    GlobInstcs::accntDbPtr->senderNameGuess(acntDbKey));

		if (!messageDb->msgsInsertNewlySentMessageEnvelope(dmId, dbId,
		        senderName, message.envelope().dbIDRecipient(),
		        recipientName, recipientAddress,
		        message.envelope().dmAnnotation())) {
			logErrorNL(
			    "Cannot insert newly sent message '%" PRId64 "' into database.",
			    dmId);
			ret = SM_DB_INS_ERR;
			goto fail;
		}

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, 80);

		//Task::storeAttachments(*messageDb, dmId, message.documents());

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, 90);
	}

	ret = SM_SUCCESS;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 100);

fail:
	isds_message_free(&isdsMessage);

	if (0 != resultData) {
		resultData->result = ret;
		resultData->dbIDRecipient = message.envelope().dbIDRecipient();
		resultData->recipientName = recipientName;
		resultData->dmId = dmId;
		resultData->isPDZ = isPDZ;
		resultData->errInfo =
		    (!isdsError.isEmpty() || !isdsLongError.isEmpty()) ?
		        isdsError + " " + isdsLongError : ""; 
	}

	return ret;
}
