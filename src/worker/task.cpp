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

#include "src/crypto/crypto_funcs.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h" /* List with whole messages. */
#include "src/worker/task.h"
#include "src/worker/task_download_message.h" /* List with whole messages. */

qdatovka_error Task::downloadDeliveryInfo(const QString &userName, qint64 dmId,
    bool signedMsg, MessageDbSet &dbSet)
{
	debugFuncCall();

	isds_error status;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}
	struct isds_message *message = NULL;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(),
		    &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		goto fail;
		/*
		status = isds_get_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(),
		    &message);
		*/
	}

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading delivery information returned status %d: '%s'.",
		    status, isds_strerror(status));
		goto fail;
	}

	Q_ASSERT(NULL != message);

	if (Q_SUCCESS != storeDeliveryInfo(signedMsg, dbSet, message)) {
		goto fail;
	}

	isds_message_free(&message);

	return Q_SUCCESS;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}

	return Q_ISDS_ERROR;
}

qdatovka_error Task::downloadMessage(const QString &userName,
    MessageDb::MsgId mId, bool signedMsg, enum MessageDirection msgDirect,
    MessageDbSet &dbSet, QString &errMsg, const QString &progressLabel)
{
	debugFuncCall();

	logDebugLv0NL("Trying to download complete message '%d'", mId.dmId);

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	isds_error status;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return Q_GLOBAL_ERROR;
	}
	// message structures - all members
	struct isds_message *message = NULL;

	emit globMsgProcEmitter.progressChange(progressLabel, 10);

	/* download signed message? */
	if (signedMsg) {
		/* sent or received message? */
		if (MSG_RECEIVED == msgDirect) {
			status = isds_get_signed_received_message(
			    isdsSessions.session(userName),
			    QString::number(mId.dmId).toUtf8().constData(),
			    &message);
		} else {
			status = isds_get_signed_sent_message(
			    isdsSessions.session(userName),
			    QString::number(mId.dmId).toUtf8().constData(),
			    &message);
		}
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return Q_GLOBAL_ERROR;
		/*
		status = isds_get_received_message(isdsSessions.session(
		    userName),
		    QString::number(mId.dmId).toUtf8().constData(),
		    &message);
		*/
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 20);

	if (IE_SUCCESS != status) {
		errMsg = isdsLongMessage(session);
		logErrorNL("Downloading message returned status %d: '%s' '%s'.",
		    status, isds_strerror(status), errMsg.toUtf8().constData());
		isds_message_free(&message);
		return Q_ISDS_ERROR;
	}

	{
		QString secKeyBeforeDownload(dbSet.secondaryKey(mId.deliveryTime));
		QDateTime newDeliveryTime(timevalToDateTime(
		    message->envelope->dmDeliveryTime));
		QString secKeyAfterDonwload(dbSet.secondaryKey(newDeliveryTime));
		/*
		 * Secondary keys may be the sam for valid and invalid
		 * delivery time when storing into single file.
		 */
		if (secKeyBeforeDownload != secKeyAfterDonwload) {
			/*
			 * The message was likely moved from invalid somewhere
			 * else.
			 */
			/* Delete the message from invalid. */
			MessageDb *db = dbSet.accessMessageDb(mId.deliveryTime, false);
			if (0 != db) {
				db->msgsDeleteMessageData(mId.dmId);
			}

			/* Store envelope in new location. */
			storeEnvelope(msgDirect, dbSet, message->envelope);
		}
		/* Update message delivery time. */
		mId.deliveryTime = newDeliveryTime;
	}

	/* Store the message. */
	storeMessage(signedMsg, msgDirect, dbSet, message,
	    progressLabel);

	emit globMsgProcEmitter.progressChange(progressLabel, 90);

	Q_ASSERT(mId.dmId == QString(message->envelope->dmID).toLongLong());

	/* Download and save delivery info and message events */
	if (Q_SUCCESS == downloadDeliveryInfo(userName, mId.dmId, signedMsg, dbSet)) {
		logDebugLv0NL("Delivery info of message '%d' processed.",
		    mId.dmId);
	} else {
		logErrorNL("Processing delivery info of message '%d' failed.",
		    mId.dmId);
	}

	Q_ASSERT(mId.deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(mId.deliveryTime, true);
	Q_ASSERT(0 != messageDb);

	downloadMessageAuthor(userName, mId.dmId, *messageDb);

	if (MSG_RECEIVED == msgDirect) {
		/*  Mark this message as downloaded in ISDS */
		if (markMessageAsDownloaded(userName, mId.dmId)) {
			logDebugLv0NL("Message '%d' marked as downloaded.",
			    mId.dmId);
		} else {
			logErrorNL(
			    "Marking message '%d' as downloaded failed.",
			    mId.dmId);
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 100);

	isds_list_free(&message->documents);
	isds_message_free(&message);

	logDebugLv0NL("Done with %s().", __func__);

	return Q_SUCCESS;
}

qdatovka_error Task::downloadMessageList(const QString &userName,
    enum MessageDirection msgDirect, MessageDbSet &dbSet, QString &errMsg,
    const QString &progressLabel, int &total, int &news,
    QStringList &newMsgIdList, ulong *dmLimit, int dmStatusFilter)
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
		logErrorNL(
		    "Downloading message list returned status %d: '%s' '%s'.",
		    status, isds_strerror(status), errMsg.toUtf8().constData());
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
		MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime,
		    true);
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
			storeEnvelope(msgDirect, dbSet, item->envelope);

			if (globPref.auto_download_whole_messages) {
				QString errMsg;

				TaskDownloadMessage *task;

				task = new (std::nothrow) TaskDownloadMessage(
				    userName, &dbSet, msgDirect, dmId,
				    deliveryTime);
				task->setAutoDelete(true);
				globWorkPool.assign(task, WorkerPool::PREPEND);
			}
			newMsgIdList.append(dmID);
			newcnt++;

		/* Message is in db (dmDbMsgStatus <> -1). */
		} else {
			/* Update envelope if message status has changed. */
			const int dmNewMsgStatus = convertHexToDecIndex(
			     *item->envelope->dmMessageStatus);

			if (dmNewMsgStatus != dmDbMsgStatus) {
				updateEnvelope(msgDirect, *messageDb,
				    item->envelope);
			}

			if (MSG_SENT == msgDirect) {
				/*
				 * Sent messages content will be downloaded
				 * only if those message state is 1 or 2.
				 */
				if (globPref.auto_download_whole_messages &&
				    (dmDbMsgStatus <= 2)) {
					QString errMsg;

					TaskDownloadMessage *task;

					task = new (std::nothrow) TaskDownloadMessage(
					    userName, &dbSet, msgDirect, dmId,
					    deliveryTime);
					task->setAutoDelete(true);
					globWorkPool.assign(task,
					    WorkerPool::PREPEND);
				}

				if (dmDbMsgStatus != dmNewMsgStatus) {
					downloadMessageState(msgDirect,
					    userName, dmId, true, dbSet);
				}
			}

			/* Message is in db, but the content is missing. */
			if (globPref.auto_download_whole_messages &&
			    !messageDb->msgsStoredWhole(dmId)) {
				QString errMsg;

				TaskDownloadMessage *task;

				task = new (std::nothrow) TaskDownloadMessage(
				    userName, &dbSet, msgDirect, dmId,
				    deliveryTime);
				task->setAutoDelete(true);
				globWorkPool.assign(task, WorkerPool::PREPEND);
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
		logDebugLv0NL("#Received total: %d #Received new: %d",
		    allcnt, newcnt);
	} else {
		logDebugLv0NL("#Sent total: %d #Sent new: %d", allcnt, newcnt);
	}

	total = allcnt;
	news =  newcnt;

	return Q_SUCCESS;
#undef USE_TRANSACTIONS
}

qdatovka_error Task::storeAttachments(MessageDb &messageDb, qint64 dmId,
    const struct isds_list *documents)
{
	const struct isds_list *file = documents;

	while (NULL != file) {
		const isds_document *item = (isds_document *) file->data;

		QByteArray dmEncodedContentBase64 = QByteArray(
		    (char *)item->data, item->data_length).toBase64();

		/* Insert/update file to db */
		if (messageDb.msgsInsertUpdateMessageFile(dmId,
		        item->dmFileDescr, item->dmUpFileGuid,
		        item->dmFileGuid, item->dmMimeType, item->dmFormat,
		        convertAttachmentType(item->dmFileMetaType),
		        dmEncodedContentBase64)) {
			logDebugLv0NL(
			    "Attachment file '%s' was stored into database.",
			    item->dmFileDescr);
		} else {
			logErrorNL("Storing attachment file '%s' failed.",
			    item->dmFileDescr);
		}
		file = file->next;
	}

	return Q_SUCCESS;
}

qdatovka_error Task::storeDeliveryInfo(bool signedMsg, MessageDbSet &dbSet,
    const struct isds_message *msg)
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
	QDateTime deliveryTime = timevalToDateTime(envel->dmDeliveryTime);
	Q_ASSERT(deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(0 != messageDb);

	/* get signed raw data from message */
	if (signedMsg) {
		if (messageDb->msgsInsertUpdateDeliveryInfoRaw(dmID,
		    QByteArray((char*)msg->raw, msg->raw_length))) {
			logDebugLv0NL(
			    "Raw delivery info of message '%d' was updated.",
			    dmID);
		} else {
			logErrorNL(
			    "Raw delivery info of message '%d' update failed.",
			    dmID);
		}
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

qdatovka_error Task::storeEnvelope(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const struct isds_envelope *envel)
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

	QDateTime deliveryTime = timevalToDateTime(envel->dmDeliveryTime);
	/* Allow invalid delivery time. */
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(0 != messageDb);

	/* insert message envelope in db */
	if (messageDb->msgsInsertMessageEnvelope(dmId,
	    /* TODO - set correctly next two values */
	    "tRecord",
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
		logDebugLv0NL("Stored envelope of message '%d' into database.",
		    dmId);
		return Q_SUCCESS;
	} else {
		logErrorNL("Storing envelope of message '%d' failed.", dmId);
		return Q_GLOBAL_ERROR;
	}
}

qdatovka_error Task::storeMessage(bool signedMsg,
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const struct isds_message *msg, const QString &progressLabel)
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

	/*
	 * If there is no raw message then all the attachments have been
	 * stored when the message has been set.
	 */
	if (!messageDb->msgsStoredWhole(dmID)) {
		messageDb->flsDeleteMessageFiles(dmID);
	}

	/* Get signed raw data from message and store to db. */
	if (signedMsg) {
		if (messageDb->msgsInsertUpdateMessageRaw(dmID,
		        QByteArray((char*) msg->raw, msg->raw_length), 0)) {
			logDebugLv0NL("Raw data of message '%d' were updated.",
			    dmID);
		} else {
			logErrorNL("Updating raw data of message '%d' failed.",
			    dmID);
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 30);

	if (updateEnvelope(msgDirect, *messageDb, envel)) {
		logDebugLv0NL("Envelope of message '%d' updated.", dmID);
	} else {
		logErrorNL("Updating envelope of message '%d' failed.", dmID);
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 50);

	if (signedMsg) {
		/* Verify message signature. */
		int ret = raw_msg_verify_signature(msg->raw,
		    msg->raw_length, 1, globPref.check_crl ? 1 : 0);
		logDebugLv0NL("Verification of message '%d' returned: %d.",
		    dmID, ret);
		if (1 == ret) {
			messageDb->msgsSetVerified(dmID, true);
			/* TODO -- handle return error. */
		} else if (0 == ret){
			messageDb->msgsSetVerified(dmID, false);
			/* TODO -- handle return error. */
		} else {
			/* TODO -- handle this error. */
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 60);

	/* insert/update hash into db */
	if (NULL != envel->hash) {
		const struct isds_hash *hash = envel->hash;

		QByteArray hashValueBase64 = QByteArray((char *) hash->value,
		    hash->length).toBase64();
		if (messageDb->msgsInsertUpdateMessageHash(dmID,
		        hashValueBase64, convertHashAlg(hash->algorithm))) {
			logDebugLv0NL("Hash of message '%d' stored.", dmID);
		} else {
			logErrorNL("Storing hash of message '%s' failed.",
			    dmID);
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 70);

	/* Insert/update all attachment files */
	storeAttachments(*messageDb, dmID, msg->documents);

	return Q_SUCCESS;
}

bool Task::downloadMessageAuthor(const QString &userName, qint64 dmId,
    MessageDb &messageDb)
{
	isds_error status;

	isds_sender_type *sender_type = NULL;
	char * raw_sender_type = NULL;
	char * sender_name = NULL;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}

	status = isds_get_message_sender(session,
	    QString::number(dmId).toUtf8().constData(),
	    &sender_type, &raw_sender_type, &sender_name);

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading author information returned status %d: '%s'.",
		    status, isds_strerror(status));
		return false;
	}

	if (messageDb.updateMessageAuthorInfo(dmId,
	        convertSenderTypeToString((int) *sender_type), sender_name)) {
		logDebugLv0NL(
		    "Author information of message '%d' were updated.",
		    dmId);
	} else {
		logErrorNL(
		    "Updating author information of message '%d' failed.",
		    dmId);
	}

	return true;
}

bool Task::downloadMessageState(enum MessageDirection msgDirect,
    const QString &userName, qint64 dmId, bool signedMsg, MessageDbSet &dbSet)
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
		logErrorNL(
		    "Downloading message state returned status %d: '%s'.",
		    status, isds_strerror(status));
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

bool Task::markMessageAsDownloaded(const QString &userName,
    qint64 dmId)
{
	debugFuncCall();

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return false;
	}

	isds_error status;

	status = isds_mark_message_read(session,
	    QString::number(dmId).toUtf8().constData());

	if (IE_SUCCESS != status) {
		return false;
	}
	return true;
}

qdatovka_error Task::updateEnvelope(enum MessageDirection msgDirect,
    MessageDb &messageDb, const struct isds_envelope *envel)
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
		logDebugLv0NL("Updated envelope of message '%d' in database.",
		    dmId);
		return Q_SUCCESS;
	} else {
		logErrorNL("Updating envelope of message '%d' failed.", dmId);
		return Q_GLOBAL_ERROR;
	}
}

qdatovka_error Task::updateMessageState(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const struct isds_envelope *envel)
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
		logDebugLv0NL(
		    "Delivery information of message '%d' were updated.",
		    dmID);
	} else {
		logErrorNL(
		    "Updating delivery information of message '%d' failed.",
		    dmID);
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
