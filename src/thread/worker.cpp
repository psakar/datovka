#include <QTimer>
#include <QEventLoop>
#include <QThread>
#include <QDebug>

#include "worker.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/pkcs7.h"




Worker::Worker(MessageDb &db, const QModelIndex &acntTopIdx, QString &text, QObject *parent) :

	QObject(parent),
	m_db(db),
	m_acntTopIdx(acntTopIdx),
	m_text(text)
{
	_working =false;
	_abort = false;
	qDebug() << m_acntTopIdx;
	qDebug() << &m_db;
}

void Worker::requestWork() {

	mutex.lock();
	_working = true;
	_abort = false;
	qDebug() << "Request worker start in Thread " <<
	    thread()->currentThreadId();
	mutex.unlock();

	emit workRequested();
}

void Worker::abort() {

	mutex.lock();
	if (_working) {
		_abort = true;
		qDebug() << "Request worker aborting in Thread " <<
		    thread()->currentThreadId();
	}

	mutex.unlock();
}

void Worker::doWork()
{
	qDebug() << "Starting worker process in Thread "
	   << thread()->currentThreadId();


	for (int i = 0; i < 20; i ++) {

		// Checks if the process should be aborted
		mutex.lock();
		bool abort = _abort;
		mutex.unlock();

		if (abort) {
			qDebug()<<"Aborting worker process in Thread "<<thread()->currentThreadId();
			break;
		}

		// This will stupidly wait 1 sec doing nothing...
		QEventLoop loop;
		QTimer::singleShot(500, &loop, SLOT(quit()));
		loop.exec();

		qDebug() << "--" << i << "-" << m_text
			<< thread()->currentThreadId() ;

		// Once we're done waiting, value is updated
		//emit valueChanged(QString::number(i));
	}

	// Set _working to false, meaning the process can't be aborted anymore.

    mutex.lock();
    _working = false;
    mutex.unlock();

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	//Once 60 sec passed, the finished signal is sent
	emit finished();
}


/* ========================================================================= */
/*
* Download sent/received message list from ISDS for current account index
*/
void Worker::downloadMessageList()
/* ========================================================================= */
{
	qDebug() << "Starting worker process in Thread "
	   << thread()->currentThreadId();

	const AccountModel::SettingsMap accountInfo =
	    m_acntTopIdx.data(ROLE_ACNT_CONF_SETTINGS).toMap();
	qDebug() << "1 ";
	if (!isdsSessions.isConnectToIsds(accountInfo.userName())) {
		if (!isdsSessions.connectToIsds(accountInfo)) {
			qDebug() << "Error connection to ISDS";
		}
	}

	isds_error status = IE_ERROR;
	struct isds_list *messageList = NULL;

	/* Download sent/received message list from ISDS for current account */
	if (m_text == "sent") {
		status = isds_get_list_of_sent_messages(isdsSessions.
		    session(accountInfo.userName()),
		    NULL, NULL, NULL,
		    //MESSAGESTATE_SENT |  MESSAGESTATE_STAMPED |
		    //MESSAGESTATE_INFECTED | MESSAGESTATE_DELIVERED,
		    MESSAGESTATE_ANY,
		    0, NULL, &messageList);
	} else if (m_text == "received") {
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
	}

	struct isds_list *box;
	box = messageList;
	int newcnt = 0;
	int allcnt = 0;

	MessageDb *messageDb = &this->m_db;

	while (0 != box) {
		allcnt++;
		box = box->next;
	}

	box = messageList;

	int delta = 0;
	if (allcnt == 0) {

	} else {
		delta = ceil(70 / allcnt);
	}

	while (0 != box) {


		isds_message *item = (isds_message *) box->data;
		int dmId = atoi(item->envelope->dmID);

		if (!messageDb->isInMessageDb(dmId)) {

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
			(messageDb->msgsInsertMessageEnvelope(dmId,
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
			    m_text))
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

	if (m_text == "received") {
		qDebug() << "#Received total:" << allcnt;
		qDebug() << "#Received new:" << newcnt;
	} else {
		qDebug() << "#Sent total:" << allcnt;
		qDebug() << "#Sent new:" << newcnt;
	}
}

