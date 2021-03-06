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

#include <cstdlib>
#include <QDebug>
#include <QTextStream>

#include "src/cli/cli.h"
#include "src/cli/cli_login.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/datovka_shared/utility/strings.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/io/isds_helper.h"
#include "src/isds/to_text_conversion.h"
#include "src/model_interaction/account_interaction.h"
#include "src/worker/task_download_message.h"
#include "src/worker/task_download_message_list.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_send_message.h"

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
	Isds::Type::DmFiltStates dmStatusFilter = Isds::Type::MFS_ANY;
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
		case 1: dmStatusFilter = Isds::Type::MFS_POSTED; break;
		case 2: dmStatusFilter = Isds::Type::MFS_STAMPED; break;
		case 3: dmStatusFilter = Isds::Type::MFS_INFECTED; break;
		case 4: dmStatusFilter = Isds::Type::MFS_DELIVERED; break;
		case 5: dmStatusFilter = Isds::Type::MFS_ACCEPTED_FICT; break;
		case 6: dmStatusFilter = Isds::Type::MFS_ACCEPTED; break;
		case 7: dmStatusFilter = Isds::Type::MFS_READ; break;
		case 8: dmStatusFilter = Isds::Type::MFS_UNDELIVERABLE; break;
		case 9: dmStatusFilter = Isds::Type::MFS_REMOVED; break;
		case 10: dmStatusFilter = Isds::Type::MFS_IN_VAULT; break;
		default: dmStatusFilter = Isds::Type::MFS_ANY; break;
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
		GlobInstcs::workPoolPtr->runSingle(task);

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
			GlobInstcs::workPoolPtr->stop();
			GlobInstcs::workPoolPtr->clear();
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
		GlobInstcs::workPoolPtr->runSingle(task);

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
			GlobInstcs::workPoolPtr->stop();
			GlobInstcs::workPoolPtr->clear();
			return CLI_ERROR;
		}
	}

	/* Wait for possible pending jobs. */
	GlobInstcs::workPoolPtr->wait();

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

		QList<Isds::Document> files =
		    messageDb->getMessageAttachments(map["dmID"].toLongLong());

		foreach (const Isds::Document &file, files) {

			QString fileName = file.fileDescr();
			if (fileName.isEmpty()) {
				errmsg = "Cannot save file because name "
				    "of file missing";
				qDebug() << CLI_PREFIX << errmsg;
				return CLI_ERROR;
			}

			if (file.binaryContent().isEmpty()) {
				errmsg = "Cannot save file " + fileName +
				    "because file content missing";
				qDebug() << CLI_PREFIX << errmsg;
				return CLI_ERROR;
			}

			fileName = path + QDir::separator() + fileName;

			enum WriteFileState ret =
			    writeFile(fileName, file.binaryContent());
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
		    messageDb->getCompleteMessageBase64(map["dmID"].toLongLong());

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
			messageDb->setMessageLocallyRead(
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

		QByteArray base64 = messageDb->getDeliveryInfoBase64(
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
	qDebug() << CLI_PREFIX << "find info about databox from username"
	    <<  map["username"].toString();

	Isds::Address address;
	Isds::DbOwnerInfo dbOwnerInfo;
	Isds::PersonName personName;
	QList<Isds::DbOwnerInfo> foundBoxes;
	QString errMsg;
	QString longErrMsg;

	dbOwnerInfo.setDbID(map.contains("dbID") ?
	    map.value("dbID").toString() : QString());
	dbOwnerInfo.setDbType(Isds::strVariant2DbType(map.value("dbType")));
	dbOwnerInfo.setIc(map.contains("ic") ?
	    map.value("ic").toString() : QString());
	personName.setFirstName(map.contains("pnFirstName") ?
	    map.value("pnFirstName").toString() : QString());
	personName.setLastName(map.contains("pnLastName") ?
	    map.value("pnLastName").toString() : QString());
	dbOwnerInfo.setPersonName(personName);
	dbOwnerInfo.setFirmName(map.contains("firmName") ?
	    map.value("firmName").toString() : QString());
	address.setZipCode(map.contains("adZipCode") ?
	    map.value("adZipCode").toString() : QString());
	dbOwnerInfo.setAddress(address);

	enum TaskSearchOwner::Result result = TaskSearchOwner::isdsSearch(
	    map["username"].toString(), dbOwnerInfo, foundBoxes, errMsg,
	    longErrMsg);

	if (TaskSearchOwner::SO_SUCCESS != result) {
		errmsg = longErrMsg;
		return CLI_ERROR;
	}

	foreach (const Isds::DbOwnerInfo &box, foundBoxes) {
		QStringList contact;
		contact.append(box.dbID());
		contact.append("|");
		contact.append(Isds::textOwnerName(box));
		contact.append("|");
		contact.append(Isds::textAddressWithoutIc(box.address()));
		contact.append("|");
		contact.append(box.address().zipCode());
		printDataToStdOut(contact);
	}

	return CLI_SUCCESS;
}

/* ========================================================================= */
QList<Isds::Document> buildDocuments(const QStringList &filePaths)
/* ========================================================================= */
{
	QList<Isds::Document> documents;
	bool mainFile = true;

	foreach (const QString &filePath, filePaths) {

		if (filePath.isEmpty()) {
			continue;
		}

		Isds::Document document;

		QFileInfo fi(filePath);
		if (!fi.isReadable() || !fi.isFile()) {
			logErrorNL(CLI_PREFIX "Wrong file name '%s' or file is missing.",
			    filePath.toUtf8().constData());
			goto fail;
		}

		document.setFileDescr(fi.fileName());
		document.setFileMetaType(mainFile ?
		    Isds::Type::FMT_MAIN : Isds::Type::FMT_ENCLOSURE);
		document.setMimeType(QStringLiteral(""));

		QFile file(QDir::fromNativeSeparators(filePath));
		if (file.exists()) {
			if (!file.open(QIODevice::ReadOnly)) {
				logErrorNL(CLI_PREFIX "Couldn't open file '%s'.",
				    filePath.toUtf8().constData());
				goto fail;
			}
		}

		document.setBinaryContent(file.readAll());
		documents.append(document);
		mainFile = false;
	}

	return documents;
fail:
	return QList<Isds::Document>();
}

/* ========================================================================= */
Isds::Envelope buildEnvelope(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	Isds::Envelope envelope;

	envelope.setDmAnnotation(map["dmAnnotation"].toString());

	if (map.contains("dmSenderIdent")) {
		envelope.setDmSenderIdent(map["dmSenderIdent"].toString());
	}

	if (map.contains("dmRecipientIdent")) {
		envelope.setDmRecipientIdent(
		    map["dmRecipientIdent"].toString());
	}

	if (map.contains("dmSenderRefNumber")) {
		envelope.setDmSenderRefNumber(
		     map["dmSenderRefNumber"].toString());
	}

	if (map.contains("dmRecipientRefNumber")) {
		envelope.setDmRecipientRefNumber(
		    map["dmRecipientRefNumber"].toString());
	}

	if (map.contains("dmToHands")) {
		envelope.setDmToHands(map["dmToHands"].toString());
	}

	if (map.contains("dmLegalTitleLaw")) {
		envelope.setDmLegalTitleLaw(
		    map["dmLegalTitleLaw"].toLongLong());
	}

	if (map.contains("dmLegalTitleYear")) {
		envelope.setDmLegalTitleYear(
		    map["dmLegalTitleYear"].toLongLong());
	}

	if (map.contains("dmLegalTitleSect")) {
		envelope.setDmLegalTitleSect(
		    map["dmLegalTitleSect"].toString());
	}

	if (map.contains("dmLegalTitlePar")) {
		envelope.setDmLegalTitlePar(map["dmLegalTitlePar"].toString());
	}

	if (map.contains("dmLegalTitlePoint")) {
		envelope.setDmLegalTitlePoint(
		    map["dmLegalTitlePoint"].toString());
	}

	if (map.contains("dmPersonalDelivery")) {
		envelope.setDmPersonalDelivery(
		    map["dmPersonalDelivery"].toString() != "0" ?
		    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	}

	if (map.contains("dmAllowSubstDelivery")) {
		envelope.setDmAllowSubstDelivery(
		    map["dmAllowSubstDelivery"].toString() != "0" ?
		    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	}

	if (map.contains("dmType")) {
		envelope.setDmType(map["dmType"].toChar());
	}

	if (map.contains("dmOVM")) {
		envelope.setDmOVM(map["dmOVM"].toString() != "0" ?
		    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	}

	if (map.contains("dmPublishOwnID")) {
		envelope.setDmPublishOwnID(
		    map["dmPublishOwnID"].toString() != "0" ?
		    Isds::Type::BOOL_TRUE : Isds::Type::BOOL_FALSE);
	}

	return envelope;
}

/* ========================================================================= */
cli_error createAndSendMsg(const QMap <QString, QVariant> &map,
    MessageDbSet *msgDbSet, QString &errmsg)
/* ========================================================================= */
{
	qDebug() << CLI_PREFIX << "creating a new message...";

	cli_error ret = CLI_ERROR;
	QStringList sendID;
	Isds::Envelope evelope = buildEnvelope(map);
	Isds::Message message;
	message.setDocuments(buildDocuments(map["dmAttachment"].toStringList()));

	foreach (const QString &recipientId,
	    map.value("dbIDRecipient").toStringList()) {

		logInfo(CLI_PREFIX "Sending message to '%s'.\n",
		    recipientId.toUtf8().constData());

		evelope.setDbIDRecipient(recipientId);
		message.setEnvelope(evelope);

		QString transactId = map["username"].toString() + "_" +
		    QDateTime::currentDateTimeUtc().toString() + "_" +
		    Utility::generateRandomString(6);

		TaskSendMessage *task = new (std::nothrow) TaskSendMessage(
		    map["username"].toString(), msgDbSet, transactId,
		    message, "Databox ID: " + recipientId, "unknown", false);
		if (task != Q_NULLPTR) {
			task->setAutoDelete(false);
			GlobInstcs::workPoolPtr->runSingle(task);
		}

		if (TaskSendMessage::SM_SUCCESS == task->m_resultData.result) {
			qDebug() << CLI_PREFIX << "message has been sent"
			    << task->m_resultData.dmId;
			sendID.append(QString::number(task->m_resultData.dmId));
			ret = CLI_SUCCESS;
		} else {
			errmsg = "Error while sending message! ISDS says: "
			    + task->m_resultData.errInfo;
			qDebug() << CLI_PREFIX << errmsg << "Error code:"
			    << task->m_resultData.result;
			ret = CLI_ERROR;
		}
		delete task; task = Q_NULLPTR;
	}

	printDataToStdOut(sendID);
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
				needsISDS = !messageDb->isCompleteMessageInDb(
				    serviceMap["dmID"].toLongLong());
			}
		} else {
			needsISDS = !messageDb->isCompleteMessageInDb(
			    serviceMap["dmID"].toLongLong());
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
				needsISDS=messageDb->getDeliveryInfoBase64(
				    serviceMap["dmID"].toLongLong()).isNull();
			}
		} else {
			needsISDS = messageDb->getDeliveryInfoBase64(
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

		if (!GlobInstcs::isdsSessionsPtr->isConnectedToIsds(username) &&
		    !connectToIsdsCLI(*GlobInstcs::isdsSessionsPtr,
		        (*GlobInstcs::acntMapPtr)[username], pwd, otp)) {
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
