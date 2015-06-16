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
const QStringList connectAttrs = QStringList() << "method" << "type"
    << "username" << "password" << "certificate" << "otpcode";
const QStringList getMsgListAttrs = QStringList() << "username" << "dmType"
    << "dmStatusFilter" << "dmOffset" << "dmLimit" << "dmFromTime"
    << "dmToTime";
const QStringList sendMsgAttrs = QStringList() << "username"
    << "dbIDRecipient" << "dmAnnotation" << "dmToHands"
    << "dmRecipientRefNumber" << "dmSenderRefNumber" << "dmRecipientIdent"
    << "dmSenderIdent" << "dmLegalTitleLaw" << "dmLegalTitleYear"
    << "dmLegalTitleSect" << "dmLegalTitlePar" << "dmLegalTitlePoint"
    << "dmPersonalDelivery" << "dmAllowSubstDelivery" << "dmType" << "dmOVM"
    << "dmPublishOwnID" << ATTACH_LABEL;
const QStringList dwnldMsgAttrs = QStringList() << "username" << "dmID";
const QStringList dwnldDelInfoAttrs = QStringList() << "username" << "dmID";
const QString databoxInfoAttrs = "username";



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
int getMsgList(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "download message list...";

	/* messages counters */
	int st, sn, rt, rn;
	st = sn = rt = rn = 0;
	QString errmsg;
	qdatovka_error ret;
	QStringList newMsgIdList;
	const QString username = map["username"].toString();

	MessageDb *messageDb =
	    MainWindow::accountMessageDb(username, 0);

	if (!isdsSessions.isConnectedToIsds(username)) {
		if (!MainWindow::connectToIsds(username, 0)) {
			qDebug() << CLI_PREFIX << "connection error:"
			  << isds_long_message(isdsSessions.session(username));
			return CLI_RET_ERROR_CODE;
		}
	}

	if (map["dmType"].toString() == "received") {

		ret = Worker::downloadMessageList(username, MSG_RECEIVED,
		    *messageDb, errmsg, NULL, 0, 0, rt, rn, newMsgIdList);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "received message list "
			    "has been downloaded for username" <<  username;
			qDebug() << CLI_PREFIX << "#Received total:" << rt
			    << " #Received new:" << rn;
			printDataToStdOut(newMsgIdList);
			return CLI_RET_OK_CODE;
		} else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "received message list! Error code:"
			    << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}

	} else if (map["dmType"].toString() == "sent") {

		ret = Worker::downloadMessageList(username, MSG_SENT,
		    *messageDb, errmsg, NULL, 0, 0, st, sn, newMsgIdList);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "sent message list has been "
			    "downloaded for username" <<  username;
			qDebug() << CLI_PREFIX << "#Sent total:" << st
			    << " #Sent new:" << sn;
			printDataToStdOut(newMsgIdList);
			return 0;
		} else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "sent message list! Error code:" << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}

	} else if (map["dmType"].toString() == "all") {

		int ok = CLI_RET_OK_CODE;
		ret = Worker::downloadMessageList(username, MSG_RECEIVED,
		    *messageDb, errmsg, NULL, 0, 0, rt, rn, newMsgIdList);
		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "received message list has "
			    "been downloaded for username" <<  username;
			qDebug() << CLI_PREFIX << "#Received total:" << rt <<
			    " #Received new:" << rn;
		}  else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "received message list! Error code:" <<
			    ret << errmsg;
			ok = CLI_RET_ERROR_CODE;
		}
		ret = Worker::downloadMessageList(username, MSG_SENT,
		    *messageDb, errmsg, NULL, 0, 0, st, sn, newMsgIdList);
		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "sent message list has been "
			    "downloaded for username" <<  username;
			qDebug() << CLI_PREFIX << "#Sent total:" << st <<
			    " #Sent new:" << sn;
		}  else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "sent message list! Error code:" << ret << errmsg;
			ok = CLI_RET_ERROR_CODE;
		}
		printDataToStdOut(newMsgIdList);
		return ok;

	} else {
		qDebug() << CLI_PREFIX << "wrong dmType value:" <<
		    map["dmType"].toString();
		return CLI_RET_ERROR_CODE;
	}

	return CLI_RET_ERROR_CODE;
}


/* ========================================================================= */
int downloadMsg(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "download message" <<
	    map["dmID"].toString() << "...";

	QString errmsg;
	qdatovka_error ret;
	const QString username = map["username"].toString();

	MessageDb *messageDb =
	    MainWindow::accountMessageDb(username, 0);

	if (!isdsSessions.isConnectedToIsds(username)) {
		if (!MainWindow::connectToIsds(username, 0)) {
			qDebug() << CLI_PREFIX << "connection error:"
			  << isds_long_message(isdsSessions.session(username));
			return CLI_RET_ERROR_CODE;
		}
	}

	if (map["dmType"].toString() == "received") {

		ret = Worker::downloadMessage(username,
		    map["dmID"].toLongLong(), true, MSG_RECEIVED,
		    *messageDb, errmsg, NULL, 0, 0);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "received message" <<
			    map["dmID"].toString() << "has been downloaded.";
			return CLI_RET_OK_CODE;
		} else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "received message! Error code:"
			    << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}

	} else if (map["dmType"].toString() == "sent") {

		ret = Worker::downloadMessage(username,
		    map["dmID"].toLongLong(), true, MSG_SENT,
		    *messageDb, errmsg, NULL, 0, 0);

		if (Q_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "sent message" <<
			    map["dmID"].toString() << "has been downloaded.";
			return CLI_RET_OK_CODE;
		} else {
			qDebug() << CLI_PREFIX << "error while downloading "
			    "received message! Error code:"
			    << ret << errmsg;
			return CLI_RET_ERROR_CODE;
		}
	} else {
		qDebug() << CLI_PREFIX << "wrong dmType value:" <<
		    map["dmType"].toString();
		return CLI_RET_ERROR_CODE;
	}

	return CLI_RET_OK_CODE;
}


/* ========================================================================= */
int createAndSendMsg(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	QString errmsg;

	qDebug() << CLI_PREFIX << "create a new message...";

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

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			errmsg = "Out of memory.";
			goto finish;
		}
		memset(document, 0, sizeof(struct isds_document));

		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;

		QFileInfo fi(map["dmAttachment"].toStringList().at(i));
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

		QFile file(map["dmAttachment"].toStringList().at(i));
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

	sent_envelope->dbIDRecipient =
	    strdup(map["dbIDRecipient"].toString().toUtf8().constData());
	if (NULL == sent_envelope->dbIDRecipient) {
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
		    map["dmPersonalDelivery"].toBool();
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
		    map["dmAllowSubstDelivery"].toBool();
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
		    map["dmOVM"].toBool();
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
		    map["dmPublishOwnID"].toBool();
	} else {
		*sent_envelope->dmPublishOwnID = false;
	}

	sent_message->documents = documents; documents = NULL;
	sent_message->envelope = sent_envelope; sent_envelope = NULL;

	qDebug() << CLI_PREFIX << "sending of a new message...";

	if (!isdsSessions.isConnectedToIsds(map["username"].toString())) {
		if (!MainWindow::connectToIsds(map["username"].toString(), 0)) {
			errmsg = isds_long_message(
			    isdsSessions.session(map["username"].toString()));
			goto finish;
		}
	}

	status = isds_send_message(
	    isdsSessions.session(map["username"].toString()), sent_message);

	errmsg =
	    isds_long_message(isdsSessions.session(map["username"].toString()));

finish:

	int ret = CLI_RET_ERROR_CODE;

	if (IE_SUCCESS == status) {
		/* Added a new message into database */
		qint64 dmId = QString(sent_message->envelope->dmID).toLongLong();
		MessageDb *messageDb =
		    MainWindow::accountMessageDb(map["username"].toString(), 0);
		const QString dbIDSender =
		    globAccountDb.dbId(map["username"].toString() + "___True");
		const QString dmSender =
		    globAccountDb.senderNameGuess(map["username"].toString()
		    + "___True");
		messageDb->msgsInsertNewlySentMessageEnvelope(dmId,
			    dbIDSender,
			    dmSender,
			    map["dbIDRecipient"].toString(),
			    "Databox ID: " + map["dbIDRecipient"].toString(),
			    "unknown",
			    map["dmAnnotation"].toString());

		qDebug() << CLI_PREFIX << "message has been sent; "
		    "dmID:" << sent_message->envelope->dmID;
		ret = CLI_RET_OK_CODE;
	} else {
		qDebug() << CLI_PREFIX << "error while sending of message! "
		    "Error code:" << status << errmsg;
	}

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
	if (service == SER_CONNECT) {
		return connectAttrs.contains(attribute);
	} else if (service == SER_GET_MSG_LIST) {
		return getMsgListAttrs.contains(attribute);
	} else if (service == SER_SEND_MSG) {
		return sendMsgAttrs.contains(attribute);
	} else if (service == SER_DWNLD_MSG) {
		return dwnldMsgAttrs.contains(attribute);
	} else if (service == SER_DWNLD_DEL_INFO) {
		return dwnldDelInfoAttrs.contains(attribute);
	} else if (service == SER_GET_USER_INFO ||
	    service == SER_GET_OWNER_INFO) {
		return (attribute == databoxInfoAttrs);
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
int checkConnectMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "connect parameters...";

	if (!map.contains("method") ||
	    map.value("method").toString().isEmpty()) {
		errmsg = createErrorMsg("method attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("type") ||
	    map.value("type").toString().isEmpty()) {
		errmsg = createErrorMsg("type attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("password") ||
	    map.value("password").toString().isEmpty()) {
		errmsg = createErrorMsg("password attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	QString method = map.value("method").toString();

	if (method == L_HOTP || method == L_HOTP) {
		if (!map.contains("otpcode") ||
		    map.value("otpcode").toString().isEmpty()) {
			errmsg = createErrorMsg("otpcode attribute missing or "
			    "contains empty security code.");
			qDebug() << errmsg;
			return ret;
		}
	} else if (method == L_CERT) {
		if (!map.contains("certificate") ||
		    map.value("certificate").toString().isEmpty()) {
			errmsg = createErrorMsg("certificate attribute missing"
			    " or contains empty certificate path.");
			qDebug() << errmsg;
			return ret;
		}
	} else if (method == L_USER) {

	} else {
		return ret;
	}

	/* CALL LIBISDS LOGIN METHOD */
	ret = 0;

	return ret;

}


/* ========================================================================= */
int checkSendMsgMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "send message parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dbIDRecipient") ||
	    map.value("dbIDRecipient").toString().isEmpty() ||
	    map.value("dbIDRecipient").toString().length() != 7) {
		errmsg = createErrorMsg("databox ID attribute of recipient "
		    "missing or contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dmAnnotation") ||
	    map.value("dmAnnotation").toString().isEmpty()) {
		errmsg = createErrorMsg("subject attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dmAttachment") ||
	    map.value("dmAttachment").toStringList().isEmpty()) {
		errmsg = createErrorMsg("attachment attribute missing or "
		    "contains empty file path list.");
		qDebug() << errmsg;
		return ret;
	}

	return createAndSendMsg(map);
}


/* ========================================================================= */
int checkGetMsgListMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "get message list parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = createErrorMsg("message type attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED) &&
	    !(dmType == MT_SENT_RECEIVED)) {
		errmsg = createErrorMsg("message type attribute "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	return getMsgList(map);
}


/* ========================================================================= */
int checkDownloadMsgMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "download message parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = createErrorMsg("message ID attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = createErrorMsg("message type attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED)) {
		errmsg = createErrorMsg("message type attribute "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	return downloadMsg(map);
}


/* ========================================================================= */
int checkDownloadDeliveryMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "download delivery info parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = createErrorMsg("message ID attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	/* CALL LIBISDS DOWNLOAD DELIVERY INFO METHOD */
	ret = 0;

	return ret;
}


/* ========================================================================= */
int checkGetUserInfoMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "get user info parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	/* CALL LIBISDS GET USER INFO METHOD */
	ret = 0;

	return ret;
}


/* ========================================================================= */
int checkGetOwnerInfoMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory "
	    "get owner info parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}

	/* CALL LIBISDS GET OWNER INFO METHOD */
	ret = 0;

	return ret;
}


/* ========================================================================= */
int runService(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() <<  CLI_PREFIX << "input:" << service << ":" << paramString;

	QMap <QString, QVariant> map;

	QString attribute = "";
	QString value = "";
	QString errmsg;
	bool newAttribute = true;
	bool newValue = false;
	bool special = false;
	int attrPosition = 0;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "parsing input string with parameters...";

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
					return ret;
				}
				if (value.isEmpty()) {
					errmsg = createErrorMsg(
					    QString("empty attribute "
					    "value on position '%1'").
					    arg(attrPosition));
					qDebug() << errmsg;
					return ret;
				}

				if (checkAttributeIfExists(service,attribute)) {
					if (attribute == ATTACH_LABEL) {
						map[attribute] =
						    parseAttachment(value);
					} else {
						map[attribute] = value;
					}
				} else {
					errmsg = createErrorMsg(
					    QString("unknown attribute "
					    "name '%1'").arg(attribute));
					qDebug() << errmsg;
					return ret;
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
				special = true;
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
		return ret;
	}
	if (value.isEmpty()) {
		errmsg = createErrorMsg(QString("empty attribute "
		    "value on position '%1'").arg(attrPosition));
		qDebug() << errmsg;
		return ret;
	}
	if (checkAttributeIfExists(service, attribute)) {
		if (attribute == ATTACH_LABEL) {
			map[attribute] = parseAttachment(value);
		} else {
			map[attribute] = value;
		}
	} else {
		errmsg = createErrorMsg(QString("unknown attribute "
		    "name '%1'").arg(attribute));
		qDebug() << errmsg;
		return ret;
	}

	// add service name to map
	map[SERVICE_LABEL] = service;

	if (service == SER_CONNECT) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_GET_MSG_LIST) {
		ret = checkGetMsgListMandatoryAttributes(map);
	} else if (service == SER_SEND_MSG) {
		ret = checkSendMsgMandatoryAttributes(map);
	} else if (service == SER_DWNLD_MSG) {
		ret = checkDownloadMsgMandatoryAttributes(map);
	} else if (service == SER_DWNLD_DEL_INFO) {
		ret = checkDownloadDeliveryMandatoryAttributes(map);
	} else if (service == SER_GET_USER_INFO) {
		ret = checkGetUserInfoMandatoryAttributes(map);
	} else if (service == SER_GET_OWNER_INFO) {
		ret = checkGetOwnerInfoMandatoryAttributes(map);
	} else {
		return ret;
	}

	return ret;
}
