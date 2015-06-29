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

#include <QTextStream>

#include "src/cli/cli.h"
#include "src/io/isds_sessions.h"
#include "src/gui/datovka.h"
#include "src/io/account_db.h"
#include "src/thread/worker.h"

// Known attributes definition
const QStringList connectAttrs = QStringList()
    << "username" << "password" << "certificate" << "otpcode";
const QStringList getMsgListAttrs = QStringList()
    << "dmType" << "dmStatusFilter" << "dmLimit" << "dmFromTime" << "dmToTime";
const QStringList sendMsgAttrs = QStringList()
    << "dbIDRecipient" << "dmAnnotation" << "dmToHands"
    << "dmRecipientRefNumber" << "dmSenderRefNumber" << "dmRecipientIdent"
    << "dmSenderIdent" << "dmLegalTitleLaw" << "dmLegalTitleYear"
    << "dmLegalTitleSect" << "dmLegalTitlePar" << "dmLegalTitlePoint"
    << "dmPersonalDelivery" << "dmAllowSubstDelivery" << "dmType" << "dmOVM"
    << "dmPublishOwnID" << "dmAttachment";
const QStringList getMsgAttrs = QStringList()
    << "dmID" << "dmType" << "zfoFile" << "download";
const QStringList getDelInfoAttrs = QStringList()
    << "dmID" << "zfoFile" << "download";


/* ========================================================================= */
void printDataToStdOut(const QStringList &data)
/* ========================================================================= */
{
	QTextStream cout(stdout);

	for (int i = 0; i < data.count(); ++i) {
		if (i == (data.count() - 1)) {
			cout << data.at(i) << endl;
		} else {
			cout << data.at(i) + ",";
		}
	}
}


/* ========================================================================= */
const QString createErrorMsg(const QString &msg)
/* ========================================================================= */
{
	return QString(CLI_PREFIX) + QString(PARSER_PREFIX) + msg;
}


/* ========================================================================= */
static
void isds_document_free_void(void **document)
/* ========================================================================= */
{
	isds_document_free((struct isds_document **) document);
}


/* ========================================================================= */
int getMsgList(const QMap<QString,QVariant> &map, MessageDb *messageDb)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Downloading of message list for username"
	    << username;

	/* messages counters */
	int st, sn, rt, rn;
	st = sn = rt = rn = 0;
	QString errmsg;
	qdatovka_error ret;
	QStringList newMsgIdList;
	ulong *dmLimit = NULL;
	uint dmStatusFilter = MESSAGESTATE_ANY;
	bool ok;

	if (map.contains("dmStatusFilter")) {
		uint number = map["dmStatusFilter"].toString().toUInt(&ok);
		if (!ok) {
			qDebug() << CLI_PREFIX << "Wrong dmStatusFilter "
			    "value:" << map["dmStatusFilter"].toString();
			return CLI_RET_ERROR_CODE;
		}
		switch (number) {
		case 1: dmStatusFilter = MESSAGESTATE_SENT; break;
		case 2: dmStatusFilter = MESSAGESTATE_STAMPED; break;
		case 3: dmStatusFilter = MESSAGESTATE_INFECTED; break;
		case 4: dmStatusFilter = MESSAGESTATE_DELIVERED; break;
		case 5: dmStatusFilter = MESSAGESTATE_SUBSTITUTED; break;
		case 6: dmStatusFilter = MESSAGESTATE_RECEIVED; break;
		case 7: dmStatusFilter = MESSAGESTATE_READ; break;
		case 8: dmStatusFilter = MESSAGESTATE_UNDELIVERABLE; break;
		case 9: dmStatusFilter = MESSAGESTATE_REMOVED; break;
		case 10: dmStatusFilter = MESSAGESTATE_IN_SAFE; break;
		default: dmStatusFilter = MESSAGESTATE_ANY; break;
		}
	}

	if (map.contains("dmLimit")) {
		bool ok;
		dmLimit = (ulong *) malloc(sizeof(ulong));
		*dmLimit = map["dmLimit"].toString().toULong(&ok);
		if (!ok) {
			qDebug() << CLI_PREFIX << "Wrong dmLimit "
			    "value:" << map["dmLimit"].toString();
			return CLI_RET_ERROR_CODE;
		}
	}

	if (map["dmType"].toString() == MT_SENT) {

		ret = Worker::downloadMessageList(username, MSG_RECEIVED,
		    *messageDb, errmsg, NULL, 0, 0, rt, rn, newMsgIdList,
		    dmLimit, dmStatusFilter);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Received message list "
			    "has been downloaded.";
			printDataToStdOut(newMsgIdList);
			return CLI_RET_OK_CODE;
		} else {
			qDebug() << CLI_PREFIX << "Error while downloading "
			    "received message list! Error code:"
			    << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}

	} else if (map["dmType"].toString() == MT_RECEIVED) {

		ret = Worker::downloadMessageList(username, MSG_SENT,
		    *messageDb, errmsg, NULL, 0, 0, st, sn, newMsgIdList,
		    dmLimit, dmStatusFilter);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Sent message list has been "
			    "downloaded.";
			printDataToStdOut(newMsgIdList);
			return 0;
		} else {
			qDebug() << CLI_PREFIX << "Error while downloading "
			    "sent message list! Error code:" << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}

	} else if (map["dmType"].toString() == MT_SENT_RECEIVED) {

		int ok = CLI_RET_OK_CODE;
		ret = Worker::downloadMessageList(username, MSG_RECEIVED,
		    *messageDb, errmsg, NULL, 0, 0, rt, rn, newMsgIdList,
		    dmLimit, dmStatusFilter);
		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Received message list has "
			    "been downloaded.";
		}  else {
			qDebug() << CLI_PREFIX << "Error while downloading "
			    "received message list! Error code:" <<
			    ret << errmsg;
			ok = CLI_RET_ERROR_CODE;
		}
		ret = Worker::downloadMessageList(username, MSG_SENT,
		    *messageDb, errmsg, NULL, 0, 0, st, sn, newMsgIdList,
		    dmLimit, dmStatusFilter);
		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Sent message list has been "
			    "downloaded.";
		}  else {
			qDebug() << CLI_PREFIX << "Error while downloading "
			    "sent message list! Error code:" << ret << errmsg;
			ok = CLI_RET_ERROR_CODE;
		}
		printDataToStdOut(newMsgIdList);
		return ok;

	} else {
		qDebug() << CLI_PREFIX << "Wrong dmType value:" <<
		    map["dmType"].toString();
		return CLI_RET_ERROR_CODE;
	}

	return CLI_RET_ERROR_CODE;
}


/* ========================================================================= */
int getMsg(const QMap<QString,QVariant> &map, MessageDb *messageDb,
    bool needsISDS)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "Downloading of message" <<
	    map["dmID"].toString();

	QString errmsg;
	qdatovka_error ret;
	const QString username = map["username"].toString();

	if (needsISDS) {
		if (map["dmType"].toString() == "received") {

			ret = Worker::downloadMessage(username,
			    map["dmID"].toLongLong(), true, MSG_RECEIVED,
			    *messageDb, errmsg, NULL, 0, 0);

			if (Q_SUCCESS == ret) {
				qDebug() << CLI_PREFIX << "Received message" <<
				    map["dmID"].toString() << "has been "
				    "downloaded.";
			} else {
				qDebug() << CLI_PREFIX << "Error while "
				    "downloading "
				    "received message! Error code:"
				    << ret << errmsg;
				return CLI_RET_ERROR_CODE;
			}

		} else if (map["dmType"].toString() == "sent") {

			ret = Worker::downloadMessage(username,
			    map["dmID"].toLongLong(), true, MSG_SENT,
			    *messageDb, errmsg, NULL, 0, 0);

			if (Q_SUCCESS == ret) {
				qDebug() << CLI_PREFIX << "Sent message" <<
				    map["dmID"].toString() << "has been "
				    "downloaded.";
			} else {
				qDebug() << CLI_PREFIX << "Error while "
				    "downloading "
				    "received message! Error code:"
				    << ret << errmsg;
				return CLI_RET_ERROR_CODE;
			}
		} else {
			qDebug() << CLI_PREFIX << "Wrong dmType value:" <<
			    map["dmType"].toString();
			return CLI_RET_ERROR_CODE;
		}
	}

	if (map.contains("zfoFile") && !map["zfoFile"].toString().isEmpty()) {
		const QFileInfo fi(map["zfoFile"].toString());
		const QString path = fi.path();
		if (!QDir(path).exists()) {
			qDebug() << CLI_PREFIX << "Wrong path" <<
			    path << "for file saving!";
			return CLI_RET_ERROR_CODE;
		}

		QByteArray base64 =
		    messageDb->msgsMessageBase64(map["dmID"].toLongLong());

		if (base64.isEmpty()) {
			qDebug() << CLI_PREFIX <<
			    "Cannot export complete message to ZFO!";
			return CLI_RET_ERROR_CODE;
		}

		QByteArray data = QByteArray::fromBase64(base64);
		enum WriteFileState ret = writeFile(map["zfoFile"].toString(),
		    data);
		if (WF_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Export of message" <<
			map["dmID"].toString() <<  "to ZFO was successful.";
		} else {
			qDebug() << CLI_PREFIX << "Export of message" <<
			map["dmID"].toString() <<  "to ZFO was NOT successful!";
			return CLI_RET_ERROR_CODE;
		}
	}

	return CLI_RET_OK_CODE;
}


/* ========================================================================= */
int getDeliveryInfo(const QMap<QString,QVariant> &map,
    MessageDb *messageDb, bool needsISDS)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "Downloading of delivery info for message" <<
	    map["dmID"].toString();

	qdatovka_error ret;
	const QString username = map["username"].toString();

	if (needsISDS) {
		ret = Worker::getDeliveryInfo(username,
		    map["dmID"].toLongLong(), true, *messageDb);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Delivery info of message" <<
			    map["dmID"].toString() << "has been downloaded.";
		} else {
			qDebug() << CLI_PREFIX << "Error while downloading "
			    "delivery info! Error code:" << ret;
			return CLI_RET_ERROR_CODE;
		}
	}

	if (map.contains("zfoFile") && !map["zfoFile"].toString().isEmpty()) {
		const QFileInfo fi(map["zfoFile"].toString());
		const QString path = fi.path();
		if (!QDir(path).exists()) {
			qDebug() << CLI_PREFIX << "Wrong path" <<
			    path << "for file saving!";
			return CLI_RET_ERROR_CODE;
		}

		QByteArray base64 =
		    messageDb->msgsGetDeliveryInfoBase64(
		         map["dmID"].toLongLong());

		if (base64.isEmpty()) {
			qDebug() << CLI_PREFIX <<
			    "Cannot export delivery info to ZFO!";
			return CLI_RET_ERROR_CODE;
		}

		QByteArray data = QByteArray::fromBase64(base64);
		enum WriteFileState ret = writeFile(map["zfoFile"].toString(),
		    data);
		if (WF_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Export of delivery info" <<
			map["dmID"].toString() <<  "to ZFO was successful.";
		} else {
			qDebug() << CLI_PREFIX << "Export of delivery info" <<
			map["dmID"].toString() <<  "to ZFO was NOT successful!";
			return CLI_RET_ERROR_CODE;
		}
	}

	return CLI_RET_OK_CODE;
}


/* ========================================================================= */
int checkAttachment(const QMap<QString,QVariant> &map,
    MessageDb *messageDb)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Checking of missing messages attachment for"
	    " username" <<  username;

	printDataToStdOut(messageDb->getAllMessageIDsWithoutAttach());

	return CLI_RET_OK_CODE;
}


/* ========================================================================= */
int getUserInfo(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Downloading info about username"
	    << username;

	if (MainWindow::getOwnerInfoFromLogin(map["username"].toString())) {
		return CLI_RET_OK_CODE;
	}

	return CLI_RET_ERROR_CODE;
}


/* ========================================================================= */
int getOwnerInfo(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "downloading info about owner and its "
	    "databox for username" <<  username;

	if (MainWindow::getOwnerInfoFromLogin(map["username"].toString())) {
		return CLI_RET_OK_CODE;
	}

	return CLI_RET_ERROR_CODE;
}


/* ========================================================================= */
// NOTE: not used now.
int createContextAmdLoginToIsds(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "creating a isds context and loggin to "
	    "databox for username" <<  username;

	AccountModel::SettingsMap accountInfo;

	/* set account items */
	accountInfo.setAccountName("cli-");
	accountInfo.setUserName(username);
	accountInfo.setRememberPwd(false);
	accountInfo.setPassword(map["password"].toString());
	if (map["type"].toString() == "test") {
		accountInfo.setTestAccount(true);
	} else {
		accountInfo.setTestAccount(false);
	}
	accountInfo.setSyncWithAll(false);

	if (map["method"].toString() == L_USER) {
		accountInfo.setLoginMethod(LIM_USERNAME);
		accountInfo.setP12File("");
	} else if (map["method"].toString() == L_CERT) {
		accountInfo.setLoginMethod(LIM_USER_CERT);
		accountInfo.setP12File(QDir::fromNativeSeparators(
		    map["certificate"].toString()));
	} else if (map["method"].toString() == L_HOTP) {
		accountInfo.setLoginMethod(LIM_HOTP);
		accountInfo.setP12File("");
	} else if (map["method"].toString() == L_TOTP) {
		accountInfo.setLoginMethod(LIM_TOTP);
		accountInfo.setP12File("");
	} else {
		return CLI_RET_ERROR_CODE;
	}

	/*
	* TODO - create isds session and login to databox
	*        without account model.
	*
	* if (!isdsSessions.isConnectedToIsds(username)) {
	*	if (!MainWindow::firstConnectToIsds(accountInfo, false)) {
	*		qDebug() << CLI_PREFIX << "connection error:"
	*		  << isds_long_message(isdsSessions.session(username));
	*		return CLI_RET_ERROR_CODE;
	*	}
	* }
	*/

	return CLI_RET_OK_CODE;
}


/* ========================================================================= */
int createAndSendMsg(const QMap <QString, QVariant> &map, MessageDb *messageDb)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;
	QStringList dbIds = map.value("dbIDRecipient").toStringList();
	QStringList sendID;
	sendID.clear();

	qDebug() << CLI_PREFIX << "creating a new message...";

	/* Sent message. */
	struct isds_message *sent_message = NULL;
	/* All remaining structures are created to be a part of the message. */
	struct isds_document *document = NULL; /* Attachment. */
	struct isds_list *documents = NULL; /* Attachment list. */
	struct isds_envelope *sent_envelope = NULL; /* Message envelope. */
	struct isds_list *last = NULL; /* No need to free it explicitly. */

	sent_envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));
	if (sent_envelope == NULL) {
		errmsg = "Out of memory.";
		goto finish;
	}
	memset(sent_envelope, 0, sizeof(struct isds_envelope));

	sent_message = (struct isds_message *)
	    malloc(sizeof(struct isds_message));
	if (sent_message == NULL) {
		errmsg = "Out of memory.";
		goto finish;
	}
	memset(sent_message, 0, sizeof(struct isds_message));

	/* Load attachments. */
	for (int i = 0; i < map["dmAttachment"].toStringList().count(); ++i) {

		const QString filePath = map["dmAttachment"].
		    toStringList().at(i);

		if (filePath.isEmpty()) {
			continue;
		}

		QFileInfo fi(filePath);
		if (!fi.isReadable() || !fi.isFile()) {
			errmsg = "wrong file name or file missing!";
			qDebug() << CLI_PREFIX << filePath << errmsg;
			goto finish;
		}

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			errmsg = "Out of memory.";
			goto finish;
		}
		memset(document, 0, sizeof(struct isds_document));

		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;


		QString name = fi.fileName();
		document->dmFileDescr = strdup(name.toUtf8().constData());
		if (NULL == document->dmFileDescr) {
			errmsg = "Out of memory.";
			goto finish;
		}

		if (0 == i) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

		QString mimeType = "";
		document->dmMimeType = strdup(mimeType.toUtf8().constData());
		if (NULL == document->dmMimeType) {
			errmsg = "Out of memory.";
			goto finish;
		}

		QFile file(QDir::fromNativeSeparators(
		    map["dmAttachment"].toStringList().at(i)));
		if (file.exists()) {
			if (!file.open(QIODevice::ReadOnly)) {
				errmsg = "Couldn't open the file \""
				    + map["dmAttachment"].toStringList().at(i)
				    + "\"";
				goto finish;
			}
		}
		QByteArray bytes = file.readAll();
		document->data_length = bytes.size();
		document->data = malloc(bytes.size());
			if (NULL == document->data) {
				errmsg = "Out of memory.";
				goto finish;
			}
		memcpy(document->data, bytes.data(), document->data_length);

		struct isds_list *newListItem = (struct isds_list *) malloc(
		    sizeof(struct isds_list));
		if (NULL == newListItem) {
			errmsg = "Out of memory.";
			goto finish;
		}

		newListItem->data = document; document = NULL;
		newListItem->next = NULL;
		newListItem->destructor = isds_document_free_void;

		if (last == NULL) {
			documents = last = newListItem;
		} else {
			last->next = newListItem;
			last = newListItem;
		}
	}

	/* Set mandatory fields of envelope. */
	sent_envelope->dmID = NULL;
	sent_envelope->dmAnnotation =
	    strdup(map["dmAnnotation"].toString().toUtf8().constData());
	if (NULL == sent_envelope->dmAnnotation) {
		errmsg = "Out of memory.";
		goto finish;
	}

	/* Set optional fields. */
	if (map.contains("dmSenderIdent")) {
		sent_envelope->dmSenderIdent =
		    strdup(map["dmSenderIdent"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmSenderIdent) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmRecipientIdent")) {
		sent_envelope->dmRecipientIdent =
		    strdup(map["dmRecipientIdent"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmRecipientIdent) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmSenderRefNumber")) {
		sent_envelope->dmSenderRefNumber =
		    strdup(map["dmSenderRefNumber"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmSenderRefNumber) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmRecipientRefNumber")) {
		sent_envelope->dmRecipientRefNumber =
		    strdup(map["dmRecipientRefNumber"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmRecipientRefNumber) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmToHands")) {
		sent_envelope->dmToHands =
		    strdup(map["dmToHands"].toString().toUtf8().constData());
		if (NULL == sent_envelope->dmToHands) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmLegalTitleLaw")) {
		sent_envelope->dmLegalTitleLaw =
		    (long int *) malloc(sizeof(long int));
		if (NULL == sent_envelope->dmLegalTitleLaw) {
			errmsg = "Out of memory.";
			goto finish;
		}
		*sent_envelope->dmLegalTitleLaw =
		    map["dmLegalTitleLaw"].toLongLong();
	} else {
		sent_envelope->dmLegalTitleLaw = NULL;
	}

	if (map.contains("dmLegalTitleYear")) {
		sent_envelope->dmLegalTitleYear =
		    (long int *) malloc(sizeof(long int));
		if (NULL == sent_envelope->dmLegalTitleYear) {
			errmsg = "Out of memory.";
			goto finish;
		}
		*sent_envelope->dmLegalTitleYear =
		    map["dmLegalTitleYear"].toLongLong();
	} else {
		sent_envelope->dmLegalTitleYear = NULL;
	}

	if (map.contains("dmLegalTitleSect")) {
		sent_envelope->dmLegalTitleSect =
		    strdup(map["dmLegalTitleSect"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmLegalTitleSect) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmLegalTitlePar")) {
		sent_envelope->dmLegalTitlePar =
		    strdup(map["dmLegalTitlePar"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmLegalTitlePar) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmLegalTitlePoint")) {
		sent_envelope->dmLegalTitlePoint =
		    strdup(map["dmLegalTitlePoint"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmLegalTitlePoint) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	if (map.contains("dmType")) {
		sent_envelope->dmType =
		    strdup(map["dmType"].toString().toUtf8()
		    .constData());
		if (NULL == sent_envelope->dmType) {
			errmsg = "Out of memory.";
			goto finish;
		}
	}

	sent_envelope->dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmPersonalDelivery) {
		errmsg = "Out of memory.";
		goto finish;
	}
	if (map.contains("dmPersonalDelivery")) {
		*sent_envelope->dmPersonalDelivery =
		    (map["dmPersonalDelivery"].toString() == "0")
		    ? false : true;
	} else {
		*sent_envelope->dmPersonalDelivery = true;
	}

	sent_envelope->dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmAllowSubstDelivery) {
		errmsg = "Out of memory.";
		goto finish;
	}
	if (map.contains("dmAllowSubstDelivery")) {
		*sent_envelope->dmAllowSubstDelivery =
		    (map["dmAllowSubstDelivery"].toString() == "0")
		    ? false : true;
	} else {
		*sent_envelope->dmAllowSubstDelivery = true;
	}

	sent_envelope->dmOVM = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmOVM) {
		errmsg = "Out of memory.";
		goto finish;
	}
	if (map.contains("dmOVM")) {
		*sent_envelope->dmOVM =
		    (map["dmOVM"].toString() == "0") ? false : true;
	} else {
		*sent_envelope->dmOVM = true;
	}

	sent_envelope->dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmPublishOwnID) {
		errmsg = "Out of memory.";
		goto finish;
	}
	if (map.contains("dmPublishOwnID")) {
		*sent_envelope->dmPublishOwnID =
		    (map["dmPublishOwnID"].toString() == "0") ? false : true;
	} else {
		*sent_envelope->dmPublishOwnID = false;
	}

	sent_message->documents = documents; documents = NULL;
	sent_message->envelope = sent_envelope; sent_envelope = NULL;

	for (int i = 0; i < dbIds.count(); ++i) {

		qDebug() << CLI_PREFIX << "message sending..." << dbIds.at(i);

		if (NULL != sent_message->envelope->dbIDRecipient) {
			free(sent_message->envelope->dbIDRecipient);
			sent_message->envelope->dbIDRecipient = NULL;
		}

		sent_message->envelope->dbIDRecipient =
		    strdup(dbIds.at(i).toUtf8().constData());
		if (NULL == sent_message->envelope->dbIDRecipient) {
			errmsg = "Out of memory.";
			goto finish;
		}

		status = isds_send_message(isdsSessions.session(
		    map["username"].toString()), sent_message);

		errmsg = isds_long_message(isdsSessions.session(
		    map["username"].toString()));

		if (IE_SUCCESS == status) {
			/* Added a new message into database */
			qint64 dmId =
			    QString(sent_message->envelope->dmID).toLongLong();
			const QString dbIDSender = globAccountDbPtr->dbId(
			    map["username"].toString() + "___True");
			const QString dmSender = globAccountDbPtr->
			    senderNameGuess(map["username"].toString() +
			    "___True");
			messageDb->msgsInsertNewlySentMessageEnvelope(dmId,
				    dbIDSender,
				    dmSender,
				    dbIds.at(i),
				    "Databox ID: " +
				    dbIds.at(i),
				    "unknown",
				    map["dmAnnotation"].toString());
			qDebug() << CLI_PREFIX << "message has been sent"
			    << QString(sent_message->envelope->dmID);
			sendID.append(sent_message->envelope->dmID);
			ret = CLI_RET_OK_CODE;
		} else {
			qDebug() << CLI_PREFIX << "error while sending of "
			"message! Error code:" << status << errmsg;
			ret = CLI_RET_ERROR_CODE;
		}
	}

	printDataToStdOut(sendID);

finish:
	isds_document_free(&document);
	isds_list_free(&documents);
	isds_envelope_free(&sent_envelope);
	isds_message_free(&sent_message);

	return ret;
}


/* ========================================================================= */
bool checkAttributeIfExists(const QString &service, const QString &attribute)
/* ========================================================================= */
{
	if (service == SER_LOGIN) {
		return connectAttrs.contains(attribute);
	} else if (service == SER_GET_MSG_LIST) {
		return getMsgListAttrs.contains(attribute);
	} else if (service == SER_SEND_MSG) {
		return sendMsgAttrs.contains(attribute);
	} else if (service == SER_GET_MSG) {
		return getMsgAttrs.contains(attribute);
	} else if (service == SER_GET_DEL_INFO) {
		return getDelInfoAttrs.contains(attribute);
	}
	return false;
}


/* ========================================================================= */
const QStringList parseAttachment(const QString &files)
/* ========================================================================= */
{
	if (files.isEmpty()) {
		return QStringList();
	}
	return files.split(";");
}


/* ========================================================================= */
const QStringList parseDbIDRecipient(const QString &dbIDRecipient)
/* ========================================================================= */
{
	if (dbIDRecipient.isEmpty()) {
		return QStringList();
	}
	return dbIDRecipient.split(";");
}


/* ========================================================================= */
bool checkLoginMandatoryAttributes(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString errmsg = "checking of mandatory parameters for login...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value!");
		qDebug() << errmsg;
		return false;
	}

	return true;
}


/* ========================================================================= */
bool checkSendMsgMandatoryAttributes(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString errmsg = "checking of mandatory parameters for send message...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dbIDRecipient") ||
	    map.value("dbIDRecipient").toStringList().isEmpty()) {
		errmsg = createErrorMsg("dbIDRecipient attribute missing or "
		    "contains empty databox id list!");
		qDebug() << errmsg;
		return false;
	}
	QStringList dbIds = map.value("dbIDRecipient").toStringList();
	for (int i = 0; i < dbIds.count(); ++i) {
		if (dbIds.at(i).length() != 7) {
			errmsg = createErrorMsg(QString("dbIDRecipient "
			    "attribute contains wrong value or length at "
			    "position %1!").arg(i+1));
			qDebug() << errmsg;
			return false;
		}
	}
	if (!map.contains("dmAnnotation") ||
	    map.value("dmAnnotation").toString().isEmpty()) {
		errmsg = createErrorMsg("dmAnnotation attribute missing or "
		    "contains empty string!");
		qDebug() << errmsg;
		return false;
	}
	if (!map.contains("dmAttachment") ||
	    map.value("dmAttachment").toStringList().isEmpty()) {
		errmsg = createErrorMsg("dmAttachment attribute missing or "
		    "contains empty file path list!");
		qDebug() << errmsg;
		return false;
	}

	return true;
}


/* ========================================================================= */
bool checkGetMsgListMandatoryAttributes(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString errmsg = "checking of mandatory parameters for "
	    "get message list...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = createErrorMsg("dmType attribute missing or "
		    "contains empty string!");
		qDebug() << errmsg;
		return false;
	}
	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED) &&
	    !(dmType == MT_SENT_RECEIVED)) {
		errmsg = createErrorMsg("dmType attribute "
		    "contains wrong value!");
		qDebug() << errmsg;
		return false;
	}

	return true;
}


/* ========================================================================= */
bool checkGetMsgMandatoryAttributes(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString errmsg = "checking of mandatory parameters "
	    "for download message...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = createErrorMsg("dmID attribute missing or "
		    "contains empty string!");
		qDebug() << errmsg;
		return false;
	}
	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = createErrorMsg("dmType attribute missing or "
		    "contains empty string!");
		qDebug() << errmsg;
		return false;
	}
	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED)) {
		errmsg = createErrorMsg("dmType attribute "
		    "contains wrong value!");
		qDebug() << errmsg;
		return false;
	}

	if (map.contains("download")) {
		QString download = map.value("download").toString();
		if (!(download == "no") && !(download == "yes")
		    && !(download == "ondemand")) {
			errmsg = createErrorMsg("download attribute has "
			    "wrong value!");
			qDebug() << errmsg;
			return false;
		}
	}

	return true;
}


/* ========================================================================= */
bool checkDownloadDeliveryMandatoryAttributes(const QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString errmsg = "checking of mandatory parameters "
	    "for download delivery info...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = createErrorMsg("dmID attribute missing or "
		    "contains empty string!");
		qDebug() << errmsg;
		return false;
	}

	if (map.contains("download")) {
		QString download = map.value("download").toString();
		if (!(download == "no") && !(download == "yes")
		    && !(download == "ondemand")) {
			errmsg = createErrorMsg("download attribute has "
			    "wrong value!");
			qDebug() << errmsg;
			return false;
		}
	}

	return true;
}


/* ========================================================================= */
bool checkMandatoryAttributes(const QString &service, QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	if (service == SER_LOGIN) {
		return checkLoginMandatoryAttributes(map);
	} else if (service == SER_GET_MSG_LIST) {
		return checkGetMsgListMandatoryAttributes(map);
	} else if (service == SER_SEND_MSG) {
		return checkSendMsgMandatoryAttributes(map);
	} else if (service == SER_GET_MSG) {
		return checkGetMsgMandatoryAttributes(map);
	} else if (service == SER_GET_DEL_INFO) {
		return checkDownloadDeliveryMandatoryAttributes(map);
	}

	return false;
}


/* ========================================================================= */
bool parsePamamString(const QString &service, const QString &paramString,
    QMap<QString,QVariant> &map)
/* ========================================================================= */
{
	QString attribute = "";
	QString value = "";
	QString errmsg;
	bool newAttribute = true;
	bool newValue = false;
	bool special = false;
	int attrPosition = 0;

	for (int i = 0; i < paramString.length(); ++i) {
		if (paramString.at(i) == ',') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				attrPosition++;
				//qDebug() << attribute << value;
				if (attribute.isEmpty()) {
					errmsg = createErrorMsg(
					    QString("empty attribute "
					    "name on position '%1'").
					    arg(attrPosition));
					qDebug() << errmsg;
					return false;
				}
				if (value.isEmpty()) {
					errmsg = createErrorMsg(
					    QString("empty attribute "
					    "value on position '%1'").
					    arg(attrPosition));
					qDebug() << errmsg;
					return false;
				}

				if (checkAttributeIfExists(service,attribute)) {
					if (attribute == "dmAttachment") {
						map[attribute] =
						    parseAttachment(value);
					} else
					if (attribute == "dbIDRecipient") {
						map[attribute] =
						    parseDbIDRecipient(value);
					} else {
						map[attribute] = value;
					}
				} else {
					errmsg = createErrorMsg(
					    QString("unknown attribute "
					    "name '%1'").arg(attribute));
					qDebug() << errmsg;
					return false;
				}
				attribute.clear();
				value.clear();
				newAttribute = true;
				newValue = false;
			}
		} else if (paramString.at(i) == '=') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				newAttribute = false;
			}
		} else if (paramString.at(i) == '\'') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				newValue = !newValue;
			}
		} else if (paramString.at(i) == '\\') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				if (attribute == "dmAttachment" ||
				    attribute == "zfoFile" ||
				    attribute == "certificate") {
					value = value + paramString.at(i);
					special = false;
				} else {
					special = true;
				}
			}
		} else {
			if (newAttribute) {
				attribute = attribute + paramString.at(i);
			}
			if (newValue) {
				value = value + paramString.at(i);
			}
		}
	}

	// parse last token
	attrPosition++;
	if (attribute.isEmpty()) {
		errmsg = createErrorMsg(QString("empty attribute "
		    "name on position '%1'").arg(attrPosition));
		qDebug() << errmsg;
		return false;
	}
	if (value.isEmpty()) {
		errmsg = createErrorMsg(QString("empty attribute "
		    "value on position '%1'").arg(attrPosition));
		qDebug() << errmsg;
		return false;
	}
	if (checkAttributeIfExists(service, attribute)) {
		if (attribute == "dmAttachment") {
			map[attribute] = parseAttachment(value);
		} else if (attribute == "dbIDRecipient") {
			map[attribute] = parseDbIDRecipient(value);
		} else {
			map[attribute] = value;
		}
	} else {
		errmsg = createErrorMsg(QString("unknown attribute "
		    "name '%1'").arg(attribute));
		qDebug() << errmsg;
		return false;
	}

	if (!checkMandatoryAttributes(service, map)) {
		return false;
	}

	// add service name to map
	map["service"] = service;

	return true;
}


/* ========================================================================= */
int doService(const QString &service, const QMap<QString,QVariant> &map,
    MessageDb *messageDb, bool needsISDS)
/* ========================================================================= */
{

	if (service == SER_GET_MSG_LIST) {
		return getMsgList(map, messageDb);
	} else if (service == SER_SEND_MSG) {
		return createAndSendMsg(map, messageDb);
	} else if (service == SER_GET_MSG) {
		return getMsg(map, messageDb, needsISDS);
	} else if (service == SER_GET_DEL_INFO) {
		return getDeliveryInfo(map, messageDb, needsISDS);
	} else if (service == SER_GET_USER_INFO) {
		return getUserInfo(map);
	} else if (service == SER_GET_OWNER_INFO) {
		return getOwnerInfo(map);
	} else if (service == SER_CHECK_ATTACHMENT) {
		return checkAttachment(map, messageDb);
	}

	return CLI_RET_ERROR_CODE;
}


/* ========================================================================= */
int runService(const QString &lParam,
    const QString &service, const QString &sParam)
/* ========================================================================= */
{
	QMap<QString,QVariant> loginMap;
	QMap <QString,QVariant> serviceMap;
	int ret = CLI_RET_ERROR_CODE;
	bool needsISDS = true;

	/* parse service parameter list */
	if (!(service.isNull()) && !(sParam.isNull())) {
		qDebug() << CLI_PREFIX << "Parsing of input string of service"
		    << service << ":" << sParam;
		if (!parsePamamString(service, sParam, serviceMap)) {
			return CLI_RET_ERROR_CODE;
		}
		qDebug() << CLI_PREFIX << "...done.";
	}

	/* parse login parameter list */
	qDebug() << CLI_PREFIX << "Parsing of input string of \"login\" :"
	    << lParam;
	if (!parsePamamString(SER_LOGIN, lParam, loginMap)) {
		return CLI_RET_ERROR_CODE;
	}
	qDebug() << CLI_PREFIX << "...done.";

	/* get username from login */
	const QString username = loginMap["username"].toString();

	/* get message database */
	MessageDb *messageDb = MainWindow::accountMessageDb(username,0);
	if (messageDb == NULL) {
		qDebug() << CLI_PREFIX << "Database doesn't exists for user"
		    << username;
		return CLI_RET_ERROR_CODE;
	}

	if (service == SER_GET_MSG) {
		if (serviceMap.contains("download")) {
			QString download =
			    serviceMap.value("download").toString();
			if (download == "no") {
				needsISDS = false;
			} else if (download == "ondemand") {
				needsISDS = messageDb->msgsMessageBase64(
				    serviceMap["dmID"].toLongLong()).isNull();
			}
		} else {
			needsISDS = messageDb->msgsMessageBase64(
			    serviceMap["dmID"].toLongLong()).isNull();
		}
	}

	if (service == SER_GET_DEL_INFO) {
		if (serviceMap.contains("download")) {
			QString download =
			    serviceMap.value("download").toString();
			if (download == "no") {
				needsISDS = false;
			} else if (download == "ondemand") {
				needsISDS=messageDb->msgsGetDeliveryInfoBase64(
				    serviceMap["dmID"].toLongLong()).isNull();
			}
		} else {
			needsISDS = messageDb->msgsGetDeliveryInfoBase64(
			    serviceMap["dmID"].toLongLong()).isNull();
		}
	}

	if (service == SER_CHECK_ATTACHMENT) {
		needsISDS = false;
	}

	/* connect to ISDS and login into databox */
	if (needsISDS) {

		QString pwd;
		QString otp;

		if (loginMap.contains("password")) {
			pwd = loginMap["password"].toString();
		}
		if (loginMap.contains("otpcode")) {
			otp = loginMap["otpcode"].toString();
		}

		if (!isdsSessions.isConnectedToIsds(username)) {
			if (!MainWindow::connectToIsds(username,0,pwd,otp)) {
				qDebug() << isds_long_message(
				    isdsSessions.session(username));
				return CLI_RET_ERROR_CODE;
			}
		}
		qDebug() << CLI_PREFIX << "User" << username
		    << "has been logged into databox.";
	}

	/* do service */
	if (!service.isNull()) {
		serviceMap["username"] = username;
		ret = doService(service, serviceMap, messageDb, needsISDS);
	}

	return ret;
}
