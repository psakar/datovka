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

#include <cinttypes>

#include "src/crypto/crypto_funcs.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task.h"

qdatovka_error Task::storeDeliveryInfo(bool signedMsg, MessageDbSet &dbSet,
    const Isds::Message &message)
{
	debugFuncCall();

	qint64 dmID = message.envelope().dmId();
	QDateTime deliveryTime = message.envelope().dmDeliveryTime();
	Q_ASSERT(deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(Q_NULLPTR != messageDb);

	/* get signed raw data from message */
	if (signedMsg) {
		if (messageDb->insertOrReplaceDeliveryInfoRaw(dmID, message.raw())) {
			logDebugLv0NL(
			    "Raw delivery info of message '%" PRId64 "' was updated.",
			    dmID);
		} else {
			logErrorNL(
			    "Raw delivery info of message '%" PRId64 "' update failed.",
			    dmID);
		}
	}

	QList<Isds::Event> events = message.envelope().dmEvents();
	foreach (const Isds::Event &event, events) {
		messageDb->insertOrUpdateMessageEvent(dmID, event);
	}

	return Q_SUCCESS;
}

qdatovka_error Task::storeMessageEnvelope(enum MessageDirection msgDirect,
    MessageDbSet &dbSet, const Isds::Envelope &envelope)
{
	debugFuncCall();

	qint64 dmId = envelope.dmId();
	QDateTime deliveryTime = envelope.dmDeliveryTime();
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(Q_NULLPTR != messageDb);

	if (messageDb->insertMessageEnvelope(envelope, "tRecord", msgDirect)) {
		logDebugLv0NL("Stored envelope of message '%" PRId64 "' into database.",
		    dmId);
		return Q_SUCCESS;
	} else {
		logErrorNL("Storing envelope of message '%" PRId64 "' failed.",
		    dmId);
		return Q_GLOBAL_ERROR;
	}
}

qdatovka_error Task::updateMessageEnvelope(enum MessageDirection msgDirect,
    MessageDb &messageDb, const Isds::Envelope &envelope)
{
	debugFuncCall();

	if (messageDb.updateMessageEnvelope(envelope, "tReturnedMessage",
	    msgDirect)) {
		logDebugLv0NL(
		    "Updated envelope of message '%" PRId64 "' in database.",
		    envelope.dmId());
		return Q_SUCCESS;
	} else {
		logErrorNL("Updating envelope of message '%" PRId64 "' failed.",
		    envelope.dmId());
		return Q_GLOBAL_ERROR;
	}
}

qdatovka_error Task::storeMessage(bool signedMsg,
    enum MessageDirection msgDirect, MessageDbSet &dbSet,
    const Isds::Message &message, const QString &progressLabel)
{
	debugFuncCall();

	qint64 dmID = message.envelope().dmId();
	QDateTime deliveryTime = message.envelope().dmDeliveryTime();
	Q_ASSERT(deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime, true);
	Q_ASSERT(Q_NULLPTR != messageDb);

	/*
	 * If there is no raw message then all the attachments have been
	 * stored when the message has been set.
	 */
	if (!messageDb->isCompleteMessageInDb(dmID)) {
		messageDb->deleteMessageAttachments(dmID);
	}

	/* Get signed raw data from message and store to db. */
	if (signedMsg) {
		if (messageDb->insertOrReplaceCompleteMessageRaw(dmID,
		    message.raw(), 0)) {
			logDebugLv0NL(
			    "Raw data of message '%" PRId64 "' were updated.",
			    dmID);
		} else {
			logErrorNL(
			    "Updating raw data of message '%" PRId64 "' failed.",
			    dmID);
		}
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 30);

	if (Q_SUCCESS == updateMessageEnvelope(msgDirect, *messageDb,
	    message.envelope())) {
		logDebugLv0NL("Envelope of message '%" PRId64 "' updated.",
		    dmID);
	} else {
		logErrorNL("Updating envelope of message '%" PRId64 "' failed.",
		    dmID);
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 50);

	if (signedMsg) {
		/* Verify message signature. */
		int ret = raw_msg_verify_signature(message.raw(),
		    message.raw().length(), 1,
		    GlobInstcs::prefsPtr->checkCrl ? 1 : 0);
		logDebugLv0NL(
		   "Verification of message '%" PRId64 "' returned: %d.",
		   dmID, ret);
		if (1 == ret) {
			messageDb->setMessageVerified(dmID, true);
			/* TODO -- handle return error. */
		} else if (0 == ret){
			messageDb->setMessageVerified(dmID, false);
			/* TODO -- handle return error. */
		} else {
			/* TODO -- handle this error. */
		}
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 60);

	if (messageDb->insertOrUpdateMessageHash(dmID,
	    message.envelope().dmHash())) {
		logDebugLv0NL("Hash of message '%" PRId64 "' stored.", dmID);
	} else {
		logErrorNL("Storing hash of message '%" PRId64 "' failed.", dmID);
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 70);

	storeAttachments(*messageDb, dmID, message.documents());

	return Q_SUCCESS;
}

qdatovka_error Task::storeAttachments(MessageDb &messageDb, qint64 dmId,
    const QList<Isds::Document> &documents)
{
	debugFuncCall();

	foreach (const Isds::Document &document, documents) {
		if (messageDb.insertOrUpdateMessageAttachment(dmId, document)) {
			logDebugLv0NL(
			    "Attachment file '%s' was stored into database.",
			    document.fileDescr());
		} else {
			logErrorNL("Storing attachment file '%s' failed.",
			    document.fileDescr());
		}
	}

	return Q_SUCCESS;
}
