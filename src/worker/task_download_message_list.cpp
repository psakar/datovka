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

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/worker/pool.h" /* List with whole messages. */
#include "src/global.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/message_conversion.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h" /* List with whole messages. */
#include "src/worker/task_download_message_list.h"

TaskDownloadMessageList::TaskDownloadMessageList(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect, bool downloadWhole,
    unsigned long dmLimit, Isds::Type::DmFiltStates dmStatusFltr)
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
	 * MFS_POSTED
	 * MFS_STAMPED
	 * MFS_DELIVERED
	 * MFS_ACCEPTED
	 * MFS_READ
	 * MFS_REMOVED
	 * MFS_IN_VAULT
	 * MFS_ANY
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

/*!
 * @brief Download message list.
 *
 * @param[out]    messageList Message list to be obtained.
 * @param[in]     msgDirect Sent or received.
 * @param[in,out] session Communication session.
 * @param[in]     dmStatusFilter Status filter.
 * @param[in,out] dmLimit Message list length limit.
 * @return Error code.
 */
static
Isds::Error getMessageList(QList<Isds::Message> &messageList,
    enum MessageDirection msgDirect, Isds::Session *session,
    Isds::Type::DmFiltStates dmStatusFilter, unsigned long int *dmLimit)
{
	Isds::Error err;

	if (Q_UNLIKELY(session == Q_NULLPTR)) {
		Q_ASSERT(0);
		err.setCode(Isds::Type::ERR_ERROR);
		return err;
	}

	messageList = QList<Isds::Message>();

	/* Download sent/received message list from ISDS for current account */
	if (MSG_SENT == msgDirect) {
		err = Isds::Service::getListOfSentMessages(session,
		    dmStatusFilter, 0, dmLimit, messageList);
	} else if (MSG_RECEIVED == msgDirect) {
		err = Isds::Service::getListOfReceivedMessages(session,
		    dmStatusFilter, 0, dmLimit, messageList);
	}

	return err;
}

enum TaskDownloadMessageList::Result TaskDownloadMessageList::downloadMessageList(
    const QString &userName, enum MessageDirection msgDirect,
    MessageDbSet &dbSet, bool downloadWhole, QString &error, QString &longError,
    const QString &progressLabel, int &total, int &news,
    QList<qint64> &newMsgIdList, ulong *dmLimit,
    Isds::Type::DmFiltStates dmStatusFilter)
{
	#define USE_TRANSACTIONS 1
	debugFuncCall();

	int newcnt = 0;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 0);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 10);

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	QList<Isds::Message> messageList;
	Isds::Error err = getMessageList(messageList, msgDirect, session,
	    dmStatusFilter, dmLimit);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 20);

	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading message list returned status %d: '%s' '%s'.",
		    err.code(), error.toUtf8().constData(),
		    longError.toUtf8().constData());
		return DL_ISDS_ERROR;
	}

	float delta = 0.0;
	float diff = 0.0;

	if (messageList.size() == 0) {
		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, 50);
	} else {
		delta = 80.0 / messageList.size();
	}

	/* Obtain invalid message database if has a separate file. */
	MessageDb *invalidDb = Q_NULLPTR;
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
	foreach (const Isds::Message &msg, messageList) {

		diff += delta;
		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    progressLabel, (int)(20 + diff));

		if (msg.envelope().isNull()) {
			/* TODO - free allocated stuff */
			return DL_ISDS_ERROR;
		}

		/*
		 * Time may be invalid (e.g. messages which failed during
		 * virus scan).
		 */
		MessageDb::MsgId msgId(msg.envelope().dmId(),
		    msg.envelope().dmDeliveryTime());

		/* Delivery time may be invalid. */
		if ((Q_NULLPTR != invalidDb) && msgId.deliveryTime.isValid()) {
			/* Try deleting possible invalid entry. */
			invalidDb->msgsDeleteMessageData(msgId.dmId);
		}
		MessageDb *messageDb = dbSet.accessMessageDb(msgId.deliveryTime,
		    true);
		Q_ASSERT(Q_NULLPTR != messageDb);

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

		const int dmDbMsgStatus = messageDb->getMessageStatus(
		    msgId.dmId);

		/* Message is NOT in db (-1), -> insert */
		if (-1 == dmDbMsgStatus) {

			const Isds::Envelope &env(msg.envelope());
			Task::storeMessageEnvelope(msgDirect, dbSet, env);

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
			const enum Isds::Type::DmState dmNewMsgStatus =
			    msg.envelope().dmMessageStatus();

			if (dmNewMsgStatus != dmDbMsgStatus) {
				/* Update envelope */
				const Isds::Envelope &env(msg.envelope());
				Task::updateMessageEnvelope(msgDirect,
				    *messageDb, env);

				/*
				 * Download whole message again if exists in db
				 * (and status has changed) or is required by
				 * downloadWhole in the settings.
				 */
				if (downloadWhole || messageDb->isCompleteMessageInDb(msgId.dmId)) {

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

	}
#ifdef USE_TRANSACTIONS
	/* Commit on all opened databases. */
	foreach (MessageDb *db, usedDbs) {
		db->commitTransaction();
	}
#endif /* USE_TRANSACTIONS */

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 100);

	if (MSG_RECEIVED == msgDirect) {
		logDebugLv0NL("#Received total: %d #Received new: %d",
		    messageList.size(), newcnt);
	} else {
		logDebugLv0NL("#Sent total: %d #Sent new: %d",
		    messageList.size(), newcnt);
	}

	total = messageList.size();
	news =  newcnt;

	return DL_SUCCESS;
#undef USE_TRANSACTIONS
}

enum TaskDownloadMessageList::Result TaskDownloadMessageList::downloadMessageState(
    enum MessageDirection msgDirect, const QString &userName, qint64 dmId,
    bool signedMsg, MessageDbSet &dbSet, QString &error, QString &longError)
{
	debugFuncCall();

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	enum TaskDownloadMessageList::Result res = DL_ERR;

	Isds::Error err;
	err.setCode(Isds::Type::ERR_ERROR);
	Isds::Message message;

	if (signedMsg) {
		err = Isds::Service::getSignedDeliveryInfo(session, dmId,
		    message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return DL_ERR;
	}

	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading message state returned status %d: '%s'.",
		    err.code(), error.toUtf8().constData());
		return DL_ISDS_ERROR;
	}

	Q_ASSERT(!message.isNull());

	res = updateMessageState(msgDirect, dbSet, message.envelope());
	if (DL_SUCCESS != res) {
		return res;
	}

	return DL_SUCCESS;
}

enum TaskDownloadMessageList::Result TaskDownloadMessageList::updateMessageState(
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const Isds::Envelope &envel)
{
	if (Q_UNLIKELY(envel.isNull())) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	qint64 dmId = envel.dmId();
	if (Q_UNLIKELY(dmId < 0)) {
		Q_ASSERT(0);
		return DL_ERR;
	}

	const QDateTime &deliveryTime(envel.dmDeliveryTime());
	/* Delivery time does not have to be valid. */
	if (Q_UNLIKELY(!deliveryTime.isValid())) {
		logWarningNL(
		    "Updating state of message '%" PRId64 "' with invalid delivery time.",
		    dmId);
	}
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	if (Q_UNLIKELY(Q_NULLPTR == messageDb)) {
		logErrorNL("Cannot access message database for message '%" PRId64 "'.",
		    dmId);
		Q_ASSERT(0);
		return DL_ERR;
	}

	QString dmDeliveryTime;
	if (!envel.dmDeliveryTime().isNull()) {
		dmDeliveryTime = qDateTimeToDbFormat(envel.dmDeliveryTime());
	}
	QString dmAcceptanceTime;
	if (!envel.dmAcceptanceTime().isNull()) {
		dmAcceptanceTime = qDateTimeToDbFormat(envel.dmAcceptanceTime());
	}

	if (1 == messageDb->getMessageStatus(dmId)) {
		/*
		 * Update message envelope when the previous state
		 * is 1. This is because the envelope was generated by this
		 * application when sending a message and we must ensure that
		 * we get proper data from ISDS rather than storing potentially
		 * guessed values.
		 */
		Task::updateMessageEnvelope(msgDirect, *messageDb, envel);

	} else if (messageDb->msgsUpdateMessageState(dmId,
	    dmDeliveryTime, dmAcceptanceTime,
	    (envel.dmMessageStatus() != Isds::Type::MS_NULL) ?
	        envel.dmMessageStatus() : 0)) {
		/* Updated message envelope delivery info in db. */
		logDebugLv0NL(
		    "Delivery information of message '%" PRId64 "' were updated.",
		    dmId);
	} else {
		logErrorNL(
		    "Updating delivery information of message '%" PRId64 "' failed.",
		    dmId);
	}

	const QList<Isds::Event> &events(envel.dmEvents());
	foreach (const Isds::Event &event, events) {
		messageDb->insertOrUpdateMessageEvent(dmId, event);
	}

	return DL_SUCCESS;
}
