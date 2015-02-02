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


#include <cassert>
#include <QDebug>
#include <QThread>

#include "worker.h"
#include "src/common.h"
#include "src/crypto/crypto_threadsafe.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "src/gui/datovka.h"
#include "src/io/isds_sessions.h"


QMutex Worker::downloadMessagesMutex(QMutex::NonRecursive);


/* ========================================================================= */
/*
 * Constructor for multiple accounts.
 */
Worker::Worker(QList<QModelIndex> acntTopIdxs, AccountDb &accountDb,
    QList<MessageDb *> messageDbList, QObject *parent)
/* ========================================================================= */
: m_acntTopIdxs(acntTopIdxs),
    m_accountDb(accountDb),
    m_messageDbList(messageDbList),
    m_dmId(),
    m_msgDirection(MSG_SENT) /* Any value will do. */
{
	/* unused */
	(void) parent;
}


/* ========================================================================= */
/*
 * Constructor for single account.
 */
Worker::Worker(QModelIndex acntTopIdx, AccountDb &accountDb,
    MessageDb *messageDb, QObject *parent)
/* ========================================================================= */
    : m_acntTopIdxs(),
    m_accountDb(accountDb),
    m_messageDbList(),
    m_dmId(),
    m_msgDirection(MSG_SENT) /* Any value will do. */
{
	/* unused */
	(void) parent;

	m_acntTopIdxs.append(acntTopIdx);
	m_messageDbList.append(messageDb);
}


/* ========================================================================= */
/*
 * Constructor for download complete message.
 */
Worker::Worker(QModelIndex acntTopIdx, AccountDb &accountDb,
    MessageDb *messageDb, QString dmId,
    enum MessageDirection msgDirection, QObject *parent)
/* ========================================================================= */
    : m_acntTopIdxs(),
    m_accountDb(accountDb),
    m_messageDbList(),
    m_dmId(dmId),
    m_msgDirection(msgDirection)
{
	/* unused */
	(void) parent;

	m_acntTopIdxs.append(acntTopIdx);
	m_messageDbList.append(messageDb);
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
* Start background sync of all accounts
*/
void Worker::syncAllAccounts(void)
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	   << thread()->currentThreadId();

	bool success = true;
	MessageDb *messageDb;

	/* Messages counters
	 * rt = receivedTotal, rn = receivedNews,
	 * st = sentTotal, sn = sentNews,
	 * rcvTtl, rcvNews, sntTtl, sntNews are send to mainwindow and
	 * shown in info-statusbar.
	 */
	int rt = 0;
	int rn = 0;
	int st = 0;
	int sn = 0;
	int rcvTtl = 0;
	int rcvNews = 0;
	int sntTtl = 0;
	int sntNews = 0;

	for (int i = 0; i < m_acntTopIdxs.size(); i++) {
//	foreach (QModelIndex index, m_acntTopIdxs)

		QModelIndex index = m_acntTopIdxs[i];
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

		// if the account is not included to sync all, skip it.
		if (!accountInfo.syncWithAll()) {
			continue;
		}

		messageDb = m_messageDbList.at(i);

		emit changeStatusBarInfo(false, index.data().toString(),
		    0, 0 ,0, 0);

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading message list for account"
		    << index.data().toString();
		qDebug() << "-----------------------------------------------";

		if (Q_CONNECT_ERROR ==
		    downloadMessageList(index,"received", *messageDb,
		    "GetListOfReceivedMessages", 0, this, rt, rn)) {
			success = false;
			continue;
		}

		rcvTtl += rt;
		rcvNews += rn;

		emit refreshAccountList(index);

		if (Q_CONNECT_ERROR ==
		    downloadMessageList(index,"sent", *messageDb,
		    "GetListOfSentMessages", 0, this, st, sn)) {
			success = false;
			continue;
		}

		sntTtl += st;
		sntNews += sn;

		emit refreshAccountList(index);

		if (!getPasswordInfo(index)) {
			success = false;
		}
	}

	emit valueChanged("Idle", 0);

	qDebug() << "-----------------------------------------------";
	success ? qDebug() << "All DONE!" : qDebug() << "An error occurred!";

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	emit changeStatusBarInfo(true, QString(), rcvTtl, rcvNews,
	    sntTtl, sntNews);

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
* Download MessageList for account
*/
void Worker::syncOneAccount(void)
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	    << thread()->currentThreadId();

	/* test account index valid */
	if (!m_acntTopIdxs[0].isValid()) {
		qDebug() << "Invalid Account index! Downloading is canceled.";
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

	qDebug() << "-----------------------------------------------";
	qDebug() << "Downloading message list for account"
	    << m_acntTopIdxs[0].data().toString();
	qDebug() << "-----------------------------------------------";

	if (Q_CONNECT_ERROR ==
	    downloadMessageList(m_acntTopIdxs[0],"received", *m_messageDbList[0],
	    "GetListOfReceivedMessages", 0, this, rt, rn)) {
		success = false;
	}

	emit refreshAccountList(m_acntTopIdxs[0]);

	if (Q_CONNECT_ERROR ==
	    downloadMessageList(m_acntTopIdxs[0],"sent", *m_messageDbList[0],
	    "GetListOfSentMessages", 0, this, st, sn)) {
		success = false;
	}

	emit refreshAccountList(m_acntTopIdxs[0]);

	if (!getPasswordInfo(m_acntTopIdxs[0])) {
		success = false;
	}

	emit valueChanged("Idle", 0);

	qDebug() << "-----------------------------------------------";
	success ? qDebug() << "All DONE!" : qDebug() << "An error occurred!";

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	emit changeStatusBarInfo(true, m_acntTopIdxs[0].data().toString(),
	    rt, rn , st, sn);

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
* Download complete message in separated thread
*/
void Worker::downloadCompleteMessage(void)
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	    << thread()->currentThreadId();

	/* test message ID valid */
	if (m_dmId.isNull() || m_dmId.isEmpty()) {
		qDebug() << "Invalid message ID! Downloading is canceled.";
		downloadMessagesMutex.unlock();
		emit finished();
		return;
	}

	MessageDb *messageDb;
	messageDb = m_messageDbList.at(0);

	/* sent message */
	if (Q_SUCCESS == downloadMessage(m_acntTopIdxs[0], m_dmId, true,
	        MSG_RECEIVED == m_msgDirection, *messageDb,
	        "DownloadMessage", 0, this)) {
		/* Only on successful download. */
		emit refreshAttachmentList(m_acntTopIdxs[0], m_dmId);
	} else {
		emit clearStatusBarAndShowDialog(m_dmId);
	}

	emit valueChanged("Idle", 0);

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*!
 * @brief Store envelope into database.
 */
qdatovka_error Worker::storeEnvelope(const QString &messageType,
    MessageDb &messageDb, const struct isds_envelope *envel)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(NULL != envel);
	if (NULL == envel) {
		return Q_GLOBAL_ERROR;
	}

	int dmId = atoi(envel->dmID);

	QString dmAmbiguousRecipient;
	if (0 == envel->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = "0";
	} else {
		dmAmbiguousRecipient = QString::number(
		    *envel->dmAmbiguousRecipient);
	}

	QString dmLegalTitleYear;
	if (0 == envel->dmLegalTitleYear) {
		dmLegalTitleYear = "";
	} else {
		dmLegalTitleYear = QString::number(
		    *envel->dmLegalTitleYear);
	}

	QString dmLegalTitleLaw;
	if (0 == envel->dmLegalTitleLaw) {
		dmLegalTitleLaw = "";
	} else {
		dmLegalTitleLaw = QString::number(
		    *envel->dmLegalTitleLaw);
	}

	QString dmSenderOrgUnitNum;
	if (0 == envel->dmSenderOrgUnitNum) {
		dmSenderOrgUnitNum = "";
	} else {
		dmSenderOrgUnitNum =
		    *envel->dmSenderOrgUnitNum != 0 ?
		    QString::number(*envel->
		    dmSenderOrgUnitNum) : "";
	}

	QString dmRecipientOrgUnitNum;
	if (0 == envel->dmRecipientOrgUnitNum) {
		dmRecipientOrgUnitNum = "";
	} else {
		dmRecipientOrgUnitNum =
		    *envel->dmRecipientOrgUnitNum != 0
		    ? QString::number(*envel->
		    dmRecipientOrgUnitNum) : "";
	}

	QString dmDeliveryTime = "";
	if (0 != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(
		    envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime = "";
	if (0 != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(
		    envel->dmAcceptanceTime);
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
	    messageType)) {
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
    const QString &messageType, MessageDb &messageDb,
    const QString &progressLabel, QProgressBar *pBar, Worker *worker,
    int &total, int &news)
/* ========================================================================= */
{
#define USE_TRANSACTIONS 1
	debugFuncCall();

	int newcnt = 0;
	int allcnt = 0;

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
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
	if (messageType == "sent") {
		status = isds_get_list_of_sent_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_SENT |  MESSAGESTATE_STAMPED |
		    //MESSAGESTATE_INFECTED | MESSAGESTATE_DELIVERED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	} else if (messageType == "received") {
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
		qDebug() << status << isds_strerror(status);
		isds_list_free(&messageList);
		return Q_ISDS_ERROR;
	}

	struct isds_list *box;
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


		isds_message *item = (isds_message *) box->data;

		if (NULL == item->envelope) {
			/* TODO - free allocated stuff */
			return Q_ISDS_ERROR;
		}

		int dmId = atoi(item->envelope->dmID);

		int dmDbMsgStatus = messageDb.msgsStatusIfExists(dmId);

		/* message is not in db (-1) */
		if (-1 == dmDbMsgStatus) {
			storeEnvelope(messageType, messageDb, item->envelope);

			if (globPref.auto_download_whole_messages) {
				downloadMessage(acntTopIdx, item->envelope->dmID,
				    true, "received" == messageType, messageDb,
				    "", 0, 0);
			}
			newcnt++;

		/* message is in db (dmDbMsgStatus <> -1) */
		} else {
			if (messageType == "sent") {
				int dmNewMsgStatus = convertHexToDecIndex(
				     *item->envelope->dmMessageStatus);

				if (dmDbMsgStatus != dmNewMsgStatus) {

					QString dmAcceptanceTime = "";
					if (0 != item->envelope->dmAcceptanceTime) {
						dmAcceptanceTime = timevalToDbFormat(
						item->envelope->dmAcceptanceTime);
					}
					getMessageState(acntTopIdx,
					    dmId, true, messageDb);
				}
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

	if (messageType == "received") {
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
qdatovka_error Worker::updateMessageState(bool signedMsg,
    MessageDb &messageDb, const struct isds_message *msg)
/* ========================================================================= */
{
	Q_ASSERT(NULL != msg);
	if (NULL == msg) {
		return Q_GLOBAL_ERROR;
	}

	const struct isds_envelope *envel = msg->envelope;

	Q_ASSERT(NULL != envel);
	if (NULL == envel) {
		return Q_GLOBAL_ERROR;
	}

	if (signedMsg) {
		/* TODO - if signedMsg then decode signed message (raw ) */
	}

	int dmID = atoi(envel->dmID);


	QString dmDeliveryTime = "";
	if (0 != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime = "";
	if (0 != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(envel->dmAcceptanceTime);
	}

	/* Update message envelope delivery info in db. */
	(messageDb.msgsUpdateMessageState(dmID,
	    dmDeliveryTime, dmAcceptanceTime,
	    convertHexToDecIndex(*envel->dmMessageStatus)))
	    ? qDebug() << "Message envelope delivery info was updated..."
	    : qDebug() << "ERROR: Message envelope delivery info update!";

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
bool Worker::getMessageState(const QModelIndex &acntTopIdx,
    int msgIdx, bool signedMsg, MessageDb &messageDb)
/* ========================================================================= */
{
	debugFuncCall();

	QString dmId = QString::number(msgIdx);
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status = IE_ERROR;

	// message and envelope structures
	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(isdsSessions.session(
		    accountInfo.userName()), dmId.toStdString().c_str(),
		    &message);
	} else {
		assert(0); /* Only signed messages can be downloaded. */
		return false;
		/*
		status = isds_get_delivery_info(isdsSessions.session(
		    accountInfo.userName()), dmId.toStdString().c_str(),
		    &message);
		*/
	}

	if (IE_SUCCESS != status) {
		isds_message_free(&message);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	updateMessageState(signedMsg, messageDb, message);

	isds_list_free(&message->envelope->events);
	isds_message_free(&message);

	return true;
}


/* ========================================================================= */
/*
 * Get password expiration info for account index
 */
bool Worker::getPasswordInfo(const QModelIndex &acntTopIdx)
/* ========================================================================= */
{
	debugFuncCall();

	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;
	bool retval;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString key = accountInfo.userName() + "___True";

	if (accountInfo.loginMethod() != LIM_USERNAME &&
	    accountInfo.loginMethod() != LIM_USER_CERT) {
		expirDate = "";
		m_accountDb.setPwdExpirIntoDb(key, expirDate);
		return true;
	} else {
		retval = false;
		status = isds_get_password_expiration(
		    isdsSessions.session(accountInfo.userName()), &expiration);

		if (IE_SUCCESS == status) {
			if (NULL != expiration) {
				expirDate = timevalToDbFormat(expiration);
				m_accountDb.setPwdExpirIntoDb(key, expirDate);
				retval = true;
			}
		}
		if (NULL != expiration) {
			free(expiration);
		}
		return retval;
	}
	return false;
}


/* ========================================================================= */
/*
 * Store message into database.
 */
qdatovka_error Worker::storeMessage(bool signedMsg, bool incoming,
    MessageDb &messageDb, const struct isds_message *msg,
    const QString &progressLabel, QProgressBar *pBar, Worker *worker)
/* ========================================================================= */
{
	debugFuncCall();

	if (!signedMsg) {
		assert(0); /* Only signed messages can be downloaded. */
		return Q_GLOBAL_ERROR;
	}

	Q_ASSERT(NULL != msg);
	if (NULL == msg) {
		return Q_GLOBAL_ERROR;
	}

	const struct isds_envelope *envel = msg->envelope;

	Q_ASSERT(NULL != envel);
	if (NULL == envel) {
		return Q_GLOBAL_ERROR;
	}

	int dmID = atoi(envel->dmID);

	/* Get signed raw data from message and store to db. */
	if (signedMsg) {
		(messageDb.msgsInsertUpdateMessageRaw(dmID,
		    QByteArray((char*) msg->raw, msg->raw_length), 0))
		? qDebug() << "Message raw data were updated..."
		: qDebug() << "ERROR: Message raw data update!";
	}

	if (0 != pBar) { pBar->setValue(30); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 30); }

	QString dmAmbiguousRecipient;
	if (0 == envel->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = "0";
	} else {
		dmAmbiguousRecipient = QString::number(
		    *envel->dmAmbiguousRecipient);
	}
	QString dmLegalTitleYear;
	if (0 == envel->dmLegalTitleYear) {
		dmLegalTitleYear = "";
	} else {
		dmLegalTitleYear = QString::number(
		    *envel->dmLegalTitleYear);
	}
	QString dmLegalTitleLaw;
	if (0 == envel->dmLegalTitleLaw) {
		dmLegalTitleLaw = "";
	} else {
		dmLegalTitleLaw = QString::number(
		    *envel->dmLegalTitleLaw);
	}
	QString dmSenderOrgUnitNum;
	if (0 == envel->dmSenderOrgUnitNum) {
		dmSenderOrgUnitNum = "";
	} else {
		dmSenderOrgUnitNum =
		    *envel->dmSenderOrgUnitNum != 0
		    ? QString::number(*envel->
		    dmSenderOrgUnitNum) : "";
	}
	QString dmRecipientOrgUnitNum;
	if (0 == envel->dmRecipientOrgUnitNum) {
		dmRecipientOrgUnitNum = "";
	} else {
		dmRecipientOrgUnitNum =
		  *envel->dmRecipientOrgUnitNum != 0
		    ? QString::number(*envel->
		    dmRecipientOrgUnitNum) : "";
	}
	QString dmDeliveryTime = "";
	if (0 != envel->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(
		    envel->dmDeliveryTime);
	}
	QString dmAcceptanceTime = "";
	if (0 != envel->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(
		    envel->dmAcceptanceTime);
	}

	if (0 != pBar) { pBar->setValue(40); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 40); }

	/* Update message envelope in db. */
	(messageDb.msgsUpdateMessageEnvelope(dmID,
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
	    (int) *envel->dmAttachmentSize,
	    envel->dmType,
	    (incoming) ? "received" : "sent"))
	    ? qDebug() << "Message envelope was updated..."
	    : qDebug() << "ERROR: Message envelope update!";

	if (0 != pBar) { pBar->setValue(50); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 50); }

	if (signedMsg) {
		/* Verify message signature. */
		int ret = rawMsgVerifySignature(msg->raw,
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
	struct isds_list *file;
	file = msg->documents;
	while (0 != file) {
		isds_document *item = (isds_document *) file->data;

		QByteArray dmEncodedContentBase64 =
		    QByteArray((char *)item->data,
		        item->data_length).toBase64();

		/* Insert/update file to db */
		(messageDb.msgsInsertUpdateMessageFile(dmID,
		   item->dmFileDescr, item->dmUpFileGuid, item->dmFileGuid,
		   item->dmMimeType, item->dmFormat,
		   convertAttachmentType(item->dmFileMetaType),
		   dmEncodedContentBase64))
		? qDebug() << "Message file" << item->dmFileDescr <<
		    "was stored into db..."
		: qDebug() << "ERROR: Message file" << item->dmFileDescr <<
		    "insert!";
		file = file->next;
	}

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
 * Download attachments, envelope and raw for message.
 */
qdatovka_error Worker::downloadMessage(const QModelIndex &acntTopIdx,
    const QString &dmId, bool signedMsg, bool incoming, MessageDb &messageDb,
    const QString &progressLabel, QProgressBar *pBar, Worker *worker)
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
		if (incoming) {
			status = isds_get_signed_received_message(
			    isdsSessions.session(accountInfo.userName()),
			    dmId.toStdString().c_str(),
			    &message);
		} else {
			status = isds_get_signed_sent_message(
			    isdsSessions.session(accountInfo.userName()),
			    dmId.toStdString().c_str(),
			    &message);
		}
	} else {
		assert(0); /* Only signed messages can be downloaded. */
		return Q_GLOBAL_ERROR;
		/*
		status = isds_get_received_message(isdsSessions.session(
		    accountInfo.userName()),
		    dmId.toStdString().c_str(),
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
	storeMessage(signedMsg, incoming, messageDb, message,
	    progressLabel, pBar, worker);

	if (0 != pBar) { pBar->setValue(90); }
	if (0 != worker) { emit worker->valueChanged(progressLabel, 90); }

	/* Download and save delivery info and message events */
	(getDeliveryInfo(acntTopIdx, message->envelope->dmID,
	    signedMsg, messageDb))
	? qDebug() << "Delivery info of message was processed..."
	: qDebug() << "ERROR: Delivery info of message not found!";

	getMessageAuthor(acntTopIdx, message->envelope->dmID, messageDb);

	if (incoming) {
		/*  Mark this message as downloaded in ISDS */
		(markMessageAsDownloaded(acntTopIdx, message->envelope->dmID))
		? qDebug() << "Message was marked as downloaded..."
		: qDebug() << "ERROR: Message was not marked as downloaded!";
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
	Q_ASSERT(NULL != msg);
	if (NULL == msg) {
		return Q_GLOBAL_ERROR;
	}

	const struct isds_envelope *envel = msg->envelope;

	Q_ASSERT(NULL != envel);
	if (NULL == envel) {
		return Q_GLOBAL_ERROR;
	}

	int dmID = atoi(envel->dmID);

	/* get signed raw data from message */
	if (signedMsg) {
		(messageDb.msgsInsertUpdateDeliveryInfoRaw(dmID,
		    QByteArray((char*)msg->raw, msg->raw_length)))
		? qDebug() << "Message raw delivery info was updated..."
		: qDebug() << "ERROR: Message raw delivery info update!";
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
    const QString &dmId, bool signedMsg, MessageDb &messageDb)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(isdsSessions.session(
		    accountInfo.userName()), dmId.toStdString().c_str(),
		    &message);
	} else {
		assert(0); /* Only signed messages can be downloaded. */
		return false;
		/*
		status = isds_get_delivery_info(isdsSessions.session(
		    accountInfo.userName()), dmId.toStdString().c_str(),
		    &message);
		*/
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return false;
	}

	storeDeliveryInfo(signedMsg, messageDb, message);

	isds_list_free(&message->envelope->events);
	isds_message_free(&message);

	return true;
}


/* ========================================================================= */
/*
* Get additional info about author (sender)
*/
bool Worker::getMessageAuthor(const QModelIndex &acntTopIdx,
    const QString &dmId, MessageDb &messageDb)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	isds_sender_type *sender_type = NULL;
	char * raw_sender_type = NULL;
	char * sender_name = NULL;

	status = isds_get_message_sender(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(),
	    &sender_type, &raw_sender_type, &sender_name);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		return false;
	}

	int dmID = atoi(dmId.toStdString().c_str());

	if (messageDb.updateMessageAuthorInfo(dmID,
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
    const QString &dmId)
/* ========================================================================= */
{
	debugFuncCall();

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	status = isds_mark_message_read(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str());

	if (IE_SUCCESS != status) {
		return false;
	}
	return true;
}
