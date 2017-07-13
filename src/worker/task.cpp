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

#include <cinttypes>

#include "src/crypto/crypto_funcs.h"
#include "src/io/account_db.h" /* globAccountDbPtr */
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task.h"

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
			    "Raw delivery info of message '%" PRId64 "' was updated.",
			    dmID);
		} else {
			logErrorNL(
			    "Raw delivery info of message '%" PRId64 "' update failed.",
			    dmID);
		}
	}

	const struct isds_list *event;
	event = envel->events;

	while (0 != event) {
		isds_event *item = (isds_event *) event->data;
		messageDb->msgsInsertUpdateMessageEvent(dmID,
		    timevalToDbFormat(item->time),
		    convertEventTypeToString(*item->type) + QLatin1String(": "),
		    item->description);
		event = event->next;
	}

	return Q_SUCCESS;
}

qdatovka_error Task::storeEnvelope(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const struct isds_envelope *envel,
    QString msgId)
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

	if (msgId.isEmpty()) {
		msgId = "tRecord";
	}

	/* insert message envelope in db */
	if (messageDb->msgsInsertMessageEnvelope(dmId,
	    msgId,
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
	        convertIsdsMsgStatusToDbRepr(*envel->dmMessageStatus) : 0,
	    envel->dmAttachmentSize ?
	        (int) *envel->dmAttachmentSize : 0,
	    envel->dmType,
	    msgDirect)) {
		logDebugLv0NL("Stored envelope of message '%" PRId64 "' into database.",
		    dmId);
		return Q_SUCCESS;
	} else {
		logErrorNL("Storing envelope of message '%" PRId64 "' failed.",
		    dmId);
		return Q_GLOBAL_ERROR;
	}
}

qdatovka_error Task::storeMessage(bool signedMsg,
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const struct isds_message *msg, const QString &progressLabel, QString msgId)
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
			logDebugLv0NL(
			    "Raw data of message '%" PRId64 "' were updated.",
			    dmID);
		} else {
			logErrorNL(
			    "Updating raw data of message '%" PRId64 "' failed.",
			    dmID);
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 30);

	if (Q_SUCCESS == updateEnvelope(msgDirect, *messageDb, envel, msgId)) {
		logDebugLv0NL("Envelope of message '%" PRId64 "' updated.",
		    dmID);
	} else {
		logErrorNL("Updating envelope of message '%" PRId64 "' failed.",
		    dmID);
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 50);

	if (signedMsg) {
		/* Verify message signature. */
		int ret = raw_msg_verify_signature(msg->raw,
		    msg->raw_length, 1, globPref.check_crl ? 1 : 0);
		logDebugLv0NL(
		   "Verification of message '%" PRId64 "' returned: %d.",
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
		        hashValueBase64,
		        convertHashAlgToString(hash->algorithm))) {
			logDebugLv0NL("Hash of message '%" PRId64 "' stored.",
			    dmID);
		} else {
			logErrorNL(
			    "Storing hash of message '%" PRId64 "' failed.",
			    dmID);
		}
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 70);

	/* Insert/update all attachment files */
	storeAttachments(*messageDb, dmID, msg->documents);

	return Q_SUCCESS;
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

qdatovka_error Task::updateEnvelope(enum MessageDirection msgDirect,
    MessageDb &messageDb, const struct isds_envelope *envel, QString msgId)
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

	if (msgId.isEmpty()) {
		msgId = "tReturnedMessage";
	}

	/* Update message envelope in db. */
	if (messageDb.msgsUpdateMessageEnvelope(dmId,
	    msgId,
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
	        convertIsdsMsgStatusToDbRepr(*envel->dmMessageStatus) : 0,
	    envel->dmAttachmentSize ?
	        (int) *envel->dmAttachmentSize : 0,
	    envel->dmType,
	    msgDirect)) {
		logDebugLv0NL(
		    "Updated envelope of message '%" PRId64 "' in database.",
		    dmId);
		return Q_SUCCESS;
	} else {
		logErrorNL("Updating envelope of message '%" PRId64 "' failed.",
		    dmId);
		return Q_GLOBAL_ERROR;
	}
}


qdatovka_error Task::storeEnvelopeWebDatovka(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const JsonLayer::Envelope &envel, bool isNew)
{
	debugFuncCall();

	QDateTime dTime = fromIsoDatetimetoDateTime(envel.dmDeliveryTime);

	/* Allow invalid delivery time. */
	MessageDb *messageDb = dbSet.accessMessageDb(dTime, true);
	Q_ASSERT(0 != messageDb);

	if (isNew) {
		/* insert message envelope in db */
		if (messageDb->msgsInsertMessageEnvelope(envel.dmID,
		    QString::number(envel.id),
		    envel.dbIDSender,
		    envel.dmSender,
		    envel.dmSenderAddress,
		    envel.dmSenderType,
		    envel.dmRecipient,
		    envel.dmRecipientAddress,
		    envel.dmAmbiguousRecipient,
		    envel.dmSenderOrgUnit,
		    envel.dmSenderOrgUnitNum,
		    envel.dbIDRecipient,
		    envel.dmRecipientOrgUnit,
		    envel.dmRecipientOrgUnitNum,
		    envel.dmToHands,
		    envel.dmAnnotation,
		    envel.dmRecipientRefNumber,
		    envel.dmSenderRefNumber,
		    envel.dmRecipientIdent,
		    envel.dmSenderIdent,
		    envel.dmLegalTitleLaw,
		    envel.dmLegalTitleYear,
		    envel.dmLegalTitleSect,
		    envel.dmLegalTitlePar,
		    envel.dmLegalTitlePoint,
		    envel.dmPersonalDelivery,
		    envel.dmAllowSubstDelivery,
		    QByteArray(),
		    fromIsoDatetimetoDbformat(envel.dmDeliveryTime),
		    fromIsoDatetimetoDbformat(envel.dmAcceptanceTime),
		    envel.dmMessageStatus,
		    envel.dmAttachmentSize,
		    envel.dmType,
		    msgDirect)) {
			logDebugLv0NL("Stored envelope of message '%" PRId64 "' into database.",
			    envel.dmID);
			return Q_SUCCESS;
		} else {
			logErrorNL("Storing envelope of message '%" PRId64 "' failed.",
			    envel.dmID);
			return Q_GLOBAL_ERROR;
		}
	} else {
		/* Update message envelope in db. */
		if (messageDb->msgsUpdateMessageEnvelope(envel.dmID,
		    QString::number(envel.id),
		    envel.dbIDSender,
		    envel.dmSender,
		    envel.dmSenderAddress,
		    envel.dmSenderType,
		    envel.dmRecipient,
		    envel.dmRecipientAddress,
		    envel.dmAmbiguousRecipient,
		    envel.dmSenderOrgUnit,
		    envel.dmSenderOrgUnitNum,
		    envel.dbIDRecipient,
		    envel.dmRecipientOrgUnit,
		    envel.dmRecipientOrgUnitNum,
		    envel.dmToHands,
		    envel.dmAnnotation,
		    envel.dmRecipientRefNumber,
		    envel.dmSenderRefNumber,
		    envel.dmRecipientIdent,
		    envel.dmSenderIdent,
		    envel.dmLegalTitleLaw,
		    envel.dmLegalTitleYear,
		    envel.dmLegalTitleSect,
		    envel.dmLegalTitlePar,
		    envel.dmLegalTitlePoint,
		    envel.dmPersonalDelivery,
		    envel.dmAllowSubstDelivery,
		    QByteArray(),
		    fromIsoDatetimetoDbformat(envel.dmDeliveryTime),
		    fromIsoDatetimetoDbformat(envel.dmAcceptanceTime),
		    envel.dmMessageStatus,
		    envel.dmAttachmentSize,
		    envel.dmType,
		    msgDirect)) {
			logDebugLv0NL("Stored envelope of message '%" PRId64 "' into database.",
			    envel.dmID);
			return Q_SUCCESS;
		} else {
			logErrorNL("Storing envelope of message '%" PRId64 "' failed.",
			    envel.dmID);
			return Q_GLOBAL_ERROR;
		}
	}
}
