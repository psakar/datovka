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


#include <QDebug>
#include <QSet>
#include <QThread>

#include "worker.h"
#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/gui/datovka.h"
#include "src/io/isds_sessions.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_message_general.h"


/* ========================================================================= */
/*
 * Job list constructor.
 */
Worker::JobList::JobList(void)
/* ========================================================================= */
    : QList<Job>(),
    QMutex(QMutex::NonRecursive)
{
}


/* ========================================================================= */
/*
 * Job list destructor.
 */
Worker::JobList::~JobList(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Atomic append.
 */
void Worker::JobList::append(const Worker::Job &value)
/* ========================================================================= */
{
	QMutex::lock();
	QList<Job>::append(value);
	QMutex::unlock();
}


/* ========================================================================= */
/*
 * Atomic get first and pop.
 */
Worker::Job Worker::JobList::firstPop(bool pop)
/* ========================================================================= */
{
	Job value;

	QMutex::lock();
	if (!isEmpty()) {
		value = QList<Job>::first();
		if (pop) {
			QList<Job>::removeFirst();
		}
	}
	QMutex::unlock();

	return value;
}


/* ========================================================================= */
/*
 * Atomic prepend.
 */
void Worker::JobList::prepend(const Worker::Job &value)
/* ========================================================================= */
{
	QMutex::lock();
	QList<Job>::prepend(value);
	QMutex::unlock();
}


Worker::JobList Worker::jobList;
QMutex Worker::downloadMessagesMutex(QMutex::NonRecursive);


/* ========================================================================= */
/*
 * Constructor for job list usage.
 */
Worker::Worker(QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_job()
{
}


/* ========================================================================= */
/*
 * Constructor for single job usage.
 */
Worker::Worker(const Job &job, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_job(job)
{
}


/* ========================================================================= */
/*
* Tread executing prepare
*/
void Worker::requestWork(void)
/* ========================================================================= */
{
	downloadMessagesMutex.lock();

	qDebug() << "Request worker start from Thread " <<
	    thread()->currentThreadId();

	emit workRequested();
}


/* ========================================================================= */
/*
* Download MessageList for account
*/
void Worker::doJob(void)
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	    << thread()->currentThreadId();

	Job job = m_job;

	if (!job.isValid()) {
		/* Use job queue. */
		job = jobList.firstPop(true);
	}

	/* Test whether job is valid. */
	if (!job.isValid()) {
		qDebug() << "Invalid worker job! Downloading is cancelled.";
		downloadMessagesMutex.unlock();
		emit finished();
		return;
	}

	/* Messages counters
	 * rt = receivedTotal, rn = receivedNews,
	 * st = sentTotal, sn = sentNews.
	 * Message counters are send to mainwindow and show in info-statusbar.
	*/
	int rt = 0;
	int rn = 0;
	int st = 0;
	int sn = 0;
	QString errMsg;
	qdatovka_error res = Q_SUCCESS;
	unsigned long dmLimit = MESSAGE_LIST_LIMIT;

	/* != -1 -- specific message required. */
	if (0 <= job.mId.dmId) {

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading message" << job.mId.dmId << "for account"
		    << AccountModel::globAccounts[job.userName].accountName();
		qDebug() << "-----------------------------------------------";

		if (Q_SUCCESS == MessageTaskGeneral::downloadMessage(job.userName, job.mId,
		        true, job.msgDirect, *job.dbSet, errMsg,
		        "DownloadMessage")) {
			/* Only on successful download. */
			emit globMsgProcEmitter.downloadSuccess(job.userName, job.mId.dmId);
		} else {
			emit globMsgProcEmitter.downloadFail(job.userName,
			    job.mId.dmId, errMsg);
		}

	} else if (MSG_RECEIVED == job.msgDirect) {

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

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading received message list for account"
		    << AccountModel::globAccounts[job.userName].accountName();
		qDebug() << "-----------------------------------------------";
		QStringList newMsgIdList;
		res = downloadMessageList(job.userName, MSG_RECEIVED,
		        *job.dbSet, errMsg, "GetListOfReceivedMessages",
		        rt, rn, newMsgIdList, &dmLimit, MESSAGESTATE_ANY);
		emit globMsgProcEmitter.downloadSuccess(job.userName, -1);
		emit globMsgProcEmitter.downloadListSummary(true,
		    rt, rn , st, sn);

		if (Q_SUCCESS == res) {
			qDebug() << "All DONE!";
		} else {
			qDebug() << "An error occurred!";
			// -1 means list of received messages
			emit globMsgProcEmitter.downloadFail(job.userName, -1,
			    errMsg);
		}

	} else if (MSG_SENT == job.msgDirect) {

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading sent message list for account"
		    << AccountModel::globAccounts[job.userName].accountName();
		qDebug() << "-----------------------------------------------";

		QStringList newMsgIdList;
		res = downloadMessageList(job.userName, MSG_SENT, *job.dbSet,
		        errMsg, "GetListOfSentMessages", st, sn,
		        newMsgIdList, &dmLimit, MESSAGESTATE_ANY);
		emit globMsgProcEmitter.downloadSuccess(job.userName, -2);
		emit globMsgProcEmitter.downloadListSummary(true,
		    rt, rn , st, sn);

		if (Q_SUCCESS == res) {
			qDebug() << "All DONE!";
		} else {
			qDebug() << "An error occurred!";
			// -2 means list of sent messages
			emit globMsgProcEmitter.downloadFail(job.userName, -2,
			    errMsg);
		}
	}

	emit globMsgProcEmitter.progressChange("Idle", 0);

	qDebug() << "-----------------------------------------------";

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
 * Download sent/received message list from ISDS for current account index
 */
qdatovka_error Worker::downloadMessageList(const QString &userName,
    enum MessageDirection msgDirect, MessageDbSet &dbSet, QString &errMsg,
    const QString &progressLabel, int &total, int &news,
    QStringList &newMsgIdList, ulong *dmLimit, int dmStatusFilter)
/* ========================================================================= */
{
#define USE_TRANSACTIONS 1
	debugFuncCall();

	int newcnt = 0;
	int allcnt = 0;

	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	isds_error status = IE_ERROR;

	emit globMsgProcEmitter.progressChange(progressLabel, 10);

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}
	struct isds_list *messageList = NULL;

	/* Download sent/received message list from ISDS for current account */
	if (MSG_SENT == msgDirect) {
		status = isds_get_list_of_sent_messages(session,
		    NULL, NULL, NULL,
		    dmStatusFilter,
		    0, dmLimit, &messageList);
	} else if (MSG_RECEIVED == msgDirect) {
		status = isds_get_list_of_received_messages(session,
		    NULL, NULL, NULL,
		    dmStatusFilter,
		    0, dmLimit, &messageList);
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 20);

	if (status != IE_SUCCESS) {
		errMsg = isdsLongMessage(session);
		qDebug() << status << isds_strerror(status) << errMsg;
		isds_list_free(&messageList);
		return Q_ISDS_ERROR;
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
		emit globMsgProcEmitter.progressChange(progressLabel, 50);
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
		emit globMsgProcEmitter.progressChange(progressLabel,
		    (int) (20 + diff));


		const isds_message *item = (isds_message *) box->data;

		if (NULL == item->envelope) {
			/* TODO - free allocated stuff */
			return Q_ISDS_ERROR;
		}

		QString dmID = QString(item->envelope->dmID);
		qint64 dmId = QString(item->envelope->dmID).toLongLong();
		/*
		 * Time may be invalid (e.g. messages which failed during
		 * virus scan).
		 */
		QDateTime deliveryTime =
		    timevalToDateTime(item->envelope->dmDeliveryTime);
		/* Delivery time may be invalid. */
		if ((0 != invalidDb) && deliveryTime.isValid()) {
			/* Try deleting possible invalid entry. */
			invalidDb->msgsDeleteMessageData(dmId);
		}
		MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
		Q_ASSERT(0 != messageDb);

#ifdef USE_TRANSACTIONS
		if (!usedDbs.contains(messageDb)) {
			usedDbs.insert(messageDb);
			messageDb->beginTransaction();
		}
#endif /* USE_TRANSACTIONS */

		const int dmDbMsgStatus = messageDb->msgsStatusIfExists(dmId);

		/* message is not in db (-1) */
		if (-1 == dmDbMsgStatus) {
			MessageTaskGeneral::storeEnvelope(msgDirect, dbSet, item->envelope);

			if (globPref.auto_download_whole_messages) {
				QString errMsg;
				MessageTaskGeneral::downloadMessage(userName,
				    MessageDb::MsgId(dmId, deliveryTime),
				    true, msgDirect, dbSet, errMsg, "");
			}
			newMsgIdList.append(dmID);
			newcnt++;

		/* Message is in db (dmDbMsgStatus <> -1). */
		} else {
			/* Update envelope if message status has changed. */
			const int dmNewMsgStatus = convertHexToDecIndex(
			     *item->envelope->dmMessageStatus);

			if (dmNewMsgStatus != dmDbMsgStatus) {
				updateEnvelope(msgDirect, *messageDb, item->envelope);
			}

			if (MSG_SENT == msgDirect) {
				/*
				 * Sent messages content will be downloaded
				 * only if those message state is 1 or 2.
				 */
				if (globPref.auto_download_whole_messages &&
				    (dmDbMsgStatus <= 2)) {
					QString errMsg;
					MessageTaskGeneral::downloadMessage(userName,
					    MessageDb::MsgId(dmId, deliveryTime),
					    true, msgDirect, dbSet, errMsg,
					    "");
				}

				if (dmDbMsgStatus != dmNewMsgStatus) {
					getMessageState(msgDirect, userName,
					    dmId, true, dbSet);
				}
			}

			/* Message is in db, but the content is missing. */
			if (globPref.auto_download_whole_messages &&
			    !messageDb->msgsStoredWhole(dmId)) {
				QString errMsg;
				MessageTaskGeneral::downloadMessage(userName,
				    MessageDb::MsgId(dmId, deliveryTime),
				    true, msgDirect, dbSet, errMsg, "");
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

	emit globMsgProcEmitter.progressChange(progressLabel, 100);

	if (MSG_RECEIVED == msgDirect) {
		qDebug() << "#Received total:" << allcnt
		    << " #Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt << " #Sent new:" << newcnt;
	}

	total = allcnt;
	news =  newcnt;

	return Q_SUCCESS;
#undef USE_TRANSACTIONS
}


/* ========================================================================= */
/*
 * Store sent message delivery information into database.
 */
qdatovka_error Worker::updateMessageState(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const struct isds_envelope *envel)
/* ========================================================================= */
{
	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmID = -1;
	{
		bool ok = false;
		dmID = QString(envel->dmID).toLongLong(&ok);
		if (!ok) {
			return Q_GLOBAL_ERROR;
		}
	}
	QDateTime deliveryTime = timevalToDateTime(envel->dmDeliveryTime);
	Q_ASSERT(deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(0 != messageDb);

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
		updateEnvelope(msgDirect, *messageDb, envel);
	} else if (messageDb->msgsUpdateMessageState(dmID,
	    dmDeliveryTime, dmAcceptanceTime,
	    envel->dmMessageStatus ?
	        convertHexToDecIndex(*envel->dmMessageStatus) : 0)) {
		/* Updated message envelope delivery info in db. */
		qDebug() << "Message envelope delivery info was updated...";
	} else {
		qDebug() << "ERROR: Message envelope delivery info update!";
	}

	const struct isds_list *event;
	event = envel->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
* Download sent message delivery info and get list of events message
*/
bool Worker::getMessageState(enum MessageDirection msgDirect,
    const QString &userName, qint64 dmId, bool signedMsg,
    MessageDbSet &dbSet)
/* ========================================================================= */
{
	debugFuncCall();

	isds_error status = IE_ERROR;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}
	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(), &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		goto fail;
		/*
		status = isds_get_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(), &message);
		*/
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		goto fail;
	}

	Q_ASSERT(NULL != message);

	if (Q_SUCCESS !=
	    updateMessageState(msgDirect, dbSet, message->envelope)) {
		goto fail;
	}

	isds_message_free(&message);

	return true;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}

	return false;
}


/* ========================================================================= */
/*
 * Update message envelope.
 */
qdatovka_error Worker::updateEnvelope(enum MessageDirection msgDirect,
    MessageDb &messageDb, const struct isds_envelope *envel)
/* ========================================================================= */
{
	debugFuncCall();

	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmId = -1;
	{
		bool ok = false;
		dmId = QString(envel->dmID).toLongLong(&ok);
		if (!ok) {
			return Q_GLOBAL_ERROR;
		}
	}

	/* Update message envelope in db. */
	if (messageDb.msgsUpdateMessageEnvelope(dmId,
	    /* TODO - set correctly next two values */
	    "tReturnedMessage",
	    envel->dbIDSender,
	    envel->dmSender,
	    envel->dmSenderAddress,
	    envel->dmSenderType ?
	        (int) *envel->dmSenderType : 0,
	    envel->dmRecipient,
	    envel->dmRecipientAddress,
	    envel->dmAmbiguousRecipient ?
	        QString::number(*envel->dmAmbiguousRecipient) : QString(),
	    envel->dmSenderOrgUnit,
	    (envel->dmSenderOrgUnitNum && *envel->dmSenderOrgUnitNum) ?
	        QString::number(*envel->dmSenderOrgUnitNum) : QString(),
	    envel->dbIDRecipient,
	    envel->dmRecipientOrgUnit,
	    (envel->dmRecipientOrgUnitNum && *envel->dmRecipientOrgUnitNum) ?
	        QString::number(*envel->dmRecipientOrgUnitNum) : QString(),
	    envel->dmToHands,
	    envel->dmAnnotation,
	    envel->dmRecipientRefNumber,
	    envel->dmSenderRefNumber,
	    envel->dmRecipientIdent,
	    envel->dmSenderIdent,
	    envel->dmLegalTitleLaw ?
	        QString::number(*envel->dmLegalTitleLaw) : QString(),
	    envel->dmLegalTitleYear ?
	        QString::number(*envel->dmLegalTitleYear) : QString(),
	    envel->dmLegalTitleSect,
	    envel->dmLegalTitlePar,
	    envel->dmLegalTitlePoint,
	    envel->dmPersonalDelivery ?
	        *envel->dmPersonalDelivery : false,
	    envel->dmAllowSubstDelivery ?
	        *envel->dmAllowSubstDelivery : false,
	    envel->timestamp ?
	        QByteArray((char *) envel->timestamp,
	            envel->timestamp_length).toBase64() : QByteArray(),
	    envel->dmDeliveryTime ?
	        timevalToDbFormat(envel->dmDeliveryTime) : QString(),
	    envel->dmAcceptanceTime ?
	        timevalToDbFormat(envel->dmAcceptanceTime) : QString(),
	    envel->dmMessageStatus ?
	        convertHexToDecIndex(*envel->dmMessageStatus) : 0,
	    envel->dmAttachmentSize ?
	        (int) *envel->dmAttachmentSize : 0,
	    envel->dmType,
	    msgDirect)) {
		return Q_SUCCESS;
	} else {
		return Q_GLOBAL_ERROR;
	}
}
