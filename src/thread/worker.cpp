#include <QThread>
#include <QDebug>

#include "worker.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/pkcs7.h"
#include "src/gui/datovka.h"


Worker::Worker(AccountDb &accountDb, AccountModel &accountModel, int count,
	    QList<MessageDb*> messageDbList, QObject *parent) :

	QObject(parent),
	m_accountDb(accountDb),
	m_accountModel(accountModel),
	m_count(count),
	m_messageDbList(messageDbList)
{
	_working =false;
	_abort = false;
}


/* ========================================================================= */
/*
* Tread executing prepare
*/
void Worker::requestWork() {
/* ========================================================================= */

	mutex.lock();
	_working = true;
	_abort = false;
	qDebug() << "Request worker start in Thread " <<
	    thread()->currentThreadId();
	mutex.unlock();

	emit workRequested();
}


/* ========================================================================= */
/*
* Abort tread executing
*/
void Worker::abort()
/* ========================================================================= */
{

	mutex.lock();
	if (_working) {
		_abort = true;
		qDebug() << "Request worker aborting in Thread " <<
		    thread()->currentThreadId();
	}

	mutex.unlock();
}


/* ========================================================================= */
/*
* Start background downloading of messages
*/
void Worker::doWork()
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	   << thread()->currentThreadId();

	bool success = true;
	MessageDb *messageDb;

	for (int i = 0; i < m_count; i++) {

		mutex.lock();
		bool abort = _abort;
		mutex.unlock();

		if (abort) {
			qDebug()<< "Aborting worker process in Thread " <<
			    thread()->currentThreadId();
			break;
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
		    downloadMessageList(index,"received", *messageDb)) {
			success = false;
			continue;
		}

		if (Q_CONNECT_ERROR ==
		    downloadMessageList(index,"sent", *messageDb)) {
			success = false;
			continue;
		}

		if (!getListSentMessageStateChanges(index, *messageDb)) {
			success = false;
			continue;
		}

		if (!getPasswordInfo(index)) {
			success = false;
		}

	}

	qDebug() << "-----------------------------------------------";
	success ? qDebug() << "All DONE!" : qDebug() << "An error occurred!";

	// Set _working to false, meaning the process can't be aborted anymore.
	mutex.lock();
	_working = false;
	mutex.unlock();

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	emit finished();
}


/* ========================================================================= */
/*
* Download sent/received message list from ISDS for current account index
*/
qdatovka_error Worker::downloadMessageList(const QModelIndex &acntTopIdx,
    const QString messageType, MessageDb &messageDb)
/* ========================================================================= */
{
	Q_ASSERT(acntTopIdx.isValid());
	if (!acntTopIdx.isValid()) {
		return Q_GLOBAL_ERROR;
	}

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo)) {
			qDebug() << "Error connection to ISDS";
			return Q_CONNECT_ERROR;
		}
	}

	isds_error status = IE_ERROR;
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

	if (status != IE_SUCCESS) {
		qDebug() << status << isds_strerror(status);
		isds_list_free(&messageList);
		return Q_ISDS_ERROR;
	}

	struct isds_list *box;
	box = messageList;
	int newcnt = 0;
	int allcnt = 0;

	while (0 != box) {
		allcnt++;
		box = box->next;
	}

	box = messageList;

	while (0 != box) {

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
			newcnt++;
		}
		box = box->next;

	}

	isds_list_free(&messageList);

	/* Redraw views' content. */
//	regenerateAccountModelYears(acntTopIdx);
	/*
	 * Force repaint.
	 * TODO -- A better solution?
	 */
//	ui->accountList->repaint();
//	accountItemSelectionChanged(ui->accountList->currentIndex());

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
    MessageDb &messageDb)
/* ========================================================================= */
{
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	struct isds_list *stateList = NULL;
	isds_error status;

	status = isds_get_list_of_sent_message_state_changes(
	    isdsSessions.session(accountInfo.userName()),NULL,NULL, &stateList);

	if (IE_SUCCESS != status) {
		isds_list_free(&stateList);
		qDebug() << status << isds_strerror(status);
		return false;
	}

	struct isds_list *stateListFirst = NULL;
	stateListFirst = stateList;

	int allcnt = 0;

	while (0 != stateList) {
		allcnt++;
		stateList = stateList->next;
	}

	stateListFirst = stateList;

	while (0 != stateListFirst) {
		isds_message_status_change *item =
		    (isds_message_status_change *) stateListFirst->data;
		int dmId = atoi(item->dmID);
		/* Download and save delivery info and message events */
		(getSentDeliveryInfo(acntTopIdx, dmId, true, messageDb))
		? qDebug() << "Delivery info of message was processed..."
		: qDebug() << "ERROR: Delivery info of message not found!";

		stateListFirst = stateListFirst->next;
	}

	isds_list_free(&stateList);

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
	QString dmId = QString::number(msgIdx);
	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		isdsSessions.connectToIsds(accountInfo);
	}

	// message and envleople structures
	struct isds_message *message = NULL;
	isds_error status;

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
	isds_error status;
	struct timeval *expiration = NULL;
	QString expirDate;

	const AccountModel::SettingsMap accountInfo =
	    acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();

	QString key = accountInfo.userName() + "___True";

	if (accountInfo.loginMethod() != "username" &&
	    accountInfo.loginMethod() != "user_certificate") {
		expirDate = "";
		m_accountDb.setPwdExpirIntoDb(key, expirDate);
		return true;
	} else {

		if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
			isdsSessions.connectToIsds(accountInfo);
		}

		status = isds_get_password_expiration(
		    isdsSessions.session(accountInfo.userName()), &expiration);

		if (IE_SUCCESS == status) {
			if (0 != expiration) {
				expirDate = timevalToDbFormat(expiration);
				m_accountDb.setPwdExpirIntoDb(key, expirDate);
				return true;
			}
		}
		return true;
	}
	return false;
}
