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
#include <QThread>

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/types.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/message_conversion.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_download_message.h"

TaskDownloadMessage::TaskDownloadMessage(const QString &userName,
    MessageDbSet *dbSet, enum MessageDirection msgDirect,
    const MessageDb::MsgId &msgId, bool listScheduled, int processFlags)
    : m_result(DM_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_mId(msgId),
    m_userName(userName),
    m_dbSet(dbSet),
    m_msgDirect(msgDirect),
    m_listScheduled(listScheduled),
    m_processFlags(processFlags)
{
	Q_ASSERT(0 != dbSet);
	Q_ASSERT(0 <= msgId.dmId);
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
	logDebugLv1NL("Downloading %s message '%" PRId64 "' for account '%s'.",
	    (MSG_RECEIVED == m_msgDirect) ? "received" : "sent",
	    UGLY_QINT64_CAST m_mId.dmId,
	    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	logDebugLv1NL("%s", "-----------------------------------------------");

	m_result = downloadMessage(m_userName, m_mId, true, m_msgDirect,
	    *m_dbSet, m_isdsError, m_isdsLongError, PL_DOWNLOAD_MESSAGE);

	if (DM_SUCCESS == m_result) {
		logDebugLv1NL(
		    "Done downloading message '%" PRId64 "' for account '%s'.",
		    UGLY_QINT64_CAST m_mId.dmId,
		    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	} else {
		logErrorNL("Downloading message '%" PRId64 "' for account '%s' failed.",
		    UGLY_QINT64_CAST m_mId.dmId,
		    (*GlobInstcs::acntMapPtr)[m_userName].accountName().toUtf8().constData());
	}

	emit GlobInstcs::msgProcEmitterPtr->downloadMessageFinished(m_userName,
	    m_mId.dmId, m_mId.deliveryTime, m_result,
	    (!m_isdsError.isEmpty() || !m_isdsLongError.isEmpty()) ?
	        m_isdsError + " " + m_isdsLongError : "",
	    m_listScheduled, m_processFlags);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadDeliveryInfo(
    const QString &userName, qint64 dmId, bool signedMsg, MessageDbSet &dbSet,
    QString &error, QString &longError)
{
	debugFuncCall();

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	Isds::Error err;
	err.setCode(Isds::Type::ERR_ERROR);
	Isds::Message message;

	if (signedMsg) {
		err = Isds::Service::getSignedDeliveryInfo(session, dmId,
		    message);
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return DM_ERR;
	}

	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading delivery information returned status %d: '%s'.",
		    err.code(), error.toUtf8().constData());
		return DM_ISDS_ERROR;
	}

	Q_ASSERT(!message.isNull());

	if (Q_SUCCESS != Task::storeDeliveryInfo(signedMsg, dbSet, message)) {
		return DM_DB_INS_ERR;
	}

	return DM_SUCCESS;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadMessage(
    const QString &userName, MessageDb::MsgId &mId, bool signedMsg,
    enum MessageDirection msgDirect, MessageDbSet &dbSet, QString &error,
    QString &longError, const QString &progressLabel)
{
	debugFuncCall();

	logDebugLv0NL("Trying to download complete message '%" PRId64 "'",
	    UGLY_QINT64_CAST mId.dmId);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 0);

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	Isds::Error err;
	Isds::Message message;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 10);

	if (signedMsg) {
		/* sent or received message? */
		if (MSG_RECEIVED == msgDirect) {
			err = Isds::Service::signedReceivedMessageDownload(
			    session, mId.dmId, message);
		} else {
			err = Isds::Service::signedSentMessageDownload(
			    session, mId.dmId, message);
		}
	} else {
		Q_ASSERT(0); /* Only signed messages can be downloaded. */
		return DM_ERR;
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 20);

	if ((err.code() != Isds::Type::ERR_SUCCESS) ||
	    (message.isNull()) || (message.envelope().isNull())) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL("Downloading message returned status %d: '%s' '%s'.",
		    err.code(), error.toUtf8().constData(),
		    longError.toUtf8().constData());
		return DM_ISDS_ERROR;
	}

	{
		QString secKeyBeforeDownload(dbSet.secondaryKey(mId.deliveryTime));
		const QDateTime &newDeliveryTime(message.envelope().dmDeliveryTime());
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
			Task::storeMessageEnvelope(msgDirect, dbSet, message.envelope());
		}
		/* Update message delivery time. */
		mId.deliveryTime = newDeliveryTime;
	}

	/* Store the message. */
	Task::storeMessage(signedMsg, msgDirect, dbSet, message, progressLabel);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 90);

	Q_ASSERT(message.envelope().dmId() == mId.dmId);

	/* Download and save delivery info and message events */
	if (DM_SUCCESS == downloadDeliveryInfo(userName, mId.dmId, signedMsg,
	        dbSet, error, longError)) {
		logDebugLv0NL(
		    "Delivery info of message '%" PRId64 "' processed.",
		    UGLY_QINT64_CAST mId.dmId);
	} else {
		logErrorNL(
		    "Processing delivery info of message '%" PRId64 "' failed.",
		    UGLY_QINT64_CAST mId.dmId);
	}

	Q_ASSERT(mId.deliveryTime.isValid());
	MessageDb *messageDb = dbSet.accessMessageDb(mId.deliveryTime, true);
	Q_ASSERT(0 != messageDb);

	downloadMessageAuthor(userName, mId.dmId, *messageDb, error, longError);

	if (MSG_RECEIVED == msgDirect) {
		/*  Mark this message as downloaded in ISDS */
		if (DM_SUCCESS == markMessageAsDownloaded(userName, mId.dmId,
		        error, longError)) {
			logDebugLv0NL(
			    "Message '%" PRId64 "' marked as downloaded.",
			    UGLY_QINT64_CAST mId.dmId);
		} else {
			logErrorNL(
			    "Marking message '%" PRId64 "' as downloaded failed.",
			    UGLY_QINT64_CAST mId.dmId);
		}
	}

	emit GlobInstcs::msgProcEmitterPtr->progressChange(progressLabel, 100);

	logDebugLv0NL("Done with %s().", __func__);

	return DM_SUCCESS;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::downloadMessageAuthor(
    const QString &userName, qint64 dmId, MessageDb &messageDb,
    QString &error, QString &longError)
{
	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	enum Isds::Type::SenderType senderType = Isds::Type::ST_NULL;
	QString senderName;
	Isds::Error err = Isds::Service::getMessageAuthor(session, dmId,
	    senderType, senderName);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL(
		    "Downloading author information returned status %d: '%s'.",
		    err.code(), error.toUtf8().constData());
		return DM_ISDS_ERROR;
	}

	if (messageDb.updateMessageAuthorInfo(dmId, senderType, senderName)) {
		logDebugLv0NL(
		    "Author information of message '%" PRId64 "' were updated.",
		    UGLY_QINT64_CAST dmId);
	} else {
		logErrorNL(
		    "Updating author information of message '%" PRId64 "' failed.",
		    UGLY_QINT64_CAST dmId);
	}

	return DM_SUCCESS;
}

enum TaskDownloadMessage::Result TaskDownloadMessage::markMessageAsDownloaded(
    const QString &userName, qint64 dmId, QString &error, QString &longError)
{
	debugFuncCall();

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return DM_ERR;
	}

	Isds::Error err = Isds::Service::markMessageAsDownloaded(session, dmId);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		return DM_ISDS_ERROR;
	}
	return DM_SUCCESS;
}
