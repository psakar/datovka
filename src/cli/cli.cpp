/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include <cstdlib>
#include <QDebug>
#include <QTextStream>

#include "src/cli/cli.h"
#include "src/cli/cli_login.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/io/isds_helper.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/model_interaction/account_interaction.h"
#include "src/worker/pool.h"
#include "src/worker/task_download_message.h"
#include "src/worker/task_download_message_list.h"
#include "src/worker/task_search_owner.h"

const QSet<QString> serviceSet = QSet<QString>() << SER_LOGIN <<
SER_GET_MSG_LIST << SER_SEND_MSG << SER_GET_MSG << SER_GET_DEL_INFO <<
SER_GET_USER_INFO << SER_GET_OWNER_INFO << SER_CHECK_ATTACHMENT <<
SER_GET_MSG_IDS << SER_FIND_DATABOX;

// Known attributes definition
const QStringList connectAttrs = QStringList()
    << "username" << "password" << "certificate" << "otpcode";
const QStringList getMsgListAttrs = QStringList()
    << "dmType" << "dmStatusFilter" << "dmLimit" << "dmFromTime" << "dmToTime"
    << "complete";
const QStringList sendMsgAttrs = QStringList()
    << "dbIDRecipient" << "dmAnnotation" << "dmToHands"
    << "dmRecipientRefNumber" << "dmSenderRefNumber" << "dmRecipientIdent"
    << "dmSenderIdent" << "dmLegalTitleLaw" << "dmLegalTitleYear"
    << "dmLegalTitleSect" << "dmLegalTitlePar" << "dmLegalTitlePoint"
    << "dmPersonalDelivery" << "dmAllowSubstDelivery" << "dmType" << "dmOVM"
    << "dmPublishOwnID" << "dmAttachment";
const QStringList getMsgAttrs = QStringList()
    << "dmID" << "dmType" << "zfoFile" << "download" << "markDownload"
    << "attachmentDir";
const QStringList getDelInfoAttrs = QStringList() << "dmID" << "zfoFile"
    << "download";
const QStringList getMsgIdsAttrs = QStringList() << "dmType";
const QStringList findDataboxAttrs = QStringList() << "dbType" << "dbID"
    << "ic" << "firmName" << "pnFirstName" << "pnLastName" << "adZipCode";

/* ========================================================================= */
static
void printDataToStdOut(const QStringList &data)
/* ========================================================================= */
{
	QTextStream cout(stdout);

	for (int i = 0; i < data.count(); ++i) {
		if (i == (data.count() - 1)) {
			cout << data.at(i) << endl << endl;
		} else {
			cout << data.at(i) << " ";
		}
	}
}

static
void printDataToStdOut(const QList<qint64> &data)
{
	QTextStream cout(stdout);

	for (int i = 0; i < data.count(); ++i) {
		if (i == (data.count() - 1)) {
			cout << data.at(i) << endl << endl;
		} else {
			cout << data.at(i) << " ";
		}
	}
}

/* ========================================================================= */
void printErrToStdErr(const cli_error err, const QString errmsg)
/* ========================================================================= */
{
	/* TODO - print error code and error message */

	QTextStream cout(stderr);
	cout << CLI_PREFIX << " error(" << err << ") : " << errmsg << endl;
}

/* ========================================================================= */
const QString createErrorMsg(const QString &msg)
/* ========================================================================= */
{
	return QString(CLI_PREFIX) + QString(PARSER_PREFIX) + msg;
}

/* ========================================================================= */
static void isds_document_free_void(void **document)
/* ========================================================================= */
{
	isds_document_free((struct isds_document **) document);
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
	} else if (service == SER_GET_MSG_IDS) {
		return getMsgIdsAttrs.contains(attribute);
	} else if (service == SER_FIND_DATABOX) {
		return findDataboxAttrs.contains(attribute);
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
cli_error getMsgList(const QMap<QString,QVariant> &map, MessageDbSet *msgDbSet,
    QString &errmsg)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Downloading of message list for username"
	    << username;

	/* messages counters */
	QList<qint64> newMsgIdList;
	bool complete = false;
	unsigned long dmLimit = 0;
	uint dmStatusFilter = MESSAGESTATE_ANY;
	bool ok;

	if (map.contains("dmStatusFilter")) {
		uint number = map["dmStatusFilter"].toString().toUInt(&ok);
		if (!ok) {
			errmsg = "Wrong dmStatusFilter value: " +
			    map["dmStatusFilter"].toString();
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ATR_VAL_ERR;
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

	if (map.contains("complete")) {
		QString compValue = map.value("complete").toString();
		if (!(compValue == "no") && !(compValue == "yes")) {
			errmsg = "complete attribute has wrong value "
			    "(no,yes is required)";
			qDebug() << createErrorMsg(errmsg);
			return CLI_ATR_VAL_ERR;
		}
		complete = (compValue == "yes") ? true : false;
	}

	if (map.contains("dmLimit")) {
		dmLimit = map["dmLimit"].toString().toULong(&ok);
		if (!ok) {
			errmsg = "Wrong dmLimit value: " +
			    map["dmLimit"].toString();
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ATR_VAL_ERR;
		}
	}

	if (0 == dmLimit) {
		/* Increase limit. */
		dmLimit = MESSAGE_LIST_LIMIT;
	}

	if ((map["dmType"].toString() != MT_RECEIVED) &&
	    (map["dmType"].toString() != MT_SENT) &&
	    (map["dmType"].toString() != MT_SENT_RECEIVED)) {
		errmsg = "Wrong dmType value: " + map["dmType"].toString();
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ATR_VAL_ERR;
	}

	QString err, longErr;
	if ((map["dmType"].toString() == MT_RECEIVED) ||
	    (map["dmType"].toString() == MT_SENT_RECEIVED)) {
		TaskDownloadMessageList *task;

		task = new (std::nothrow) TaskDownloadMessageList(
		    username, msgDbSet, MSG_RECEIVED, complete, dmLimit,
		    dmStatusFilter);
		task->setAutoDelete(false);
		globWorkPool.runSingle(task);

		bool success =
		   TaskDownloadMessageList::DL_SUCCESS == task->m_result;

		if (success) {
			newMsgIdList += task->m_newMsgIdList;
			qDebug() << CLI_PREFIX <<
			    "Received message list has been downloaded";
		} else {
			errmsg =
			    "Error while downloading received message list";
			qDebug() << CLI_PREFIX << errmsg << "Error code:" <<
			    task->m_result << task->m_isdsError <<
			    task->m_isdsLongError;
		}

		delete task;

		if (!success) {
			/* Stop pending jobs. */
			globWorkPool.stop();
			globWorkPool.clear();
			return CLI_ERROR;
		}
	}

	if ((map["dmType"].toString() == MT_SENT) ||
	    (map["dmType"].toString() == MT_SENT_RECEIVED)) {
		TaskDownloadMessageList *task;

		task = new (std::nothrow) TaskDownloadMessageList(
		    username, msgDbSet, MSG_SENT, complete, dmLimit,
		    dmStatusFilter);
		task->setAutoDelete(false);
		globWorkPool.runSingle(task);

		bool success =
		   TaskDownloadMessageList::DL_SUCCESS == task->m_result;

		if (success) {
			newMsgIdList += task->m_newMsgIdList;
			qDebug() << CLI_PREFIX <<
			    "Sent message list has been downloaded";
		} else {
			errmsg = "Error while downloading sent message list";
			qDebug() << CLI_PREFIX << errmsg << "Error code:" <<
			    task->m_result << task->m_isdsError <<
			    task->m_isdsLongError;
		}

		delete task;

		if (!success) {
			/* Stop pending jobs. */
			globWorkPool.stop();
			globWorkPool.clear();
			return CLI_ERROR;
		}
	}

	/* Wait for possible pending jobs. */
	globWorkPool.wait();

	printDataToStdOut(newMsgIdList);
	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error getMsg(const QMap<QString,QVariant> &map, MessageDbSet *msgDbSet,
    bool needsISDS, QString &errmsg)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "Downloading of message" <<
	    map["dmID"].toString();

	QString err, longErr;
	TaskDownloadMessage::Result ret;
	const QString username = map["username"].toString();

	MessageDb::MsgId msgId =
	    msgDbSet->msgsMsgId(map["dmID"].toLongLong());
	if (msgId.dmId < 0) {
		errmsg = "Message does not exist in the database "
		    "for user " + username;
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ERROR;
	}
	MessageDb *messageDb =
	    msgDbSet->accessMessageDb(msgId.deliveryTime, false);
	if (messageDb == NULL) {
		errmsg = "Database doesn't exists for user " + username;
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ERROR;
	}

	if (needsISDS) {
		if (map["dmType"].toString() == MT_RECEIVED) {

			ret = TaskDownloadMessage::downloadMessage(username,
			    msgId, true, MSG_RECEIVED,
			    *msgDbSet, err, longErr, NULL);

			if (TaskDownloadMessage::DM_SUCCESS == ret) {
				qDebug() << CLI_PREFIX << "Received message" <<
				    map["dmID"].toString() << "has been "
				    "downloaded";
			} else {
				errmsg = "Error while downloading received "
				    "message";
				qDebug() << CLI_PREFIX << errmsg <<
				    "Error code:" << ret << err;
				return CLI_ERROR;
			}

		} else if (map["dmType"].toString() == MT_SENT) {

			ret = TaskDownloadMessage::downloadMessage(username,
			    msgId, true, MSG_SENT,
			    *msgDbSet, err, longErr, NULL);

			if (TaskDownloadMessage::DM_SUCCESS == ret) {
				qDebug() << CLI_PREFIX << "Sent message" <<
				    map["dmID"].toString() << "has been "
				    "downloaded";
			} else {
				errmsg = "Error while downloading sent "
				    "message";
				qDebug() << CLI_PREFIX << errmsg <<
				    "Error code:" << ret << err << longErr;
				return CLI_ERROR;
			}
		} else {
			errmsg = "Wrong dmType value: " +
			    map["dmType"].toString();
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ATR_VAL_ERR;
		}
	}

	if (map.contains("attachmentDir") &&
	    !map["attachmentDir"].toString().isEmpty()) {
		const QFileInfo fi(map["attachmentDir"].toString());
		const QString path = fi.path();
		if (!QDir(path).exists()) {
			errmsg = "Wrong path " + path + " for file saving";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}

		QList<MessageDb::FileData> files =
		    messageDb->getFilesFromMessage(map["dmID"].toLongLong());

		foreach (const MessageDb::FileData &file, files) {

			QString fileName = file.dmFileDescr;
			if (fileName.isEmpty()) {
				errmsg = "Cannot save file because name "
				    "of file missing";
				qDebug() << CLI_PREFIX << errmsg;
				return CLI_ERROR;
			}

			if (file.dmEncodedContent.isEmpty()) {
				errmsg = "Cannot save file " + fileName +
				    "because file content missing";
				qDebug() << CLI_PREFIX << errmsg;
				return CLI_ERROR;
			}

			fileName = path + QDir::separator() + fileName;

			QByteArray data = QByteArray::fromBase64(file.dmEncodedContent);
			enum WriteFileState ret = writeFile(fileName, data);
			if (WF_SUCCESS == ret) {
				qDebug() << CLI_PREFIX << "Save file" <<
				    fileName << "of message" <<
				    map["dmID"].toString();
			} else {
				errmsg = "Saving error of file " + fileName +
				    " of message " + map["dmID"].toString() +
				    " failed";
				qDebug() << CLI_PREFIX << errmsg;
				return CLI_ERROR;
			}
		}
	}

	if (map.contains("zfoFile") && !map["zfoFile"].toString().isEmpty()) {
		const QFileInfo fi(map["zfoFile"].toString());
		const QString path = fi.path();
		if (!QDir(path).exists()) {
			errmsg = "Wrong path " + path + " for file saving";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}

		QByteArray base64 =
		    messageDb->msgsMessageBase64(map["dmID"].toLongLong());

		if (base64.isEmpty()) {
			errmsg = "Cannot export complete message to ZFO";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}

		QByteArray data = QByteArray::fromBase64(base64);
		enum WriteFileState ret = writeFile(map["zfoFile"].toString(),
		    data);
		if (WF_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Export of message" <<
			map["dmID"].toString() <<  "to ZFO was successful.";
		} else {
			errmsg = "Export of message " + map["dmID"].toString() +
			    " to ZFO was NOT successful";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}
	}

	if (map.contains("markDownload")) {
		if (map.value("markDownload").toString() == "yes") {
			messageDb->smsgdtSetLocallyRead(
			    map["dmID"].toLongLong(), true);
		}
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error getDeliveryInfo(const QMap<QString,QVariant> &map,
    MessageDbSet *msgDbSet, bool needsISDS, QString &errmsg)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "Downloading of delivery info for message" <<
	    map["dmID"].toString();

	const QString username = map["username"].toString();

	MessageDb::MsgId msgId =
	    msgDbSet->msgsMsgId(map["dmID"].toLongLong());
	if (msgId.dmId < 0) {
		errmsg = "Message does not exist in the database "
		    "for user " + username;
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ERROR;
	}
	MessageDb *messageDb =
	    msgDbSet->accessMessageDb(msgId.deliveryTime, false);
	if (messageDb == NULL) {
		errmsg = "Database doesn't exists for user " + username;
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ERROR;
	}

	if (needsISDS) {
		QString isdsError, isdsLongError;
		if (TaskDownloadMessage::DM_SUCCESS ==
		    TaskDownloadMessage::downloadDeliveryInfo(username,
		        map["dmID"].toLongLong(), true, *msgDbSet, isdsError,
		        isdsLongError)) {
			qDebug() << CLI_PREFIX << "Delivery info of message"
			    << map["dmID"].toString() << "has been downloaded."
			    << isdsError << isdsLongError;
		} else {
			errmsg = "Error while downloading delivery info";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}
	}

	if (map.contains("zfoFile") && !map["zfoFile"].toString().isEmpty()) {
		const QFileInfo fi(map["zfoFile"].toString());
		const QString path = fi.path();
		if (!QDir(path).exists()) {
			errmsg = "Wrong path " + path + " for file saving";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}

		QByteArray base64 = messageDb->msgsGetDeliveryInfoBase64(
		    map["dmID"].toLongLong());

		if (base64.isEmpty()) {
			errmsg = "Cannot export delivery info to ZFO";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}

		QByteArray data = QByteArray::fromBase64(base64);
		enum WriteFileState ret = writeFile(map["zfoFile"].toString(),
		    data);
		if (WF_SUCCESS == ret) {
			qDebug() << CLI_PREFIX << "Export of delivery info" <<
			map["dmID"].toString() <<  "to ZFO was successful.";
		} else {
			errmsg = "Export of delivery info "
			     + map["dmID"].toString() +
			    " to ZFO was NOT successful";
			qDebug() << CLI_PREFIX << errmsg;
			return CLI_ERROR;
		}
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkAttachment(const QMap<QString,QVariant> &map,
    MessageDbSet *msgDbSet)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Checking of missing messages attachment for"
	    " username" <<  username;

	printDataToStdOut(msgDbSet->getAllMessageIDsWithoutAttach());

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error getMsgIds(const QMap<QString,QVariant> &map,
    MessageDbSet *msgDbSet, QString &errmsg)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Get list of message IDs "
	    "from local database for username" <<  username;

	if ((map["dmType"].toString() != MT_RECEIVED) &&
	    (map["dmType"].toString() != MT_SENT) &&
	    (map["dmType"].toString() != MT_SENT_RECEIVED)) {
		errmsg = "Wrong dmType value: " + map["dmType"].toString();
		qDebug() << CLI_PREFIX << errmsg;
		return CLI_ATR_VAL_ERR;
	}

	if (map["dmType"].toString() == MT_RECEIVED) {
		printDataToStdOut(msgDbSet->getAllMessageIDs(MessageDb::TYPE_RECEIVED));
	} else if (map["dmType"].toString() == MT_SENT) {
		printDataToStdOut(msgDbSet->getAllMessageIDs(MessageDb::TYPE_SENT));
	} else {
		printDataToStdOut(msgDbSet->getAllMessageIDs(MessageDb::TYPE_RECEIVED));
		printDataToStdOut(msgDbSet->getAllMessageIDs(MessageDb::TYPE_SENT));
	}
	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error getUserInfo(const QMap<QString,QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "Downloading info about username"
	    << username;

	if (IsdsHelper::getUserInfoFromLogin(map["username"].toString())) {
		return CLI_SUCCESS;
	}

	errmsg = "Cannot download user info";
	return CLI_ERROR;
}

/* ========================================================================= */
cli_error getOwnerInfo(const QMap <QString, QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "downloading info about owner and its "
	    "databox for username" <<  username;

	if (IsdsHelper::getOwnerInfoFromLogin(map["username"].toString())) {
		return CLI_SUCCESS;
	}

	errmsg = "Cannot download owner info";
	return CLI_ERROR;
}

/* ========================================================================= */
cli_error findDatabox(const QMap <QString, QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	const QString username = map["username"].toString();

	qDebug() << CLI_PREFIX << "find info about databox from username"
	    <<  username;

	enum TaskSearchOwner::BoxType boxType = TaskSearchOwner::BT_OVM;

	/* set type of search databox */
	QString dbTypeTmp = map.value("dbType").toString();
	if (dbTypeTmp == DB_FO) {
		boxType = TaskSearchOwner::BT_FO;
	} else if (dbTypeTmp == DB_PFO) {
		boxType = TaskSearchOwner::BT_PFO;
	} else if (dbTypeTmp == DB_PO) {
		boxType = TaskSearchOwner::BT_PO;
	} else {
		boxType = TaskSearchOwner::BT_OVM;
	}

	TaskSearchOwner::SoughtOwnerInfo soughtInfo(
	    map.contains("dbID") ? map.value("dbID").toString() : QString(),
	    boxType,
	    map.contains("ic") ? map.value("ic").toString() : QString(),
	    map.contains("pnFirstName") ? map.value("pnFirstName").toString() : QString(),
	    map.contains("pnLastName") ? map.value("pnLastName").toString() : QString(),
	    map.contains("firmName") ? map.value("firmName").toString() : QString(),
	    map.contains("adZipCode") ? map.value("adZipCode").toString() : QString());

	QList<TaskSearchOwner::BoxEntry> foundBoxes;
	QString errMsg;
	QString longErrMsg;
	enum TaskSearchOwner::Result result = TaskSearchOwner::isdsSearch(
	    username, soughtInfo, foundBoxes, errMsg, longErrMsg);

	if (TaskSearchOwner::SO_SUCCESS != result) {
		errmsg = longErrMsg;
		return CLI_ERROR;
	}

	foreach (const TaskSearchOwner::BoxEntry &entry, foundBoxes) {
		QStringList contact;
		contact.append(entry.id);
		contact.append("|" + entry.name + "|" + entry.address +
		    "|" + entry.zipCode);
		printDataToStdOut(contact);
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
static
struct isds_list *buildDocuments(const QStringList &filePaths)
/* ========================================================================= */
{
	struct isds_document *document = NULL; /* Attachment. */
	struct isds_list *documents = NULL; /* Attachment list (entry). */
	struct isds_list *last = NULL; /* No need to free it explicitly. */

	bool mainFile = true;

	foreach (const QString &filePath, filePaths) {

		if (filePath.isEmpty()) {
			continue;
		}

		QFileInfo fi(filePath);
		if (!fi.isReadable() || !fi.isFile()) {
			logErrorNL(CLI_PREFIX "Wrong file name '%s' or file is missing.",
			    filePath.toUtf8().constData());
			goto fail;
		}

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
		memset(document, 0, sizeof(struct isds_document));

		// TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;

		QString name = fi.fileName();
		document->dmFileDescr = strdup(name.toUtf8().constData());
		if (NULL == document->dmFileDescr) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}

		if (mainFile) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
			mainFile = false;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

		document->dmMimeType = strdup("");
		if (NULL == document->dmMimeType) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}

		QFile file(QDir::fromNativeSeparators(filePath));
		if (file.exists()) {
			if (!file.open(QIODevice::ReadOnly)) {
				logErrorNL(CLI_PREFIX "Couldn't open file '%s'.",
				    filePath.toUtf8().constData());
				goto fail;
			}
		}
		QByteArray bytes = file.readAll();
		document->data_length = bytes.size();
		document->data = malloc(bytes.size());
		if (NULL == document->data) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
		memcpy(document->data, bytes.data(), document->data_length);

		/* Add document on the list of document. */
		struct isds_list *newListItem = (struct isds_list *)
		    malloc(sizeof(struct isds_list));
		if (NULL == newListItem) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
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

	return documents;

fail:
	isds_document_free(&document);
	isds_list_free(&documents);
	return NULL;
}

/* ========================================================================= */
struct isds_envelope *buildEnvelope(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	struct isds_envelope *envelope = NULL; /* Message envelope. */

	envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));
	if (envelope == NULL) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}
	memset(envelope, 0, sizeof(struct isds_envelope));

	/* Set mandatory fields of envelope. */
	envelope->dmID = NULL;
	envelope->dmAnnotation = strdup(
	    map["dmAnnotation"].toString().toUtf8().constData());
	if (NULL == envelope->dmAnnotation) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}

	/* Set optional fields. */
	if (map.contains("dmSenderIdent")) {
		envelope->dmSenderIdent = strdup(
		    map["dmSenderIdent"].toString().toUtf8().constData());
		if (NULL == envelope->dmSenderIdent) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmRecipientIdent")) {
		envelope->dmRecipientIdent = strdup(
		    map["dmRecipientIdent"].toString().toUtf8().constData());
		if (NULL == envelope->dmRecipientIdent) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmSenderRefNumber")) {
		envelope->dmSenderRefNumber = strdup(
		    map["dmSenderRefNumber"].toString().toUtf8().constData());
		if (NULL == envelope->dmSenderRefNumber) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmRecipientRefNumber")) {
		envelope->dmRecipientRefNumber = strdup(
		    map["dmRecipientRefNumber"].toString().toUtf8().constData());
		if (NULL == envelope->dmRecipientRefNumber) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmToHands")) {
		envelope->dmToHands = strdup(
		    map["dmToHands"].toString().toUtf8().constData());
		if (NULL == envelope->dmToHands) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmLegalTitleLaw")) {
		envelope->dmLegalTitleLaw =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleLaw) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleLaw =
		    map["dmLegalTitleLaw"].toLongLong();
	} else {
		envelope->dmLegalTitleLaw = NULL;
	}

	if (map.contains("dmLegalTitleYear")) {
		envelope->dmLegalTitleYear =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleYear) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleYear =
		    map["dmLegalTitleYear"].toLongLong();
	} else {
		envelope->dmLegalTitleYear = NULL;
	}

	if (map.contains("dmLegalTitleSect")) {
		envelope->dmLegalTitleSect = strdup(
		    map["dmLegalTitleSect"].toString().toUtf8().constData());
		if (NULL == envelope->dmLegalTitleSect) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmLegalTitlePar")) {
		envelope->dmLegalTitlePar = strdup(
		    map["dmLegalTitlePar"].toString().toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePar) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmLegalTitlePoint")) {
		envelope->dmLegalTitlePoint = strdup(
		    map["dmLegalTitlePoint"].toString().toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePoint) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	if (map.contains("dmType")) {
		envelope->dmType = strdup(
		    map["dmType"].toString().toUtf8().constData());
		if (NULL == envelope->dmType) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto fail;
		}
	}

	envelope->dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPersonalDelivery) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}
	if (map.contains("dmPersonalDelivery")) {
		*envelope->dmPersonalDelivery =
		    map["dmPersonalDelivery"].toString() != "0";
	} else {
		*envelope->dmPersonalDelivery = false;
	}

	envelope->dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmAllowSubstDelivery) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}
	if (map.contains("dmAllowSubstDelivery")) {
		*envelope->dmAllowSubstDelivery =
		    map["dmAllowSubstDelivery"].toString() != "0";
	} else {
		*envelope->dmAllowSubstDelivery = true;
	}

	envelope->dmOVM = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmOVM) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}
	if (map.contains("dmOVM")) {
		*envelope->dmOVM = map["dmOVM"].toString() != "0";
	} else {
		*envelope->dmOVM = true;
	}

	envelope->dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPublishOwnID) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto fail;
	}
	if (map.contains("dmPublishOwnID")) {
		*envelope->dmPublishOwnID =
		    map["dmPublishOwnID"].toString() != "0";
	} else {
		*envelope->dmPublishOwnID = false;
	}

	return envelope;

fail:
	isds_envelope_free(&envelope);
	return NULL;
}

/* ========================================================================= */
cli_error createAndSendMsg(const QMap <QString, QVariant> &map,
    MessageDbSet *msgDbSet, QString &errmsg)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	cli_error ret = CLI_ERROR;
	QStringList sendID;

	qDebug() << CLI_PREFIX << "creating a new message...";

	/* Sent message. */
	struct isds_message *sent_message = NULL;

	sent_message = (struct isds_message *)
	    malloc(sizeof(struct isds_message));
	if (sent_message == NULL) {
		logErrorNL(CLI_PREFIX "%s", "Memory allocation failed.");
		goto finish;
	}
	memset(sent_message, 0, sizeof(struct isds_message));

	/* Attach envelope and attachment files to message structure. */
	sent_message->documents =
	    buildDocuments(map["dmAttachment"].toStringList());
	if (NULL == sent_message->documents) {
		goto finish;
	}
	sent_message->envelope = buildEnvelope(map);
	if (NULL == sent_message->envelope) {
		goto finish;
	}

	foreach (const QString &recipientId, map.value("dbIDRecipient").toStringList()) {

		logInfo(CLI_PREFIX "Sending message to '%s'.\n",
		    recipientId.toUtf8().constData());

		if (NULL != sent_message->envelope->dbIDRecipient) {
			free(sent_message->envelope->dbIDRecipient);
			sent_message->envelope->dbIDRecipient = NULL;
		}

		sent_message->envelope->dbIDRecipient =
		    strdup(recipientId.toUtf8().constData());
		if (NULL == sent_message->envelope->dbIDRecipient) {
			logErrorNL(CLI_PREFIX "%s",
			    "Memory allocation failed.");
			goto finish;
		}

		struct isds_ctx *session =
		    globIsdsSessionsPtr->session(map["username"].toString());
		if (NULL == session) {
			Q_ASSERT(0);
			ret = CLI_ERROR;
			goto finish;
		}

		status = isds_send_message(session, sent_message);
		QString err = isdsLongMessage(session);

		if (IE_SUCCESS == status) {
			/* Store new message into database. */
			qint64 dmId =
			    QString(sent_message->envelope->dmID).toLongLong();
			const QString acntDbKey(AccountDb::keyFromLogin(
			    map["username"].toString()));
			const QString dbIDSender(
			    globAccountDbPtr->dbId(acntDbKey));
			const QString dmSender(
			    globAccountDbPtr->senderNameGuess(acntDbKey));
			QDateTime deliveryTime = timevalToDateTime(
			    sent_message->envelope->dmDeliveryTime);
			MessageDb *messageDb =
			    msgDbSet->accessMessageDb(deliveryTime, true);
			if (messageDb == NULL) {
				errmsg = "Message has been sent but "
				    "it was not stored to database. "
				    "Database doesn't exists for user "
				    + map["username"].toString();
				qDebug() << CLI_PREFIX << errmsg;
			} else {
				messageDb->msgsInsertNewlySentMessageEnvelope(
				    dmId, dbIDSender, dmSender, recipientId,
				    "Databox ID: " + recipientId, "unknown",
				    map["dmAnnotation"].toString());
			}
			qDebug() << CLI_PREFIX << "message has been sent"
			    << QString(sent_message->envelope->dmID);
			sendID.append(sent_message->envelope->dmID);
			ret = CLI_SUCCESS;
		} else {
			errmsg = "Error while sending message! ISDS says: "
			    + err;
			qDebug() << CLI_PREFIX << errmsg << "Error code:"
			    << status;
			ret = CLI_ERROR;
		}
	}

	printDataToStdOut(sendID);

finish:
	isds_message_free(&sent_message);

	return ret;
}

/* ========================================================================= */
cli_error checkLoginMandatoryAttributes(const QMap<QString,QVariant> &map,
    QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters for login...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = "Username attribute missing or contains wrong value. "
		"Username must be exactly 6 chars long.";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkSendMsgMandatoryAttributes(const QMap<QString,QVariant> &map,
    QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters for send message...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dbIDRecipient") ||
	    map.value("dbIDRecipient").toStringList().isEmpty()) {
		errmsg = "dbIDRecipient attribute missing or "
		    "contains empty databox id list";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}
	QStringList dbIds = map.value("dbIDRecipient").toStringList();
	for (int i = 0; i < dbIds.count(); ++i) {
		if (dbIds.at(i).length() != 7) {
			errmsg = (QString("dbIDRecipient "
			    "attribute contains wrong value at "
			    "position %1! dbIDRecipient must be exactly 7 "
			    "chars long.").arg(i+1));
			qDebug() << createErrorMsg(errmsg);
			return CLI_ATR_VAL_ERR;
		}
	}
	if (!map.contains("dmAnnotation") ||
	    map.value("dmAnnotation").toString().isEmpty()) {
		errmsg = "dmAnnotation attribute missing or "
		    "contains empty string";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}
	if (!map.contains("dmAttachment") ||
	    map.value("dmAttachment").toStringList().isEmpty()) {
		errmsg = "dmAttachment attribute missing or "
		    "contains empty file path list";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkGetMsgListMandatoryAttributes(const QMap<QString,QVariant> &map,
    QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters for "
	    "get message list...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = "dmType attribute missing or "
		    "contains empty string";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}
	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED) &&
	    !(dmType == MT_SENT_RECEIVED)) {
		errmsg = "dmType attribute "
		    "contains wrong value";
		qDebug() << createErrorMsg(errmsg);
		return CLI_ATR_VAL_ERR;
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkGetMsgMandatoryAttributes(const QMap<QString,QVariant> &map,
    QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters "
	    "for download message...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = "dmID attribute missing or "
		    "contains empty string";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}
	if (!map.contains("dmType") ||
	    map.value("dmType").toString().isEmpty()) {
		errmsg = "dmType attribute missing or "
		    "contains empty string";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}
	QString dmType = map.value("dmType").toString();
	if (!(dmType == MT_SENT) && !(dmType == MT_RECEIVED)) {
		errmsg = "dmType attribute "
		    "contains wrong value";
		qDebug() << createErrorMsg(errmsg);
		return CLI_ATR_VAL_ERR;
	}

	if (map.contains("download")) {
		QString download = map.value("download").toString();
		if (!(download == "no") && !(download == "yes")
		    && !(download == "ondemand")) {
			errmsg = "download attribute has wrong value "
			"(no,yes,ondemand is required)";
			qDebug() << createErrorMsg(errmsg);
			return CLI_ATR_VAL_ERR;
		}
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkDownloadDeliveryMandatoryAttributes(
    const QMap<QString,QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters "
	    "for download delivery info...";
	//qDebug() << CLI_PREFIX << errmsg;

	if (!map.contains("dmID") ||
	    map.value("dmID").toString().isEmpty()) {
		errmsg = "dmID attribute missing or "
		    "contains empty string";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}

	if (map.contains("download")) {
		QString download = map.value("download").toString();
		if (!(download == "no") && !(download == "yes")
		    && !(download == "ondemand")) {
			errmsg = "download attribute has "
			    "wrong value (no,yes,ondemand is required)";
			qDebug() << createErrorMsg(errmsg);
			return CLI_ATR_VAL_ERR;
		}
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkFindDataboxMandatoryAttributes(
    const QMap<QString,QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	errmsg = "checking of mandatory parameters "
	    "for find databox...";

	bool isAttr = false;

	/* dbType */
	if (map.contains("dbType")) {
		QString dbType = map.value("dbType").toString();
		if (!(dbType == DB_OVM) && !(dbType == DB_PO)
		    && !(dbType == DB_PFO) && !(dbType == DB_FO)) {
			errmsg = "dbType attribute has wrong value";
			qDebug() << createErrorMsg(errmsg);
			return CLI_ATR_VAL_ERR;
		}
	} else {
		errmsg = "dbType attribute missing";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}

	/* dbID */
	if (map.contains("dbID")) {
		if (map.value("dbID").toString().length() != 7) {
			errmsg = "dbID attribute contains wrong "
			    "value or is empty";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	/* ic */
	if (map.contains("ic")) {
		if (map.value("ic").toString().length() > 8) {
			errmsg = "ic attribute contains wrong "
			    "value or is empty";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	/* firmName */
	if (map.contains("firmName")) {
		if (map.value("firmName").toString().length() < 3) {
			errmsg = "firmName attribute is empty or contains "
			    "less than 3 characters";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	/* pnFirstName */
	if (map.contains("pnFirstName")) {
		if (map.value("pnFirstName").toString().length() < 3) {
			errmsg = "pnFirstName attribute is empty or contains "
			    "less than 3 characters";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	/* pnLastName */
	if (map.contains("pnLastName")) {
		if (map.value("pnLastName").toString().length() < 3) {
			errmsg = "pnLastName attribute is empty or contains "
			    "less than 3 characters";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	/* adZipCode */
	if (map.contains("adZipCode")) {
		if (map.value("adZipCode").toString().length() != 5) {
			errmsg = "adZipCode attribute is empty or contains "
			    "wrong value";
			qDebug() << createErrorMsg(errmsg);
			return CLI_REQ_ATR_ERR;
		}
		isAttr = true;
	}

	if (!isAttr) {
		errmsg = "Not specified some attribute for this service";
		qDebug() << createErrorMsg(errmsg);
		return CLI_REQ_ATR_ERR;
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error checkMandatoryAttributes(const QString &service,
    QMap<QString,QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	if (service == SER_LOGIN) {
		return checkLoginMandatoryAttributes(map, errmsg);
	} else if (service == SER_GET_MSG_LIST) {
		return checkGetMsgListMandatoryAttributes(map, errmsg);
	} else if (service == SER_SEND_MSG) {
		return checkSendMsgMandatoryAttributes(map, errmsg);
	} else if (service == SER_GET_MSG) {
		return checkGetMsgMandatoryAttributes(map, errmsg);
	} else if (service == SER_GET_DEL_INFO) {
		return checkDownloadDeliveryMandatoryAttributes(map, errmsg);
	} else if (service == SER_GET_MSG_IDS) {
		return checkGetMsgListMandatoryAttributes(map, errmsg);
	} else if (service == SER_FIND_DATABOX) {
		return checkFindDataboxMandatoryAttributes(map, errmsg);
	}
	errmsg = "Unknown service name";
	return CLI_UNKNOWN_SER;
}

/* ========================================================================= */
cli_error parsePamamString(const QString &service, const QString &paramString,
    QMap<QString,QVariant> &map, QString &errmsg)
/* ========================================================================= */
{
	QString attribute = "";
	QString value = "";
	cli_error err = CLI_ERROR;
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
					errmsg = QString("empty attribute "
					    "name on position '%1'").
					    arg(attrPosition);
					qDebug() << createErrorMsg(errmsg);
					return CLI_ATR_NAME_ERR;
				}
				if (value.isEmpty()) {
					errmsg = QString("empty attribute "
					    "value on position '%1' or value "
					    "is not between apostrophes").
					    arg(attrPosition);
					qDebug() << createErrorMsg(errmsg);
					return CLI_ATR_VAL_ERR;
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
					errmsg = QString("unknown attribute "
					    "name '%1'").arg(attribute);
					qDebug() << createErrorMsg(errmsg);
					return CLI_UNKNOWN_ATR;
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
		errmsg = QString("empty attribute "
		    "name on position '%1'").arg(attrPosition);
		qDebug() << createErrorMsg(errmsg);
		return CLI_ATR_NAME_ERR;
	}
	if (value.isEmpty()) {
		errmsg = QString("empty attribute value on position '%1' "
		"or value is not between apostrophes").arg(attrPosition);
		qDebug() << createErrorMsg(errmsg);
		return CLI_ATR_VAL_ERR;
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
		errmsg = QString("unknown attribute name '%1'").arg(attribute);
		qDebug() << createErrorMsg(errmsg);
		return CLI_UNKNOWN_ATR;
	}

	err = checkMandatoryAttributes(service, map, errmsg);
	if (CLI_SUCCESS != err) {
		map.clear();
		return err;
	}

	// add service name to map
	map["service"] = service;

	return CLI_SUCCESS;
}

/* ========================================================================= */
cli_error doService(const QString &service, const QMap<QString,QVariant> &map,
    MessageDbSet *msgDbSet, bool needsISDS, QString &errmsg)
/* ========================================================================= */
{

	if (service == SER_GET_MSG_LIST) {
		return getMsgList(map, msgDbSet, errmsg);
	} else if (service == SER_SEND_MSG) {
		return createAndSendMsg(map, msgDbSet, errmsg);
	} else if (service == SER_GET_MSG) {
		return getMsg(map, msgDbSet, needsISDS, errmsg);
	} else if (service == SER_GET_DEL_INFO) {
		return getDeliveryInfo(map, msgDbSet, needsISDS, errmsg);
	} else if (service == SER_GET_USER_INFO) {
		return getUserInfo(map, errmsg);
	} else if (service == SER_GET_OWNER_INFO) {
		return getOwnerInfo(map, errmsg);
	} else if (service == SER_CHECK_ATTACHMENT) {
		return checkAttachment(map, msgDbSet);
	} else if (service == SER_GET_MSG_IDS) {
		return getMsgIds(map, msgDbSet, errmsg);
	} else if (service == SER_FIND_DATABOX) {
		return findDatabox(map, errmsg);
	}
	errmsg = "Unknown service name";
	return CLI_UNKNOWN_SER;
}

/* ========================================================================= */
int runService(const QString &lParam,
    const QString &service, const QString &sParam)
/* ========================================================================= */
{
	QMap<QString,QVariant> loginMap;
	QMap <QString,QVariant> serviceMap;
	cli_error cret = CLI_ERROR;
	bool needsISDS = true;
	QString errmsg = "Unknown error";
	int ret = EXIT_FAILURE;

	/* parse service parameter list */
	if (!(service.isNull()) && !(sParam.isNull())) {
		qDebug() << CLI_PREFIX << "Parsing of input string of service"
		    << service << ":" << sParam;
		cret = parsePamamString(service, sParam, serviceMap, errmsg);
		if (CLI_SUCCESS != cret) {
			qDebug() << CLI_PREFIX << "...error";
			printErrToStdErr(cret, errmsg);
			return ret;
		}
		qDebug() << CLI_PREFIX << "...done";
	}

	/* parse login parameter list */
	qDebug() << CLI_PREFIX << "Parsing of input string of \"login\" :"
	    << lParam;
	cret = parsePamamString(SER_LOGIN, lParam, loginMap, errmsg);
	if (CLI_SUCCESS != cret) {
		qDebug() << CLI_PREFIX << "...error";
		printErrToStdErr(cret, errmsg);
		return ret;
	}
	qDebug() << CLI_PREFIX << "...done";

	/* get username from login */
	const QString username = loginMap["username"].toString();

	/* get message database set */
	MessageDbSet *msgDbSet = Q_NULLPTR;
	{
		enum AccountInteraction::AccessStatus status;
		QString dbDir, namesStr;
		msgDbSet = AccountInteraction::accessDbSet(username, status,
		    dbDir, namesStr);
	}
	if (msgDbSet == NULL) {
		errmsg = "Database doesn't exists for user " + username;
		qDebug() << CLI_PREFIX << errmsg;
		printErrToStdErr(CLI_DB_ERR, errmsg);
		return ret;
	}

	if (service == SER_GET_MSG) {
		MessageDb::MsgId msgId =
		    msgDbSet->msgsMsgId(serviceMap["dmID"].toLongLong());
		if (msgId.dmId < 0) {
			errmsg = "Message does not exist in the database "
			    "for user " + username;
			qDebug() << CLI_PREFIX << errmsg;
			printErrToStdErr(CLI_DB_ERR, errmsg);
			return ret;
		}
		MessageDb *messageDb =
		    msgDbSet->accessMessageDb(msgId.deliveryTime, false);
		if (messageDb == NULL) {
			errmsg = "Database doesn't exists for user " + username;
			qDebug() << CLI_PREFIX << errmsg;
			printErrToStdErr(CLI_DB_ERR, errmsg);
			return ret;
		}

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
		MessageDb::MsgId msgId =
		    msgDbSet->msgsMsgId(serviceMap["dmID"].toLongLong());
		if (msgId.dmId < 0) {
			errmsg = "Message does not exist in the database "
			    "for user " + username;
			qDebug() << CLI_PREFIX << errmsg;
			printErrToStdErr(CLI_DB_ERR, errmsg);
			return ret;
		}
		MessageDb *messageDb =
		    msgDbSet->accessMessageDb(msgId.deliveryTime, false);
		if (messageDb == NULL) {
			errmsg = "Database doesn't exists for user " + username;
			qDebug() << CLI_PREFIX << errmsg;
			printErrToStdErr(CLI_DB_ERR, errmsg);
			return ret;
		}

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

	if (service == SER_CHECK_ATTACHMENT || service == SER_GET_MSG_IDS) {
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

		if (!globIsdsSessionsPtr->isConnectedToIsds(username) &&
		    !connectToIsdsCLI(*globIsdsSessionsPtr,
		        globAccounts[username], pwd, otp)) {
			errmsg = "Missing session for " + username +
			   " or connection fails";
			qDebug() << errmsg;
			printErrToStdErr(CLI_CONNECT_ERR, errmsg);
			return ret;
		}
		qDebug() << CLI_PREFIX << "User" << username
		    << "has been logged into databox";
		cret = CLI_SUCCESS;
	}

	/* do service */
	if (!service.isNull()) {
		serviceMap["username"] = username;
		cret = doService(service, serviceMap, msgDbSet, needsISDS,
		    errmsg);
	}

	if (CLI_SUCCESS == cret) {
		ret = EXIT_SUCCESS;
	} else {
		printErrToStdErr(cret, errmsg);
	}

	return ret;
}
