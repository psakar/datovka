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
#include <QThread>

#include "worker.h"
#include "src/common.h"
#include "src/crypto/crypto_funcs.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/gui/datovka.h"
#include "src/io/isds_sessions.h"


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

	bool success = true;

	/* Messages counters
	 * rt = receivedTotal, rn = receivedNews,
	 * st = sentTotal, sn = sentNews.
	 * Message counters are send to mainwindow and show in info-statusbar.
	*/
	int rt = 0;
	int rn = 0;
	int st = 0;
	int sn = 0;

	/* != -1 -- specifice message required. */
	if (0 <= job.msgId) {

		if (Q_SUCCESS == downloadMessage(job.acntTopIdx, job.msgId,
		        true, job.msgDirect, *job.msgDb,
		        "DownloadMessage", 0, this)) {
			/* Only on successful download. */
			emit refreshAttachmentList(job.acntTopIdx, job.msgId);
		} else {
			emit clearStatusBarAndShowDialog(job.msgId);
		}

	} else if (MSG_RECEIVED == job.msgDirect) {

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading received message list for account"
		    << job.acntTopIdx.data().toString();
		qDebug() << "-----------------------------------------------";
		if (Q_CONNECT_ERROR ==
		    downloadMessageList(job.acntTopIdx, MSG_RECEIVED,
		        *job.msgDb,
		        "GetListOfReceivedMessages", 0, this, rt, rn)) {
			success = false;
		}
		emit refreshAccountList(job.acntTopIdx);

		emit changeStatusBarInfo(true, rt, rn , st, sn);

		qDebug() << "-----------------------------------------------";
		if (success) {
			qDebug() << "All DONE!";
		} else {
			qDebug() << "An error occurred!";
		}

	} else if (MSG_SENT == job.msgDirect) {

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading sent message list for account"
		    << job.acntTopIdx.data().toString();
		qDebug() << "-----------------------------------------------";
		if (Q_CONNECT_ERROR ==
		    downloadMessageList(job.acntTopIdx, MSG_SENT, *job.msgDb,
		        "GetListOfSentMessages", 0, this, st, sn)) {
			success = false;
		}
		emit refreshAccountList(job.acntTopIdx);

		emit changeStatusBarInfo(true, rt, rn , st, sn);

		qDebug() << "-----------------------------------------------";
		if (success) {
			qDebug() << "All DONE!";
		} else {
			qDebug() << "An error occurred!";
		}

	}

	emit valueChanged("Idle", 0);

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
 * Store envelope into database.
 */
qdatovka_error Worker::storeEnvelope(enum MessageDirection msgDirect,
    MessageDb &messageDb, const struct isds_envelope *envel)
/* ========================================================================= */
{
	debugFuncCall();

	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmId = QString(envel->dmID).toLongLong();

	QString dmAmbiguousRecipient;
	if (NULL == envel->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = "0";
	} else {
		dmAmbiguousRecipient = QString::number(
		    *envel->dmAmbiguousRecipient);
	}

	QString dmLegalTitleYear;
	if (NULL != envel->dmLegalTitleYear) {
		dmLegalTitleYear = QString::number(
		    *envel->dmLegalTitleYear);
	}

	QString dmLegalTitleLaw;
	if (NULL != envel->dmLegalTitleLaw) {
		dmLegalTitleLaw = QString::number(
		    *envel->dmLegalTitleLaw);
	}

	QString dmSenderOrgUnitNum;
	if ((NULL != envel->dmSenderOrgUnitNum) &&
	    (0 != *envel->dmSenderOrgUnitNum)) {
		dmSenderOrgUnitNum = QString::number(
		    *envel->dmSenderOrgUnitNum);
	}

	QString dmRecipientOrgUnitNum;
	if ((NULL != envel->dmRecipientOrgUnitNum) &&
	    (0 != *envel->dmRecipientOrgUnitNum)) {
		dmRecipientOrgUnitNum = QString::number(
		    *envel->dmRecipientOrgUnitNum);
	}

	QString dmDeliveryTime;
	if (NULL != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime;
	if (NULL != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(envel->dmAcceptanceTime);
	}

	/* insert message envelope in db */
	if (messageDb.msgsInsertMessageEnvelope(dmId,
	    /* TODO - set correctly next two values */
	    "tRecord",
	    envel->dbIDSender,
	    envel->dmSender,
	    envel->dmSenderAddress,
	    (int) *envel->dmSenderType,
	    envel->dmRecipient,
	    envel->dmRecipientAddress,
	    dmAmbiguousRecipient,
	    envel->dmSenderOrgUnit,
	    dmSenderOrgUnitNum,
	    envel->dbIDRecipient,
	    envel->dmRecipientOrgUnit,
	    dmRecipientOrgUnitNum,
	    envel->dmToHands,
	    envel->dmAnnotation,
	    envel->dmRecipientRefNumber,
	    envel->dmSenderRefNumber,
	    envel->dmRecipientIdent,
	    envel->dmSenderIdent,
	    dmLegalTitleLaw,
	    dmLegalTitleYear,
	    envel->dmLegalTitleSect,
	    envel->dmLegalTitlePar,
	    envel->dmLegalTitlePoint,
	    envel->dmPersonalDelivery,
	    envel->dmAllowSubstDelivery,
	    (NULL != envel->timestamp) ?
	        QByteArray((char *) envel->timestamp,
	            envel->timestamp_length).toBase64() : QByteArray(),
	    dmDeliveryTime,
	    dmAcceptanceTime,
	    convertHexToDecIndex(*envel->dmMessageStatus),
	    (int) *envel->dmAttachmentSize,
	    envel->dmType,
	    msgDirect)) {
		qDebug() << "Message envelope" << dmId <<
		    "was inserted into db...";
		return Q_SUCCESS;
	} else {
		qDebug() << "ERROR: Message envelope " << dmId <<
		    "insert!";
		return Q_GLOBAL_ERROR;
	}
}


/* ========================================================================= */
/*
 * Download sent/received message list from ISDS for current account index
 */
qdatovka_error Worker::downloadMessageList(const QModelIndex &acntTopIdx,
    enum MessageDirection msgDirect, MessageDb &messageDb,
    const QString &progressLabel, QProgressBar *pBar, Worker *worker,
    int &total, int &news)
/* ========================================================================= */
{
#define USE_TRANSACTIONS 1
	debugFuncCall();

	int newcnt = 0;
	int allcnt = 0;

	if (!acntTopIdx.isValid()) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	if (0 != pBar) { pBar->setValue(0); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 0); }

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status = IE_ERROR;

	if (0 != pBar) { pBar->setValue(10); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 10); }

	struct isds_list *messageList = NULL;

	/* Download sent/received message list from ISDS for current account */
	if (MSG_SENT == msgDirect) {
		status = isds_get_list_of_sent_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_SENT |  MESSAGESTATE_STAMPED |
		    //MESSAGESTATE_INFECTED | MESSAGESTATE_DELIVERED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	} else if (MSG_RECEIVED == msgDirect) {
		status = isds_get_list_of_received_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_DELIVERED | MESSAGESTATE_SUBSTITUTED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	}

	if (0 != pBar) { pBar->setValue(20); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 20); }

	if (status != IE_SUCCESS) {
		qDebug() << status << isds_strerror(status) <<
		    isds_long_message(isdsSessions.session(
		         accountInfo.userName()));
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
		if (0 != pBar) { pBar->setValue(50); }
		if (0 != worker) { emit worker->valueChanged(progressLabel, 50); }
	} else {
		delta = 80.0 / allcnt;
	}

#ifdef USE_TRANSACTIONS
	messageDb.beginTransaction();
#endif /* USE_TRANSACTIONS */
	while (0 != box) {

		diff += delta;
		if (0 != pBar) { pBar->setValue((int) (20 + diff)); }
		if (0 != worker) { emit worker->valueChanged(progressLabel,
		    (int) (20 + diff)); }


		const isds_message *item = (isds_message *) box->data;

		if (NULL == item->envelope) {
			/* TODO - free allocated stuff */
			return Q_ISDS_ERROR;
		}

		qint64 dmId = QString(item->envelope->dmID).toLongLong();

		int dmDbMsgStatus = messageDb.msgsStatusIfExists(dmId);

		/* message is not in db (-1) */
		if (-1 == dmDbMsgStatus) {
			storeEnvelope(msgDirect, messageDb, item->envelope);

			if (globPref.auto_download_whole_messages) {
				downloadMessage(acntTopIdx, dmId,
				    true, msgDirect, messageDb, "", 0, 0);
			}
			newcnt++;

		/* Message is in db (dmDbMsgStatus <> -1). */
		} else {
			if (MSG_SENT == msgDirect) {
				int dmNewMsgStatus = convertHexToDecIndex(
				     *item->envelope->dmMessageStatus);

				/*
				 * Sent messages content will be downloaded
				 * only if those message state is 1 or 2.
				 */
				if (globPref.auto_download_whole_messages &&
				    (dmDbMsgStatus <= 2)) {
					downloadMessage(acntTopIdx, dmId,
					    true, msgDirect, messageDb,
					    "", 0, 0);
				}

				if (dmDbMsgStatus != dmNewMsgStatus) {
					getMessageState(msgDirect, acntTopIdx,
					    dmId, true, messageDb);
				}
			}

			/* Message is in db, but the content is missing. */
			if (globPref.auto_download_whole_messages &&
			    !messageDb.msgsStoredWhole(dmId)) {
				downloadMessage(acntTopIdx, dmId,
				    true, msgDirect, messageDb, "", 0, 0);
			}
		}

		box = box->next;

	}
#ifdef USE_TRANSACTIONS
	messageDb.commitTransaction();
#endif /* USE_TRANSACTIONS */

	isds_list_free(&messageList);

	if (0 != pBar) { pBar->setValue(100); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 100); }

	if (MSG_RECEIVED == msgDirect) {
		qDebug() << "#Received total:" << allcnt;
		qDebug() << "#Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt;
		qDebug() << "#Sent new:" << newcnt;
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
    MessageDb &messageDb, const struct isds_envelope *envel)
/* ========================================================================= */
{
	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmID = QString(envel->dmID).toLongLong();

	QString dmDeliveryTime;
	if (NULL != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime;
	if (NULL != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(envel->dmAcceptanceTime);
	}

	if (1 == messageDb.messageState(dmID)) {
		/*
		 * Update message envelope when the previous state
		 * is 1. This is because the envelope was generated by this
		 * application when sending a message and we must ensure that
		 * we get proper data from ISDS rather than storing potentially
		 * guessed values.
		 */
		updateEnvelope(msgDirect, messageDb, envel);
	} else if (messageDb.msgsUpdateMessageState(dmID,
	    dmDeliveryTime, dmAcceptanceTime,
	    convertHexToDecIndex(*envel->dmMessageStatus))) {
		/* Updated message envelope delivery info in db. */
		qDebug() << "Message envelope delivery info was updated...";
	} else {
		qDebug() << "ERROR: Message envelope delivery info update!";
	}

	const struct isds_list *event;
	event = envel->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb.msgsInsertUpdateMessageEvent(dmID,
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
    const QModelIndex &acntTopIdx, qint64 dmId, bool signedMsg,
    MessageDb &messageDb)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status = IE_ERROR;

	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(
		    isdsSessions.session(accountInfo.userName()),
		    QString::number(dmId).toUtf8().constData(), &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		goto fail;
		/*
		status = isds_get_delivery_info(
		    isdsSessions.session(accountInfo.userName()),
		    QString::number(dmId).toUtf8().constData(), &message);
		*/
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		goto fail;
	}

	Q_ASSERT(NULL != message);

	if (Q_SUCCESS !=
	    updateMessageState(msgDirect, messageDb, message->envelope)) {
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
 * Store message into database.
 */
qdatovka_error Worker::storeMessage(bool signedMsg,
    enum MessageDirection msgDirect,
    MessageDb &messageDb, const struct isds_message *msg,
    const QString &progressLabel, QProgressBar *pBar, Worker *worker)
/* ========================================================================= */
{
	debugFuncCall();

	if (!signedMsg) {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return Q_GLOBAL_ERROR;
	}

	if (NULL == msg) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	const struct isds_envelope *envel = msg->envelope;

	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmID = QString(envel->dmID).toLongLong();

	/* Get signed raw data from message and store to db. */
	if (signedMsg) {
		(messageDb.msgsInsertUpdateMessageRaw(dmID,
		    QByteArray((char*) msg->raw, msg->raw_length), 0))
		? qDebug() << "Message raw data were updated..."
		: qDebug() << "ERROR: Message raw data update!";
	}

	if (0 != pBar) { pBar->setValue(30); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 30); }

	if (updateEnvelope(msgDirect, messageDb, envel)) {
		qDebug() << "Message envelope was updated...";
	} else {
		qDebug() << "ERROR: Message envelope update!";
	}

	if (0 != pBar) { pBar->setValue(50); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 50); }

	if (signedMsg) {
		/* Verify message signature. */
		int ret = raw_msg_verify_signature(msg->raw,
		    msg->raw_length, 1, globPref.check_crl ? 1 : 0);
		qDebug() << "Verification ret" << ret;
		if (1 == ret) {
			messageDb.msgsSetVerified(dmID,
			    true);
			/* TODO -- handle return error. */
		} else if (0 == ret){
			messageDb.msgsSetVerified(dmID,
			    false);
			/* TODO -- handle return error. */
		} else {
			/* TODO -- handle this error. */
		}
	}

	if (0 != pBar) { pBar->setValue(60); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 60); }

	/* insert/update hash into db */
	if (NULL != envel->hash) {
		const struct isds_hash *hash = envel->hash;

		QByteArray hashValueBase64 = QByteArray((char *) hash->value,
		    hash->length).toBase64();
		if (messageDb.msgsInsertUpdateMessageHash(dmID,
		        hashValueBase64, convertHashAlg(hash->algorithm))) {
			qDebug() << "Message hash was stored into db...";
		} else {
			qDebug() << "ERROR: Message hash insert!";
		}
	}

	if (0 != pBar) { pBar->setValue(70); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 70); }

	/* Insert/update all attachment files */
	const struct isds_list *file = msg->documents;
	while (0 != file) {
		const isds_document *item = (isds_document *) file->data;

		QByteArray dmEncodedContentBase64 =
		    QByteArray((char *)item->data,
		        item->data_length).toBase64();

		/* Insert/update file to db */
		if (messageDb.msgsInsertUpdateMessageFile(dmID,
		   item->dmFileDescr, item->dmUpFileGuid, item->dmFileGuid,
		   item->dmMimeType, item->dmFormat,
		   convertAttachmentType(item->dmFileMetaType),
		   dmEncodedContentBase64)) {
			qDebug() << "Message file" << item->dmFileDescr
			    << "was stored into db...";
		} else {
			qDebug() << "ERROR: Message file" << item->dmFileDescr
			    << "insert!";
		}
		file = file->next;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
 * Download attachments, envelope and raw for message.
 */
qdatovka_error Worker::downloadMessage(const QModelIndex &acntTopIdx,
    qint64 dmId, bool signedMsg, enum MessageDirection msgDirect,
    MessageDb &messageDb, const QString &progressLabel, QProgressBar *pBar,
    Worker *worker)
/* ========================================================================= */
{
	debugFuncCall();

	qDebug() << "Downloading complete message" << dmId;

	if (0 != pBar) { pBar->setValue(0); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 0); }

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	// message structures - all members
	struct isds_message *message = NULL;

	if (0 != pBar) { pBar->setValue(10); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 10); }

	/* download signed message? */
	if (signedMsg) {
		/* sent or received message? */
		if (MSG_RECEIVED == msgDirect) {
			status = isds_get_signed_received_message(
			    isdsSessions.session(accountInfo.userName()),
			    QString::number(dmId).toUtf8().constData(),
			    &message);
		} else {
			status = isds_get_signed_sent_message(
			    isdsSessions.session(accountInfo.userName()),
			    QString::number(dmId).toUtf8().constData(),
			    &message);
		}
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return Q_GLOBAL_ERROR;
		/*
		status = isds_get_received_message(isdsSessions.session(
		    accountInfo.userName()),
		    QString::number(dmId).toUtf8().constData(),
		    &message);
		*/
	}

	if (0 != pBar) { pBar->setValue(20); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 20); }

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return Q_ISDS_ERROR;
	}

	/* Download and store the message. */
	storeMessage(signedMsg, msgDirect, messageDb, message,
	    progressLabel, pBar, worker);

	if (0 != pBar) { pBar->setValue(90); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 90); }

	Q_ASSERT(dmId == QString(message->envelope->dmID).toLongLong());

	/* Download and save delivery info and message events */
	if (getDeliveryInfo(acntTopIdx, dmId, signedMsg, messageDb)) {
		qDebug() << "Delivery info of message was processed...";
	} else {
		qDebug() << "ERROR: Delivery info of message not found!";
	}

	getMessageAuthor(acntTopIdx, dmId, messageDb);

	if (MSG_RECEIVED == msgDirect) {
		/*  Mark this message as downloaded in ISDS */
		if (markMessageAsDownloaded(acntTopIdx, dmId)) {
			qDebug() << "Message was marked as downloaded...";
		} else {
			qDebug() << "ERROR: Message was not marked as downloaded!";
		}
	}

	if (0 != pBar) { pBar->setValue(100); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 100); }

	isds_list_free(&message->documents);
	isds_message_free(&message);

	qDebug() << "downloadMessage(): Done!";

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
 * Store received message delivery information into database.
 */
qdatovka_error Worker::storeDeliveryInfo(bool signedMsg,
    MessageDb &messageDb, const struct isds_message *msg)
/* ========================================================================= */
{
	if (NULL == msg) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	const struct isds_envelope *envel = msg->envelope;

	if (NULL == envel) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}

	qint64 dmID = QString(envel->dmID).toLongLong();

	/* get signed raw data from message */
	if (signedMsg) {
		if (messageDb.msgsInsertUpdateDeliveryInfoRaw(dmID,
		    QByteArray((char*)msg->raw, msg->raw_length))) {
			qDebug() << "Message raw delivery info was updated...";
		} else {
			qDebug() << "ERROR: Message raw delivery info update!";
		}
	}

	const struct isds_list *event;
	event = envel->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb.msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
* Download message delivery info, raw and get list of events message
*/
bool Worker::getDeliveryInfo(const QModelIndex &acntTopIdx,
    qint64 dmId, bool signedMsg, MessageDb &messageDb)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(isdsSessions.session(
		    accountInfo.userName()),
		    QString::number(dmId).toUtf8().constData(),
		    &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		goto fail;
		/*
		status = isds_get_delivery_info(isdsSessions.session(
		    accountInfo.userName()),
		    QString::number(dmId).toUtf8().constData(),
		    &message);
		*/
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		goto fail;
	}

	Q_ASSERT(NULL != message);

	if (Q_SUCCESS != storeDeliveryInfo(signedMsg, messageDb, message)) {
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
* Get additional info about author (sender)
*/
bool Worker::getMessageAuthor(const QModelIndex &acntTopIdx,
    qint64 dmId, MessageDb &messageDb)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	isds_sender_type *sender_type = NULL;
	char * raw_sender_type = NULL;
	char * sender_name = NULL;

	status = isds_get_message_sender(isdsSessions.session(
	    accountInfo.userName()),
	    QString::number(dmId).toUtf8().constData(),
	    &sender_type, &raw_sender_type, &sender_name);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	if (messageDb.updateMessageAuthorInfo(dmId,
	        convertSenderTypeToString((int) *sender_type), sender_name)) {
		qDebug() << "Author info of message was added...";
	} else {
		qDebug() << "ERROR: Author info of message wrong!";
	}

	return true;
}


/* ========================================================================= */
/*
* Set message as downloaded from ISDS.
*/
bool Worker::markMessageAsDownloaded(const QModelIndex &acntTopIdx,
    qint64 dmId)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	status = isds_mark_message_read(isdsSessions.session(
	    accountInfo.userName()),
	    QString::number(dmId).toUtf8().constData());

	if (IE_SUCCESS != status) {
		return false;
	}
	return true;
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

	qint64 dmID = QString(envel->dmID).toLongLong();

	QString dmAmbiguousRecipient;
	if (NULL != envel->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = QString::number(
		    *envel->dmAmbiguousRecipient);
	}
	QString dmLegalTitleYear;
	if (NULL != envel->dmLegalTitleYear) {
		dmLegalTitleYear = QString::number(*envel->dmLegalTitleYear);
	}
	QString dmLegalTitleLaw;
	if (NULL != envel->dmLegalTitleLaw) {
		dmLegalTitleLaw = QString::number(*envel->dmLegalTitleLaw);
	}
	QString dmSenderOrgUnitNum;
	if ((NULL != envel->dmSenderOrgUnitNum) &&
	    (0 != *envel->dmSenderOrgUnitNum)) {
		dmSenderOrgUnitNum = QString::number(
		    *envel->dmSenderOrgUnitNum);
	}
	QString dmRecipientOrgUnitNum;
	if ((NULL != envel->dmRecipientOrgUnitNum) &&
	    (0 != *envel->dmRecipientOrgUnitNum)) {
		dmRecipientOrgUnitNum = QString::number(
		    *envel->dmRecipientOrgUnitNum);
	}
	QString dmDeliveryTime;
	if (NULL != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime;
	if (NULL != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(envel->dmAcceptanceTime);
	}

	/* Update message envelope in db. */
	if (messageDb.msgsUpdateMessageEnvelope(dmID,
	    /* TODO - set correctly next two values */
	    "tReturnedMessage",
	    envel->dbIDSender,
	    envel->dmSender,
	    envel->dmSenderAddress,
	    (int) *envel->dmSenderType,
	    envel->dmRecipient,
	    envel->dmRecipientAddress,
	    dmAmbiguousRecipient,
	    envel->dmSenderOrgUnit,
	    dmSenderOrgUnitNum,
	    envel->dbIDRecipient,
	    envel->dmRecipientOrgUnit,
	    dmRecipientOrgUnitNum,
	    envel->dmToHands,
	    envel->dmAnnotation,
	    envel->dmRecipientRefNumber,
	    envel->dmSenderRefNumber,
	    envel->dmRecipientIdent,
	    envel->dmSenderIdent,
	    dmLegalTitleLaw,
	    dmLegalTitleYear,
	    envel->dmLegalTitleSect,
	    envel->dmLegalTitlePar,
	    envel->dmLegalTitlePoint,
	    envel->dmPersonalDelivery,
	    envel->dmAllowSubstDelivery,
	    (NULL != envel->timestamp) ?
	        QByteArray((char *) envel->timestamp,
	            envel->timestamp_length).toBase64() : QByteArray(),
	    dmDeliveryTime,
	    dmAcceptanceTime,
	    convertHexToDecIndex(*envel->dmMessageStatus),
	    (NULL != envel->dmAttachmentSize) ?
	        (int) *envel->dmAttachmentSize : 0,
	    envel->dmType,
	    msgDirect)) {
		return Q_SUCCESS;
	} else {
		return Q_GLOBAL_ERROR;
	}
}
