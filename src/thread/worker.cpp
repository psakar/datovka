
#include <cmath> /* ceil(3) */
#include <QDebug>
#include <QThread>

#include "worker.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/pkcs7.h"
#include "src/log/log.h"
#include "src/gui/datovka.h"
#include "src/io/isds_sessions.h"


/* ========================================================================= */
/*!
 * @brief Construtor.
 */
Worker::Worker(QModelIndex acntTopIdx, QString dmId,
	    AccountDb &accountDb, AccountModel &accountModel, int count,
	    QList<MessageDb*> messageDbList,
	    QList<bool> downloadThisAccounts, QObject *parent = 0) :
	m_acntTopIdx(acntTopIdx),
	m_dmId(dmId),
	m_accountDb(accountDb),
	m_accountModel(accountModel),
	m_count(count),
	m_messageDbList(messageDbList),
	m_downloadThisAccounts(downloadThisAccounts)
/* ========================================================================= */
{	/* unused */
	(void) parent;
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

	for (int i = 0; i < m_count; i++) {

		if (!m_downloadThisAccounts.at(i)) {
			continue;
		}

		QModelIndex index = m_accountModel.index(i, 0);
		const AccountModel::SettingsMap accountInfo =
		    index.data(ROLE_ACNT_CONF_SETTINGS).toMap();

		// if the account is not included to sync all, skip it.
		if (!accountInfo[SYNC].toBool()) {
			continue;
		}

		QStandardItem *item = m_accountModel.itemFromIndex(index);
		QStandardItem *itemTop = AccountModel::itemTop(item);
		messageDb = m_messageDbList.at(i);

		qDebug() << "-----------------------------------------------";
		qDebug() << "Downloading message list for account"
		    << itemTop->text();
		qDebug() << "-----------------------------------------------";

		if (Q_CONNECT_ERROR ==
		    downloadMessageList(index,"received", *messageDb,
		    "GetListOfReceivedMessages", 0, this)) {
			success = false;
			continue;
		}
		emit refreshAccountList(index);

		if (Q_CONNECT_ERROR ==
		    downloadMessageList(index,"sent", *messageDb,
		    "GetListOfSentMessages", 0, this)) {
			success = false;
			continue;
		}

		emit refreshAccountList(index);

		if (!getListSentMessageStateChanges(index, *messageDb,
		    "GetMessageStateChanges")) {
			success = false;
			continue;
		}

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
	if (!m_acntTopIdx.isValid()) {
		qDebug() << "Invalid Account index! Downloading is canceled.";
		downloadMessagesMutex.unlock();
		emit finished();
	}

	bool success = true;
	MessageDb *messageDb;

	const AccountModel::SettingsMap accountInfo =
	    m_acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QStandardItem *item = m_accountModel.itemFromIndex(m_acntTopIdx);
	QStandardItem *itemTop = AccountModel::itemTop(item);
	messageDb = m_messageDbList.at(0);

	qDebug() << "-----------------------------------------------";
	qDebug() << "Downloading message list for account" << itemTop->text();
	qDebug() << "-----------------------------------------------";

	if (Q_CONNECT_ERROR ==
	    downloadMessageList(m_acntTopIdx,"received", *messageDb,
	    "GetListOfReceivedMessages", 0, this)) {
		success = false;
	}

	emit refreshAccountList(m_acntTopIdx);

	if (Q_CONNECT_ERROR ==
	    downloadMessageList(m_acntTopIdx,"sent", *messageDb,
	    "GetListOfSentMessages", 0, this)) {
		success = false;
	}

	emit refreshAccountList(m_acntTopIdx);

	if (!getListSentMessageStateChanges(m_acntTopIdx, *messageDb,
	    "GetMessageStateChanges")) {
		success = false;
	}

	emit refreshAccountList(m_acntTopIdx);

	if (!getPasswordInfo(m_acntTopIdx)) {
		success = false;
	}

	emit valueChanged("Idle", 0);

	qDebug() << "-----------------------------------------------";
	success ? qDebug() << "All DONE!" : qDebug() << "An error occurred!";

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
* Download complete message / will be used in future
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
	}

	MessageDb *messageDb;
	messageDb = m_messageDbList.at(0);

	/* sent message */
	downloadMessage(m_acntTopIdx, m_dmId, true, false, *messageDb);

	downloadMessagesMutex.unlock();

	emit finished();
}


/* ========================================================================= */
/*
* Download sent/received message list from ISDS for current account index
*/
qdatovka_error Worker::downloadMessageList(const QModelIndex &acntTopIdx,
    const QString messageType, MessageDb &messageDb, QString label,
    QProgressBar *pBar, Worker *worker)
/* ========================================================================= */
{
	debug_func_call();

	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	if (0 != pBar) { pBar->setValue(0); }
	if (0 != worker) { emit worker->valueChanged(label, 0); }

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status = IE_ERROR;

	if (0 != pBar) { pBar->setValue(10); }
	if (0 != worker) { emit worker->valueChanged(label, 10); }

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
	if (0 != worker) { emit worker->valueChanged(label, 20); }

	if (status != IE_SUCCESS) {
		qDebug() << status << isds_strerror(status);
		isds_list_free(&messageList);
		return Q_ISDS_ERROR;
	}

	struct isds_list *box;
	box = messageList;
	int newcnt = 0;
	int allcnt = 0;
	int delta = 0;
	int diff = 0;

	while (0 != box) {
		allcnt++;
		box = box->next;
	}

	box = messageList;

	if (allcnt == 0) {
		if (0 != pBar) { pBar->setValue(50); }
		if (0 != worker) { emit worker->valueChanged(label, 50); }
	} else {
		delta = ceil(80 / allcnt);
	}

	while (0 != box) {

		diff = diff + delta;
		if (0 != pBar) { pBar->setValue(20+diff); }
		if (0 != worker) { emit worker->valueChanged(label, 20+diff); }

		isds_message *item = (isds_message *) box->data;
		int dmId = atoi(item->envelope->dmID);

		if (!messageDb.isInMessageDb(dmId)) {

			QString dmAmbiguousRecipient;
			if (0 == item->envelope->dmAmbiguousRecipient) {
				dmAmbiguousRecipient = "0";
			} else {
				dmAmbiguousRecipient = QString::number(
				    *item->envelope->dmAmbiguousRecipient);
			}

			QString dmLegalTitleYear;
			if (0 == item->envelope->dmLegalTitleYear) {
				dmLegalTitleYear = "";
			} else {
				dmLegalTitleYear = QString::number(
				    *item->envelope->dmLegalTitleYear);
			}

			QString dmLegalTitleLaw;
			if (0 == item->envelope->dmLegalTitleLaw) {
				dmLegalTitleLaw = "";
			} else {
				dmLegalTitleLaw = QString::number(
				    *item->envelope->dmLegalTitleLaw);
			}

			QString dmSenderOrgUnitNum;
			if (0 == item->envelope->dmSenderOrgUnitNum) {
				dmSenderOrgUnitNum = "";
			} else {
				dmSenderOrgUnitNum =
				    *item->envelope->dmSenderOrgUnitNum != 0 ?
				    QString::number(*item->envelope->
				    dmSenderOrgUnitNum) : "";
			}

			QString dmRecipientOrgUnitNum;
			if (0 == item->envelope->dmRecipientOrgUnitNum) {
				dmRecipientOrgUnitNum = "";
			} else {
				dmRecipientOrgUnitNum =
				    *item->envelope->dmRecipientOrgUnitNum != 0
				    ? QString::number(*item->envelope->
				    dmRecipientOrgUnitNum) : "";
			}

			QString dmDeliveryTime = "";
			if (0 != item->envelope->dmDeliveryTime) {
				dmDeliveryTime = timevalToDbFormat(
				    item->envelope->dmDeliveryTime);
			}
			QString dmAcceptanceTime = "";
			if (0 != item->envelope->dmAcceptanceTime) {
				dmAcceptanceTime = timevalToDbFormat(
				    item->envelope->dmAcceptanceTime);
			}

			/* insert message envelope in db */
			(messageDb.msgsInsertMessageEnvelope(dmId,
			    /* TODO - set correctly next two values */
			    false, "tRecord",
			    item->envelope->dbIDSender,
			    item->envelope->dmSender,
			    item->envelope->dmSenderAddress,
			    (int)*item->envelope->dmSenderType,
			    item->envelope->dmRecipient,
			    item->envelope->dmRecipientAddress,
			    dmAmbiguousRecipient,
			    item->envelope->dmSenderOrgUnit,
			    dmSenderOrgUnitNum,
			    item->envelope->dbIDRecipient,
			    item->envelope->dmRecipientOrgUnit,
			    dmRecipientOrgUnitNum,
			    item->envelope->dmToHands,
			    item->envelope->dmAnnotation,
			    item->envelope->dmRecipientRefNumber,
			    item->envelope->dmSenderRefNumber,
			    item->envelope->dmRecipientIdent,
			    item->envelope->dmSenderIdent,
			    dmLegalTitleLaw,
			    dmLegalTitleYear,
			    item->envelope->dmLegalTitleSect,
			    item->envelope->dmLegalTitlePar,
			    item->envelope->dmLegalTitlePoint,
			    item->envelope->dmPersonalDelivery,
			    item->envelope->dmAllowSubstDelivery,
			    (char*)item->envelope->timestamp,
			    dmDeliveryTime,
			    dmAcceptanceTime,
			    convertHexToDecIndex(*item->envelope->dmMessageStatus),
			    (int)*item->envelope->dmAttachmentSize,
			    item->envelope->dmType,
			    messageType))
			? qDebug() << "Message envelope" << dmId <<
			    "was inserted into db..."
			: qDebug() << "ERROR: Message envelope " << dmId <<
			    "insert!";

			if (globPref.auto_download_whole_messages) {
				downloadMessage(acntTopIdx, item->envelope->dmID,
				    true, "received" == messageType, messageDb);
			}

			newcnt++;
		}
		box = box->next;

	}

	isds_list_free(&messageList);

	if (0 != pBar) { pBar->setValue(100); }
	if (0 != worker) { emit worker->valueChanged(label, 100); }

	if (messageType == "received") {
		qDebug() << "#Received total:" << allcnt;
		qDebug() << "#Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt;
		qDebug() << "#Sent new:" << newcnt;
	}
	return Q_SUCCESS;
}


/* ========================================================================= */
/*
* Get list of sent message state changes
*/
bool Worker::getListSentMessageStateChanges(const QModelIndex &acntTopIdx,
    MessageDb &messageDb, QString label)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	emit valueChanged(label, 0);
	isds_error status = IE_ERROR;

	emit valueChanged(label, 10);

	struct isds_list *stateList = NULL;

	status = isds_get_list_of_sent_message_state_changes(
	    isdsSessions.session(accountInfo.userName()),NULL,NULL, &stateList);

	if (IE_SUCCESS != status) {
		isds_list_free(&stateList);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	emit valueChanged(label, 20);

	struct isds_list *stateListFirst = NULL;
	stateListFirst = stateList;

	int allcnt = 0;

	while (0 != stateList) {
		allcnt++;
		stateList = stateList->next;
	}

	stateListFirst = stateList;

	emit valueChanged(label, 30);

	int delta = 0;
	int diff = 0;

	if (allcnt == 0) {
		emit valueChanged(label, 60);
	} else {
		delta = ceil(70 / allcnt);
	}

	while (0 != stateListFirst) {
		isds_message_status_change *item =
		    (isds_message_status_change *) stateListFirst->data;
		int dmId = atoi(item->dmID);
		diff = diff + delta;
		emit valueChanged(label, 30+diff);
		/* Download and save delivery info and message events */
		(getSentDeliveryInfo(acntTopIdx, dmId, true, messageDb))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		stateListFirst = stateListFirst->next;
	}

	isds_list_free(&stateList);

	emit valueChanged(label, 100);
//	regenerateAccountModelYears(acntTopIdx);

	return true;
}


/* ========================================================================= */
/*
* Download sent message delivery info and get list of events message
*/
bool Worker::getSentDeliveryInfo(const QModelIndex &acntTopIdx,
    int msgIdx, bool signedMsg, MessageDb &messageDb)
/* ========================================================================= */
{
	debug_func_call();

	QString dmId = QString::number(msgIdx);
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status = IE_ERROR;

	// message and envelope structures
	struct isds_message *message = NULL;

	(signedMsg)
	? status = isds_get_signed_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message)
	: status = isds_get_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message);

	if (IE_SUCCESS != status) {
		isds_message_free(&message);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	/* TODO - if signedMsg == true then decode signed message (raw ) */

	int dmID = atoi(message->envelope->dmID);

	struct isds_list *event;
	event = message->envelope->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb.msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

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
	debug_func_call();

	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;
	bool retval;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString key = accountInfo.userName() + "___True";

	if (accountInfo.loginMethod() != "username" &&
	    accountInfo.loginMethod() != "user_certificate") {
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
 * Download attachments, envelope and raw for message.
 */
qdatovka_error Worker::downloadMessage(const QModelIndex &acntTopIdx,
    const QString dmId, bool signedMsg, bool incoming, MessageDb &messageDb)
/* ========================================================================= */
{
	debug_func_call();

	qDebug() << "Downloading complete message" << dmId;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	// message structures - all members
	struct isds_message *message = NULL;

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
		status = isds_get_received_message(isdsSessions.session(
		    accountInfo.userName()),
		    dmId.toStdString().c_str(),
		    &message);
	}

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return Q_ISDS_ERROR;
	}

	int dmID = atoi(message->envelope->dmID);

	/* get signed raw data from message */
	if (signedMsg) {
		QString raw = QByteArray((char*)message->raw,
		    message->raw_length).toBase64();

		(messageDb.msgsInsertUpdateMessageRaw(dmID, raw, 0))
		? qDebug() << "Message raw data were updated..."
		: qDebug() << "ERROR: Message raw data update!";
	}

	QString timestamp = QByteArray((char *)message->envelope->timestamp,
	    message->envelope->timestamp_length).toBase64();
	QString dmAmbiguousRecipient;
	if (0 == message->envelope->dmAmbiguousRecipient) {
		dmAmbiguousRecipient = "0";
	} else {
		dmAmbiguousRecipient = QString::number(
		    *message->envelope->dmAmbiguousRecipient);
	}
	QString dmLegalTitleYear;
	if (0 == message->envelope->dmLegalTitleYear) {
		dmLegalTitleYear = "";
	} else {
		dmLegalTitleYear = QString::number(
		    *message->envelope->dmLegalTitleYear);
	}
	QString dmLegalTitleLaw;
	if (0 == message->envelope->dmLegalTitleLaw) {
		dmLegalTitleLaw = "";
	} else {
		dmLegalTitleLaw = QString::number(
		    *message->envelope->dmLegalTitleLaw);
	}
	QString dmSenderOrgUnitNum;
	if (0 == message->envelope->dmSenderOrgUnitNum) {
		dmSenderOrgUnitNum = "";
	} else {
		dmSenderOrgUnitNum =
		    *message->envelope->dmSenderOrgUnitNum != 0
		    ? QString::number(*message->envelope->
		    dmSenderOrgUnitNum) : "";
	}
	QString dmRecipientOrgUnitNum;
	if (0 == message->envelope->dmRecipientOrgUnitNum) {
		dmRecipientOrgUnitNum = "";
	} else {
		dmRecipientOrgUnitNum =
		  *message->envelope->dmRecipientOrgUnitNum != 0
		    ? QString::number(*message->envelope->
		    dmRecipientOrgUnitNum) : "";
	}
	QString dmDeliveryTime = "";
	if (0 != message->envelope->dmDeliveryTime) {
		dmDeliveryTime = timevalToDbFormat(
		    message->envelope->dmDeliveryTime);
	}
	QString dmAcceptanceTime = "";
	if (0 != message->envelope->dmAcceptanceTime) {
		dmAcceptanceTime = timevalToDbFormat(
		    message->envelope->dmAcceptanceTime);
	}

	/* update message envelope in db */
	(messageDb.msgsUpdateMessageEnvelope(dmID,
	    /* TODO - set correctly next two values */
	    false, "tReturnedMessage",
	    message->envelope->dbIDSender,
	    message->envelope->dmSender,
	    message->envelope->dmSenderAddress,
	    (int)*message->envelope->dmSenderType,
	    message->envelope->dmRecipient,
	    message->envelope->dmRecipientAddress,
	    dmAmbiguousRecipient,
	    message->envelope->dmSenderOrgUnit,
	    dmSenderOrgUnitNum,
	    message->envelope->dbIDRecipient,
	    message->envelope->dmRecipientOrgUnit,
	    dmRecipientOrgUnitNum,
	    message->envelope->dmToHands,
	    message->envelope->dmAnnotation,
	    message->envelope->dmRecipientRefNumber,
	    message->envelope->dmSenderRefNumber,
	    message->envelope->dmRecipientIdent,
	    message->envelope->dmSenderIdent,
	    dmLegalTitleLaw,
	    dmLegalTitleYear,
	    message->envelope->dmLegalTitleSect,
	    message->envelope->dmLegalTitlePar,
	    message->envelope->dmLegalTitlePoint,
	    message->envelope->dmPersonalDelivery,
	    message->envelope->dmAllowSubstDelivery,
	    timestamp,
	    dmDeliveryTime,
	    dmAcceptanceTime,
	    convertHexToDecIndex(*message->envelope->dmMessageStatus),
	    (int)*message->envelope->dmAttachmentSize,
	    message->envelope->dmType,
	    (incoming) ? "received" : "sent"))
	    ? qDebug() << "Message envelope was updated..."
	    : qDebug() << "ERROR: Message envelope update!";

	/* insert/update hash into db */
	QString hashValue = QByteArray((char*)message->envelope->hash->value,
	    message->envelope->hash->length).toBase64();
	(messageDb.msgsInsertUpdateMessageHash(dmID,
	    hashValue, convertHashAlg(message->envelope->hash->algorithm)))
	? qDebug() << "Message hash was stored into db..."
	: qDebug() << "ERROR: Message hash insert!";

	/* Insert/update all attachment files */
	struct isds_list *file;
	file = message->documents;
	while (0 != file) {
		isds_document *item = (isds_document *) file->data;

		QString dmEncodedContent = QByteArray((char *)item->data,
		    item->data_length).toBase64();

		/* Insert/update file to db */
		(messageDb.msgsInsertUpdateMessageFile(dmID,
		   item->dmFileDescr, item->dmUpFileGuid, item->dmFileGuid,
		   item->dmMimeType, item->dmFormat,
		   convertAttachmentType(item->dmFileMetaType),
		   dmEncodedContent))
		? qDebug() << "Message file" << item->dmFileDescr <<
		    "was stored into db..."
		: qDebug() << "ERROR: Message file" << item->dmFileDescr <<
		    "insert!";
		file = file->next;
	}

	if (incoming) {
		/* Download and save delivery info and message events */
		(getReceivedsDeliveryInfo(acntTopIdx, message->envelope->dmID,
		    signedMsg, messageDb))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		getMessageAuthor(acntTopIdx, message->envelope->dmID, messageDb);

		/*  Mark this message as downloaded in ISDS */
		(markMessageAsDownloaded(acntTopIdx, message->envelope->dmID))
		? qDebug() << "Message was marked as downloaded..."
		: qDebug() << "ERROR: Message was not marked as downloaded!";
	} else {
		/* Download and save delivery info and message events */
		(getSentDeliveryInfo(acntTopIdx, dmID, true, messageDb))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		getMessageAuthor(acntTopIdx, message->envelope->dmID, messageDb);
	}

	isds_list_free(&message->documents);
	isds_message_free(&message);

	qDebug() << "downloadMessage(): Done!";

	return Q_SUCCESS;
}


/* ========================================================================= */
/*
* Download message delivery info, raw and get list of events message
*/
bool Worker::getReceivedsDeliveryInfo(const QModelIndex &acntTopIdx,
    const QString dmId, bool signedMsg, MessageDb &messageDb)
/* ========================================================================= */
{

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	isds_error status;

	struct isds_message *message = NULL;

	(signedMsg)
	? status = isds_get_signed_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message)
	: status = isds_get_delivery_info(isdsSessions.session(
	    accountInfo.userName()), dmId.toStdString().c_str(), &message);

	if (IE_SUCCESS != status) {
		qDebug() << status << isds_strerror(status);
		isds_message_free(&message);
		return false;
	}

	int dmID = atoi(message->envelope->dmID);

	/* get signed raw data from message */
	if (signedMsg) {
		QString raw = QByteArray((char*)message->raw,
		    message->raw_length).toBase64();
		(messageDb.msgsInsertUpdateDeliveryRaw(dmID, raw))
		? qDebug() << "Message raw delivery info was updated..."
		: qDebug() << "ERROR: Message raw delivery info update!";
	}

	struct isds_list *event;
	event = message->envelope->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb.msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type),
		    item->description);
		event = event->next;
	}

	isds_list_free(&message->envelope->events);
	isds_message_free(&message);

	return true;
}


/* ========================================================================= */
/*
* Get additional info about author (sender)
*/
bool Worker::getMessageAuthor(const QModelIndex &acntTopIdx,
    const QString dmId, MessageDb &messageDb)
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

	(messageDb.addMessageAuthorInfo(dmID,
	    convertSenderTypeToString((int)*sender_type), sender_name))
	? qDebug() << "Author info of message was added..."
	: qDebug() << "ERROR: Author info of message wrong!";

	return true;
}


/* ========================================================================= */
/*
* Set message as downloaded from ISDS.
*/
bool Worker::markMessageAsDownloaded(const QModelIndex &acntTopIdx,
    const QString dmId)
/* ========================================================================= */
{
	debug_func_call();

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
