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

#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_message_general.h"

qdatovka_error MessageTaskGeneral::downloadDeliveryInfo(const QString &userName,
    qint64 dmId, bool signedMsg, MessageDbSet &dbSet)
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
		    "Dowloading delivery information returned status %d: '%s'.",
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

qdatovka_error MessageTaskGeneral::storeDeliveryInfo(bool signedMsg,
    MessageDbSet &dbSet, const struct isds_message *msg)
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

qdatovka_error MessageTaskGeneral::storeEnvelope(
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const struct isds_envelope *envel)
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
		logError("Storing envelope of message '%d' failed.", dmId);
		return Q_GLOBAL_ERROR;
	}
}
