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
#include <QThread>

#include "src/datovka_shared/worker/pool.h" /* List with whole messages. */
#include "src/global.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/isds_conversion.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h" /* List with whole messages. */
#include "src/worker/task_download_message_list.h"

TaskDownloadMessageList::TaskDownloadMessageList(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect, bool downloadWhole,
    unsigned long dmLimit, int dmStatusFltr)
    : m_result(DL_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_newMsgIdList(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect),
    m_downloadWhole(downloadWhole),
    m_dmLimit(dmLimit),
    m_dmStatusFilter(dmStatusFltr)
{
	Q_ASSERT(0 != m_dbSet);
}

void TaskDownloadMessageList::run(void)
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

	/* dmStatusFilter
	 *
	 * MESSAGESTATE_SENT |
	 * MESSAGESTATE_STAMPED |
	 * MESSAGESTATE_DELIVERED |
	 * MESSAGESTATE_RECEIVED |
	 * MESSAGESTATE_READ |
	 * MESSAGESTATE_REMOVED |
	 * MESSAGESTATE_IN_SAFE |
	 * MESSAGESTATE_ANY
	 */

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message list for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent",
	    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	if (MSG_RECEIVED == m_msgDirect) {
		m_result = downloadMessageList(m_userName, MSG_RECEIVED,
		    *m_dbSet, m_downloadWhole, m_isdsError, m_isdsLongError,
		    PL_DOWNLOAD_RECEIVED_LIST, rt, rn, m_newMsgIdList, &m_dmLimit,
		    m_dmStatusFilter);
	} else {
		m_result = downloadMessageList(m_userName, MSG_SENT,
		    *m_dbSet, m_downloadWhole, m_isdsError, m_isdsLongError,
		    PL_DOWNLOAD_SENT_LIST, st, sn, m_newMsgIdList, &m_dmLimit,
		    m_dmStatusFilter);
	}

	if (DL_SUCCESS == m_result) {
		logDebugLv1NL("Done downloading message list for account '%s'.",
		    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	} else {
		logErrorNL("Downloading message list for account '%s' failed.",
		    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	}

	emit GlobInstcs::msgProcEmitterPtr->downloadMessageListFinished(
	    m_userName, m_msgDirect, m_result,
	    (!m_isdsError.isEmpty() || !m_isdsLongError.isEmpty()) ?
	        m_isdsError + " " + m_isdsLongError : "",
	    true, rt, rn , st, sn);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskDownloadMessageList::Result TaskDownloadMessageList::downloadMessageList(
    const QString &userName, enum MessageDirection msgDirect,
    MessageDbSet &dbSet, bool downloadWhole, QString &error, QString &longError,
    const QString &progressLabel, int &total, int &news,
    QList<qint64> &newMsgIdList, ulong *dmLimit, int dmStatusFilter)
{
	#define USE_TRANSACTIONS 1
	debugFuncCall();

	int newcnt = 0;
	int allcnt = 0;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 0);

	isds_error status = IE_ERROR;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 10);

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DL_ERR;
	}
	struct isds_list *messageList = NULL;

	/* Download sent/received message list from ISDS for current account */
	if (MSG_SENT == msgDirect) {
		status = isds_get_list_of_sent_messages(session,
		    NULL, NULL, NULL, dmStatusFilter, 0, dmLimit, &messageList);
	} else if (MSG_RECEIVED == msgDirect) {
		status = isds_get_list_of_received_messages(session,
		    NULL, NULL, NULL, dmStatusFilter, 0, dmLimit, &messageList);
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 20);

	if (status != IE_SUCCESS) {
		error = isds_error(status);
		longError = isdsLongMessage(session);
		logErrorNL(
		    "Downloading message list returned status %d: '%s' '%s'.",
		    status, error.toUtf8().constData(),
		    longError.toUtf8().constData());
		isds_list_free(&messageList);
		return DL_ISDS_ERROR;
	}

	const struct isds_list *box;
	box = messageList;
	float delta = 0.0;
	float diff = 0.0;

	while (0 != box) {
		allcnt++;
		box = box->next;
	}

	box = messageList;

	if (allcnt == 0) {
		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, 50);
	} else {
		delta = 80.0 / allcnt;
	}

	/* Obtain invalid message database if has a separate file. */
	MessageDb *invalidDb = NULL;
	{
		QString invSecKey(dbSet.secondaryKey(QDateTime()));
		QString valSecKey(dbSet.secondaryKey(
		    QDateTime::currentDateTime()));
		if (invSecKey != valSecKey) {
			/* Invalid database file may not exist. */
			invalidDb = dbSet.accessMessageDb(QDateTime(), false);
		}
	}

#ifdef USE_TRANSACTIONS
	QSet<MessageDb *> usedDbs;
#endif /* USE_TRANSACTIONS */
	while (0 != box) {

		diff += delta;
		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, (int)(20 + diff));

		const isds_message *item = (isds_message *)box->data;

		if (NULL == item->envelope) {
			/* TODO - free allocated stuff */
			return DL_ISDS_ERROR;
		}

		/*
		 * Time may be invalid (e.g. messages which failed during
		 * virus scan).
		 */
		MessageDb::MsgId msgId(
		    QString(item->envelope->dmID).toLongLong(),
		    timevalToDateTime(item->envelope->dmDeliveryTime));

		/* Delivery time may be invalid. */
		if ((0 != invalidDb) && msgId.deliveryTime.isValid()) {
			/* Try deleting possible invalid entry. */
			invalidDb->msgsDeleteMessageData(msgId.dmId);
		}
		MessageDb *messageDb = dbSet.accessMessageDb(msgId.deliveryTime,
		    true);
		Q_ASSERT(0 != messageDb);

#ifdef USE_TRANSACTIONS
		if (!usedDbs.contains(messageDb)) {
			usedDbs.insert(messageDb);
			if (!messageDb->beginTransaction()) {
				logWarningNL(
				    "Cannot begin transaction for '%s'.",
				    userName.toUtf8().constData());
//				return DL_DB_INS_ERR;
			}
		}
#endif /* USE_TRANSACTIONS */

		const int dmDbMsgStatus = messageDb->msgsStatusIfExists(
		    msgId.dmId);

		/* Message is NOT in db (-1), -> insert */
		if (-1 == dmDbMsgStatus) {

			Task::storeEnvelope(msgDirect, dbSet, item->envelope);
			if (downloadWhole) {
				TaskDownloadMessage *task =
				    new (std::nothrow) TaskDownloadMessage(
				        userName, &dbSet, msgDirect, msgId,
				        true);
				if (task != Q_NULLPTR) {
					task->setAutoDelete(true);
					GlobInstcs::workPoolPtr->assignLo(task,
					    WorkerPool::PREPEND);
				}
			}
			newMsgIdList.append(msgId.dmId);
			newcnt++;

		/* Message is in db (dmDbMsgStatus <> -1), -> update */
		} else {

			/* Update message and envelope only if status has changed. */
			const int dmNewMsgStatus = IsdsConversion::msgStatusIsdsToDbRepr(
			     *item->envelope->dmMessageStatus);

			if (dmNewMsgStatus != dmDbMsgStatus) {
				/* Update envelope */
				Task::updateEnvelope(msgDirect, *messageDb,
				    item->envelope);

				/*
				 * Download whole message again if exists in db
				 * (and status has changed) or is required by
				 * downloadWhole in the settings.
				 */
				if (downloadWhole || messageDb->msgsStoredWhole(msgId.dmId)) {

					TaskDownloadMessage *task =
					    new (std::nothrow) TaskDownloadMessage(
					        userName, &dbSet, msgDirect,
					        msgId, true);
					if (task != Q_NULLPTR) {
						task->setAutoDelete(true);
						GlobInstcs::workPoolPtr->assignLo(
						    task, WorkerPool::PREPEND);
					}
				}

				/* Update delivery info of sent message */
				if (MSG_SENT == msgDirect) {
					downloadMessageState(msgDirect,
					    userName, msgId.dmId, true, dbSet,
					    error, longError);
				}
			}
		}

		box = box->next;

	}
#ifdef USE_TRANSACTIONS
	/* Commit on all opened databases. */
	foreach (MessageDb *db, usedDbs) {
		db->commitTransaction();
	}
#endif /* USE_TRANSACTIONS */

	isds_list_free(&messageList);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 100);

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

enum TaskDownloadMessageList::Result TaskDownloadMessageList::downloadMessageState(
    enum MessageDirection msgDirect, const QString &userName, qint64 dmId,
    bool signedMsg, MessageDbSet &dbSet, QString &error, QString &longError)
{
	debugFuncCall();

	enum TaskDownloadMessageList::Result res = DL_ERR;

	isds_error status = IE_ERROR;

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DL_ERR;
	}
	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(), &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		res = DL_ERR;
		goto fail;
		/*
		status = isds_get_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(), &message);
		*/
	}

	if (IE_SUCCESS != status) {
		error = isds_error(status);
		longError = isdsLongMessage(session);
		logErrorNL(
		    "Downloading message state returned status %d: '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
		res = DL_ISDS_ERROR;
		goto fail;
	}

	Q_ASSERT(NULL != message);

	res = updateMessageState(msgDirect, dbSet, message->envelope);
	if (DL_SUCCESS != res) {
		goto fail;
	}

	isds_message_free(&message);

	return DL_SUCCESS;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}

	return res;
}

enum TaskDownloadMessageList::Result TaskDownloadMessageList::updateMessageState(
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const struct isds_envelope *envel)
{
	if (NULL == envel) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	qint64 dmID = -1;
	{
		bool ok = false;
		dmID = QString(envel->dmID).toLongLong(&ok);
		if (!ok) {
			return DL_ERR;
		}
	}

	QDateTime deliveryTime;
	if (NULL != envel->dmDeliveryTime) {
		deliveryTime = timevalToDateTime(envel->dmDeliveryTime);
	}
	/* Delivery time does not have to be valid. */
	if (Q_UNLIKELY(!deliveryTime.isValid())) {
		logWarningNL(
		    "Updating state of message '%" PRId64 "' with invalid delivery time.",
		    dmID);
	}
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	if (Q_UNLIKELY(Q_NULLPTR == messageDb)) {
		logErrorNL("Cannot access message database for message '%" PRId64 "'.",
		    dmID);
		Q_ASSERT(0);
		return DL_ERR;
	}

	QString dmDeliveryTime;
	if (NULL != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime;
	if (NULL != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(envel->dmAcceptanceTime);
	}

	if (1 == messageDb->messageState(dmID)) {
		/*
		 * Update message envelope when the previous state
		 * is 1. This is because the envelope was generated by this
		 * application when sending a message and we must ensure that
		 * we get proper data from ISDS rather than storing potentially
		 * guessed values.
		 */
		Task::updateEnvelope(msgDirect, *messageDb, envel);
	} else if (messageDb->msgsUpdateMessageState(dmID,
	    dmDeliveryTime, dmAcceptanceTime,
	    envel->dmMessageStatus ?
	        IsdsConversion::msgStatusIsdsToDbRepr(*envel->dmMessageStatus) : 0)) {
		/* Updated message envelope delivery info in db. */
		logDebugLv0NL(
		    "Delivery information of message '%" PRId64 "' were updated.",
		    dmID);
	} else {
		logErrorNL(
		    "Updating delivery information of message '%" PRId64 "' failed.",
		    dmID);
	}

	const struct isds_list *event;
	event = envel->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    IsdsConversion::eventTypeToStr(*item->type) + QLatin1String(": "),
		    item->description);
		event = event->next;
	}

	return DL_SUCCESS;
}
