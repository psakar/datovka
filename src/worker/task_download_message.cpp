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

#include <QThread>

#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h"

TaskDownloadMessage::TaskDownloadMessage(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect,
    qint64 dmId, const QDateTime &dTime)
    : m_result(DM_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect),
    m_mId(dmId, dTime)
{
	Q_ASSERT(0 != dbSet);
	Q_ASSERT(0 <= dmId);
}

void TaskDownloadMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	if ((MSG_RECEIVED != m_msgDirect) && (MSG_SENT != m_msgDirect)) {
		Q_ASSERT(0);
		return;
	}

	if (0 > m_mId.dmId) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	logDebugLv1NL("%s", "-----------------------------------------------");
	logDebugLv1NL("Downloading %s message '%d' for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent", m_mId.dmId,
	    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	m_result = downloadMessage(m_userName, m_mId, true, m_msgDirect,
	    *m_dbSet, m_isdsError, m_isdsLongError, PL_DOWNLOAD_MESSAGE);

	if (DM_SUCCESS == m_result) {
		logDebugLv1NL("Done downloading message '%d' for account '%s'.",
		    m_mId.dmId,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());

		/* Only on successful download. */
		emit globMsgProcEmitter.downloadSuccess(m_userName, m_mId.dmId);
	} else {
		logErrorNL("Downloading message '%d' for account '%s' failed.",
		    m_mId.dmId,
		    AccountModel::globAccounts[m_userName].accountName().toUtf8().constData());

		emit globMsgProcEmitter.downloadFail(m_userName, m_mId.dmId,
		    m_isdsError + " " + m_isdsLongError);
	}

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadDeliveryInfo(
    const QString &userName, qint64 dmId, bool signedMsg, MessageDbSet &dbSet,
    QString &error, QString &longError)
{
	debugFuncCall();

	isds_error status;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DM_ERR;
	}
	struct isds_message *message = NULL;

	enum Result res = DM_ERR;

	if (signedMsg) {
		status = isds_get_signed_delivery_info(session,
		    QString::number(dmId).toUtf8().constData(),
		    &message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		res = DM_ERR;
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
		error = isds_error(status);
		longError = isds_long_message(session);
		res = DM_ISDS_ERROR;
		goto fail;
	}

	Q_ASSERT(NULL != message);

	if (Q_SUCCESS != Task::storeDeliveryInfo(signedMsg, dbSet, message)) {
		res = DM_DB_INS_ERR;
		goto fail;
	}

	isds_message_free(&message);

	return DM_SUCCESS;

fail:
	if (NULL != message) {
		isds_message_free(&message);
	}

	return res;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadMessage(
    const QString &userName, MessageDb::MsgId mId, bool signedMsg,
    enum MessageDirection msgDirect, MessageDbSet &dbSet, QString &error,
    QString &longError, const QString &progressLabel)
{
	debugFuncCall();

	logDebugLv0NL("Trying to download complete message '%d'", mId.dmId);

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	isds_error status;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DM_ERR;
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
		return DM_ERR;
		/*
		status = isds_get_received_message(isdsSessions.session(
		    userName),
		    QString::number(mId.dmId).toUtf8().constData(),
		    &message);
		*/
	}

	emit globMsgProcEmitter.progressChange(progressLabel, 20);

	if (IE_SUCCESS != status) {
		error = isds_error(status);
		longError = isds_long_message(session);
		logErrorNL("Downloading message returned status %d: '%s' '%s'.",
		    status, error.toUtf8().constData(),
		    longError.toUtf8().constData());
		isds_message_free(&message);
		return DM_ISDS_ERROR;
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
	if (DM_SUCCESS == downloadDeliveryInfo(userName, mId.dmId, signedMsg,
	        dbSet, error, longError)) {
		logDebugLv0NL("Delivery info of message '%d' processed.",
		    mId.dmId);
	} else {
		logErrorNL("Processing delivery info of message '%d' failed.",
		    mId.dmId);
	}

	Q_ASSERT(mId.deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(mId.deliveryTime, true);
	Q_ASSERT(0 != messageDb);

	downloadMessageAuthor(userName, mId.dmId, *messageDb, error, longError);

	if (MSG_RECEIVED == msgDirect) {
		/*  Mark this message as downloaded in ISDS */
		if (DM_SUCCESS == markMessageAsDownloaded(userName, mId.dmId,
		        error, longError)) {
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

	return DM_SUCCESS;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadMessageAuthor(
    const QString &userName, qint64 dmId, MessageDb &messageDb,
    QString &error, QString &longError)
{
	isds_error status;

	isds_sender_type *sender_type = NULL;
	char * raw_sender_type = NULL;
	char * sender_name = NULL;

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	status = isds_get_message_sender(session,
	    QString::number(dmId).toUtf8().constData(),
	    &sender_type, &raw_sender_type, &sender_name);

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Downloading author information returned status %d: '%s'.",
		    status, isds_strerror(status));
		error = isds_error(status);
		longError = isds_long_message(session);
		return DM_ISDS_ERROR;
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

	return DM_SUCCESS;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::markMessageAsDownloaded(
    const QString &userName, qint64 dmId, QString &error, QString &longError)
{
	debugFuncCall();

	struct isds_ctx *session = isdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	isds_error status = isds_mark_message_read(session,
	    QString::number(dmId).toUtf8().constData());

	if (IE_SUCCESS != status) {
		error = isds_error(status);
		longError = isds_long_message(session);
		return DM_ISDS_ERROR;
	}
	return DM_SUCCESS;
}
