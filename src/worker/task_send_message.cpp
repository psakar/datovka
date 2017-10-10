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

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_send_message.h"

TaskSendMessage::ResultData::ResultData(void)
    : result(SM_ERR),
    errInfo(),
    dbIDRecipient(),
    recipientName(),
    isPDZ(false),
    dmId(-1)
{
}

TaskSendMessage::ResultData::ResultData(enum Result res, const QString &eInfo,
    const QString &recId, const QString &recName, bool pdz, qint64 mId)
    : result(res),
    errInfo(eInfo),
    dbIDRecipient(recId),
    recipientName(recName),
    isPDZ(pdz),
    dmId(mId)
{
}

TaskSendMessage::TaskSendMessage(const QString &userName,
    MessageDbSet *dbSet, const QString &transactId, const IsdsMessage &message,
    const QString &recipientName, const QString &recipientAddress, bool isPDZ)
    : m_resultData(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_transactId(transactId),
    m_message(message),
    m_recipientName(recipientName),
    m_recipientAddress(recipientAddress),
    m_isPDZ(isPDZ)
{
	Q_ASSERT(0 != m_dbSet);
}

/*!
 * @brief Free document list.
 *
 * @param[in,out] document Pointer to document.
 */
static
void isds_document_free_void(void **document)
{
	isds_document_free((struct isds_document **) document);
}

/*!
 * @brief Converts internal list of documents representation to libisds'.
 *
 * @param[in] docs List of documents.
 * @return Libisds list of documents or NULL on failure.
 */
static
struct isds_list *libisdsDocuments(const QList<IsdsDocument> &docs)
{
	size_t totalAttachSize = 0; /* Sum of all attachment sizes. */

	struct isds_document *document = NULL; /* Attachment. */
	struct isds_list *documents = NULL; /* Attachment list (entry). */
	struct isds_list *last = NULL; /* No need to free it explicitly. */

	for (int i = 0; i < docs.size(); ++i) {
		const IsdsDocument &doc = docs.at(i);

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		memset(document, 0, sizeof(struct isds_document));

		/* Set document content. */
		// TODO - document is binary document only -> is_xml = false;
		document->is_xml = doc.isXml;
		if (doc.dmFileDescr.isEmpty()) {
			Q_ASSERT(0);
			continue;
		}
		document->dmFileDescr = strdup(doc.dmFileDescr.toUtf8().constData());
		if (NULL == document->dmFileDescr) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}

		if (0 == i) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

		if (doc.dmMimeType.isNull()) {
			document->dmMimeType = strdup("");
		} else {
			document->dmMimeType = strdup(
			    doc.dmMimeType.toUtf8().constData());
		}
		if (NULL == document->dmMimeType) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}

		totalAttachSize += doc.data.size();
		document->data_length = doc.data.size();
		document->data = malloc(doc.data.size());
		if (NULL == document->data) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		memcpy(document->data, doc.data.data(), document->data_length);

		/* Add document on the list of document. */
		struct isds_list *newListItem = (struct isds_list *)
		    malloc(sizeof(struct isds_list));
		if (NULL == newListItem) {
			logErrorNL("%s", "Memory allocation failed.");
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

	if (totalAttachSize <= 0) {
		goto fail;
	}

	return documents;

fail:
	isds_document_free(&document);
	isds_list_free(&documents);
	return NULL;
}

/*!
 * @brief Converts internal envelope representation to libisds'.
 *
 * @param[in] env Internal envelope representation.
 * @return Libisds envelope structure or NULL on failure.
 */
static
struct isds_envelope *libisdsEnvelope(const IsdsEnvelope &env)
{
	struct isds_envelope *envelope = NULL;

	envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));
	if (envelope == NULL) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	memset(envelope, 0, sizeof(struct isds_envelope));

	/* Set mandatory fields of envelope. */
	Q_ASSERT(env.dmID.isNull());
	envelope->dmID = NULL;
	if (env.dmAnnotation.isEmpty()) {
		Q_ASSERT(0);
		goto fail;
	}
	Q_ASSERT(!env.dbIDRecipient.isEmpty());
	envelope->dbIDRecipient =
	    strdup(env.dbIDRecipient.toUtf8().constData());
	if (NULL == envelope->dbIDRecipient) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	Q_ASSERT(!env.dmAnnotation.isEmpty());
	envelope->dmAnnotation = strdup(env.dmAnnotation.toUtf8().constData());
	if (NULL == envelope->dmAnnotation) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}

	/* Set optional fields. */
	if (!env.dmToHands.isEmpty()) {
		envelope->dmToHands = strdup(
		    env.dmToHands.toUtf8().constData());
		if (NULL == envelope->dmToHands) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmSenderIdent.isEmpty()) {
		envelope->dmSenderIdent = strdup(
		    env.dmSenderIdent.toUtf8().constData());
		if (NULL == envelope->dmSenderIdent) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmRecipientIdent.isEmpty()) {
		envelope->dmRecipientIdent = strdup(
		    env.dmRecipientIdent.toUtf8().constData());
		if (NULL == envelope->dmRecipientIdent) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmSenderRefNumber.isEmpty()) {
		envelope->dmSenderRefNumber = strdup(
		    env.dmSenderRefNumber.toUtf8().constData());
		if (NULL == envelope->dmSenderRefNumber) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmRecipientRefNumber.isEmpty()) {
		envelope->dmRecipientRefNumber = strdup(
		    env.dmRecipientRefNumber.toUtf8().constData());
		if (NULL == envelope->dmRecipientRefNumber) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (env._using_dmLegalTitleLaw) {
		envelope->dmLegalTitleLaw =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleLaw) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleLaw = env.dmLegalTitleLaw;
	} else {
		envelope->dmLegalTitleLaw = NULL;
	}
	if (env._using_dmLegalTitleYear) {
		envelope->dmLegalTitleYear =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleYear) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleYear = env.dmLegalTitleYear;
	} else {
		envelope->dmLegalTitleYear = NULL;
	}

	if (!env.dmLegalTitleSect.isEmpty()) {
		envelope->dmLegalTitleSect = strdup(
		    env.dmLegalTitleSect.toUtf8().constData());
		if (NULL == envelope->dmLegalTitleSect) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmLegalTitlePar.isEmpty()) {
		envelope->dmLegalTitlePar = strdup(
		    env.dmLegalTitlePar.toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePar) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!env.dmLegalTitlePoint.isEmpty()) {
		envelope->dmLegalTitlePoint = strdup(
		    env.dmLegalTitlePoint.toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePoint) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	envelope->dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPersonalDelivery) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmPersonalDelivery = env.dmPersonalDelivery;

	/* only OVM can change */
	envelope->dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmAllowSubstDelivery) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmAllowSubstDelivery = env.dmAllowSubstDelivery;

	if (!env.dmType.isEmpty()) {
		envelope->dmType = strdup(env.dmType.toUtf8().constData());
		if (NULL == envelope->dmType) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}

	envelope->dmOVM = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmOVM) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmOVM = env.dmOVM;

	envelope->dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPublishOwnID) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmPublishOwnID = env.dmPublishOwnID;

	return envelope;

fail:
	isds_envelope_free(&envelope);
	return NULL;
}

/*!
 * @brief Converts internal message representation to libbsds'.
 *
 * @param[in] msg Internal message representation.
 * @return Libisds message structure or NULL on failure.
 */
static
struct isds_message *libisdsMessage(const IsdsMessage &msg)
{
	struct isds_message *message;

	message = (struct isds_message *) malloc(sizeof(struct isds_message));
	if (message == NULL) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	memset(message, 0, sizeof(struct isds_message));

	message->documents = libisdsDocuments(msg.documents);
	if (NULL == message->documents) {
		goto fail;
	}

	message->envelope = libisdsEnvelope(msg.envelope);
	if (NULL == message->envelope) {
		goto fail;
	}

	return message;

fail:
	isds_message_free(&message);
	return NULL;
}

void TaskSendMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting send message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	struct isds_message *message = libisdsMessage(m_message);
	if (NULL == message) {
		logErrorNL("Could not create isds_message for account '%s'.",
		    globAccounts[m_userName].accountName().toUtf8().constData());
		return;
	}

	sendMessage(m_userName, *m_dbSet, message, m_recipientName,
	    m_recipientAddress, m_isPDZ, PL_SEND_MESSAGE, &m_resultData);

	isds_message_free(&message);

	emit globMsgProcEmitter.sendMessageFinished(m_userName, m_transactId,
	    m_resultData.result, m_resultData.errInfo,
	    m_resultData.dbIDRecipient, m_resultData.recipientName,
	    m_isPDZ, m_resultData.dmId);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Send message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskSendMessage::Result TaskSendMessage::sendMessage(
    const QString &userName, MessageDbSet &dbSet, struct isds_message *message,
    const QString &recipientName, const QString &recipientAddress, bool isPDZ,
    const QString &progressLabel, TaskSendMessage::ResultData *resultData)
{
	Q_ASSERT(!userName.isEmpty());

	Q_ASSERT(NULL != message);
	Q_ASSERT(NULL != message->envelope);

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	enum TaskSendMessage::Result ret = SM_ERR;

	isds_error status;
	qint64 dmId = -1;
	struct isds_envelope *envelope = message->envelope;
	struct isds_ctx *session = NULL;

	QString isdsError, isdsLongError;

	session = globIsdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		logErrorNL("%s", "Missing ISDS session.");
		ret = SM_ERR;
		goto fail;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 40);

	logInfo("Sending message from user '%s'.\n",
	    userName.toUtf8().constData());
	status = isds_send_message(session, message);
	if (IE_SUCCESS != status) {
		isdsError = isds_error(status);
		isdsLongError = isdsLongMessage(session);
		logErrorNL("Sending message returned status %d: '%s' '%s'.",
		    status, isdsError.toUtf8().constData(),
		    isdsLongError.toUtf8().constData());
		ret = SM_ISDS_ERROR;
		goto fail;
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 70);

	{
		{
			bool ok = false;
			dmId = QString(envelope->dmID).toLongLong(&ok);
			if (!ok) {
				Q_ASSERT(0);
				dmId = -1;
			}
		}

		QDateTime deliveryTime =
		    timevalToDateTime(message->envelope->dmDeliveryTime);

		MessageDb *messageDb = dbSet.accessMessageDb(deliveryTime,
		    true);
		if (0 == messageDb) {
			Q_ASSERT(0);
			ret = SM_DB_INS_ERR;
			goto fail;
		}

		const QString acntDbKey(AccountDb::keyFromLogin(userName));
		const QString dbId(globAccountDbPtr->dbId(acntDbKey));
		const QString senderName(
		    globAccountDbPtr->senderNameGuess(acntDbKey));

		if (!messageDb->msgsInsertNewlySentMessageEnvelope(dmId, dbId,
		        senderName, message->envelope->dbIDRecipient,
		        recipientName, recipientAddress,
		        envelope->dmAnnotation)) {
			logErrorNL(
			    "Cannot insert newly sent message '%" PRId64 "' into database.",
			    dmId);
			ret = SM_DB_INS_ERR;
			goto fail;
		}

		emit globMsgProcEmitter.progressChange(progressLabel, 80);

		Task::storeAttachments(*messageDb, dmId, message->documents);

		emit globMsgProcEmitter.progressChange(progressLabel, 90);
	}

	ret = SM_SUCCESS;

	emit globMsgProcEmitter.progressChange(progressLabel, 100);

fail:
	if (0 != resultData) {
		resultData->result = ret;
		resultData->dbIDRecipient = message->envelope->dbIDRecipient;
		resultData->recipientName = recipientName;
		resultData->dmId = dmId;
		resultData->isPDZ = isPDZ;
		resultData->errInfo =
		    (!isdsError.isEmpty() || !isdsLongError.isEmpty()) ?
		        isdsError + " " + isdsLongError : ""; 
	}

	return ret;
}
