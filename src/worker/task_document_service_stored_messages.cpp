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
#include <QByteArray>
#include <QDateTime>
#include <QSet>
#include <QThread>

#include "src/document_service/io/document_service_connection.h"
#include "src/document_service/json/entry_error.h"
#include "src/document_service/json/stored_files.h"
#include "src/io/document_service_db.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_document_service_stored_messages.h"

#define IGNORE_SSL_ERRORS true

TaskDocumentServiceStoredMessages::TaskDocumentServiceStoredMessages(
    const QString &urlStr, const QString &tokenStr, enum Operation operation,
    const MessageDbSet *dbSet, const QList<qint64> &exludedDmIds)
    : m_result(DS_DSM_ERR),
    m_id(QString::number(QDateTime::currentMSecsSinceEpoch()) +
        QStringLiteral(":") + QString::number((quint64)this)),
    m_url(urlStr),
    m_token(tokenStr),
    m_operation(operation),
    m_dbSet(dbSet),
    m_exludedDmIds(exludedDmIds)
{
	Q_ASSERT(!m_url.isEmpty() && !m_token.isEmpty());
	Q_ASSERT((DS_UPDATE_STORED == operation) || (Q_NULLPTR != m_dbSet));
}

void TaskDocumentServiceStoredMessages::run(void)
{
	if ((DS_DOWNLOAD_ALL == m_operation) && (Q_NULLPTR == m_dbSet)) {
		Q_ASSERT(0);
		return;
	}

	if (m_url.isEmpty() || m_token.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL(
	    "Starting download stored messages from document service task '%s' in thread '%p'.",
	    m_id.toUtf8().constData(), (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = downloadStoredMessages(m_url, m_token, m_operation, m_dbSet,
	    m_exludedDmIds);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download stored messages from document service task '%s' finished in thread '%p'.",
	    m_id.toUtf8().constData(), (void *) QThread::currentThreadId());

	emit globMsgProcEmitter.documentServiceStoredMessagesFinished(m_id);
}

const QString &TaskDocumentServiceStoredMessages::id(void) const
{
	return m_id;
}

/*!
 * @brief Obtains all message identifiers from message database.
 *
 * @param[in] dbSet Database set to be used to obtain message identifiers.
 * @patam[in] exludedDmIds Message identifiers that should not be queried.
 * @return List of message identifiers.
 */
static
QList<qint64> obtainDbSetDmIds(const MessageDbSet *dbSet,
    const QList<qint64> &exludedDmIds)
{
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return QList<qint64>();
	}

	QSet<qint64> dmIdSet;

	foreach (const MessageDb::MsgId &msgId, dbSet->getAllMessageIDsFromDB()) {
		dmIdSet.insert(msgId.dmId);
	}

	dmIdSet -= exludedDmIds.toSet();
	return dmIdSet.toList();
}

/*!
 * @brief Obtains all message identifiers held in document service database.
 *
 * @patam[in] exludedDmIds Message identifiers that should not be queried.
 * @return List of message identifiers.
 */
static
QList<qint64> obtainHeldDmIds(const QList<qint64> &exludedDmIds)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		Q_ASSERT(0);
		return QList<qint64>();
	}

	QSet<qint64> dmIdSet = globDocumentServiceDbPtr->getAllDmIds().toSet();

	dmIdSet -= exludedDmIds.toSet();
	return dmIdSet.toList();
}

/*!
 * @brief Checks whether response contains all requested entries.
 *
 * @param[in] sfRes Stored files response structure
 * @param[in] sentDmIds Set identifiers.
 * @return True if response contains requested entries.
 */
static
bool receivedRequestedContent(const StoredFilesResp &sfRes,
    const QList<qint64> &sentDmIds)
{
	QSet<qint64> sentDmIdSet(sentDmIds.toSet());

	foreach (const DmEntry &entry, sfRes.dms()) {
		if (!sentDmIdSet.remove(entry.dmId())) {
			logErrorNL(
			    "Obtained response for message '%" PRId64 "'that has not been requested.",
			    entry.dmId());
			return false;
		}
	}

	if (!sentDmIdSet.isEmpty()) {
		logErrorNL("%s", "Did not obtain all requested message entries.");
		return false;
	}

	return true;
}

/*!
 * @brief Store response into database.
 *
 * @param[in] sfRes Stored files response structure.
 * @param[in] clear If true then all messages not held within the service will
 *                  be explicitly removed.
 * @return True on success, false on error.
 */
static
bool storeStoredFilesResponseContent(const StoredFilesResp &sfRes, bool clear)
{
	/* Process only messages. */

	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return false;
	}

	if (!globDocumentServiceDbPtr->beginTransaction()) {
		return false;
	}

	foreach (const DmEntry &entry, sfRes.dms()) {
		if (entry.locations().isEmpty()) {
			/* Not held within the document service. */
			if (clear) {
				globDocumentServiceDbPtr->deleteStoredMsg(
				    entry.dmId());
			} else {
				continue;
			}
		}

		if (!globDocumentServiceDbPtr->updateStoredMsg(entry.dmId(),
		        entry.locations())) {
			logErrorNL(
			    "Could not update information about message '%" PRId64 "'.",
			    entry.dmId());
			goto fail;
		}
	}

	if (!globDocumentServiceDbPtr->commitTransaction()) {
		goto fail;
	}
	return true;

fail:
	globDocumentServiceDbPtr->rollbackTransaction();
	return false;
}

/*!
 * @brief Process response.
 *
 * @param[in]  clear If true then all messages not held within the service will
 *                   be explicitly removed.
 * @param[in]  sfRes Stored files response structure.
 * @param[in]  sentDmIds Set identifiers.
 * @param[out] limit Obtained limit.
 * @return Number of processed responses, negative value on error.
 */
static
int processStoredFilesResponse(bool clear, const StoredFilesResp &sfRes,
    const QList<qint64> &sentDmIds, int &limit)
{
	/* Limit should always be received. */
	if (sfRes.limit() <= 0) {
		Q_ASSERT(0);
		return -1;
	}
	limit = sfRes.limit();

	switch (sfRes.error().code()) {
	case ErrorEntry::ERR_NO_ERROR:
		break;
	case ErrorEntry::ERR_LIMIT_EXCEEDED:
		return 0; /* Nothing was processed. Should ask again. */
		break;
	default:
		logErrorNL("Received error '%s'.", sfRes.error().trVerbose());
		return -1;
		break;
	}

	if (!receivedRequestedContent(sfRes, sentDmIds)) {
		return -1;
	}

	if (!storeStoredFilesResponseContent(sfRes, clear)) {
		return -1;
	}

	return sfRes.dms().size();
}

/*!
 * @brief Call document service and process response.
 *
 * @param[in]  clear If true then all messages not held within the service will
 *                   be explicitly removed.
 * @param[in]  dsc Document service connection.
 * @param[in]  dmIds Message identifiers.
 * @param[out] limit Obtained limit.
 * @return Number of processed responses, negative value on error.
 */
static
int callStoredFiles(bool clear, DocumentServiceConnection &dsc,
    const QList<qint64> &dmIds, int &limit)
{
	if (dmIds.isEmpty()) {
		Q_ASSERT(0);
		return -1;
	}

	StoredFilesReq sfReq(dmIds, QList<qint64>());
	if (!sfReq.isValid()) {
		logErrorNL("%s", "Could not create stored_files request.");
		return -1;
	}

	QByteArray response;

	if (dsc.communicate(DocumentServiceConnection::SRVC_STORED_FILES,
	        sfReq.toJson(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			StoredFilesResp sfRes(StoredFilesResp::fromJson(response, &ok));
			if (!ok || !sfRes.isValid()) {
				logErrorNL("%s",
				    "Communication error. Received invalid response to stored_files service.");
				logErrorNL("Invalid response '%s'.", response.constData());
				return -1;
			}

			return processStoredFilesResponse(clear, sfRes, dmIds,
			    limit);
		} else {
			logErrorNL("%s",
			    "Communication error. Received empty response to stored_files service.");
			return -1;
		}
	} else {
		logErrorNL("%s",
		    "Communication error when attempting to access stored_files service.");
		return -1;
	}
}

/*!
 * @brief Calls stored files service for all supplied message identifiers.
 *
 * @param[in] urlStr Document service URL.
 * @param[in] tokenStr Document service access token.
 * @param[in] dmIds Message identifiers.
 * @param[in] clear If true then all messages not held within the service will
 *                  be explicitly removed.
 * @return Processing state.
 */
static
enum TaskDocumentServiceStoredMessages::Result updateMessages(
    const QString &urlStr, const QString &tokenStr, const QList<qint64> &dmIds,
    bool clear)
{
	if (dmIds.isEmpty()) {
		return TaskDocumentServiceStoredMessages::DS_DSM_SUCCESS;
	}

	DocumentServiceConnection dsc(IGNORE_SSL_ERRORS);
	dsc.setConnection(urlStr, tokenStr);

	int pos = 0; /* Position. */
	int currentLimit = 1; /* Start with smallest query to obtain maximal size. */
	int nextLimit = 0;

	/* While list is not processed. */
	while ((pos >= 0) && (pos < dmIds.size())) {
		QList<qint64> queryList(dmIds.mid(pos, currentLimit));

		int ret = callStoredFiles(clear, dsc, queryList, nextLimit);
		if (ret < 0) {
			return TaskDocumentServiceStoredMessages::DS_DSM_ERR;
		}
		pos += ret;
		currentLimit = nextLimit; /* Update limit. */
	}

	return TaskDocumentServiceStoredMessages::DS_DSM_ERR;
}

enum TaskDocumentServiceStoredMessages::Result
TaskDocumentServiceStoredMessages::downloadStoredMessages(
    const QString &urlStr, const QString &tokenStr, enum Operation operation,
    const MessageDbSet *dbSet, const QList<qint64> &exludedDmIds)
{
	if ((DS_DOWNLOAD_ALL == operation) && (Q_NULLPTR == dbSet)) {
		Q_ASSERT(0);
		return DS_DSM_ERR;
	}

	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return DS_DSM_DB_INS_ERR;
	}

	QList<qint64> dmIds;
	if (DS_DOWNLOAD_ALL == operation) {
		dmIds = obtainDbSetDmIds(dbSet, exludedDmIds);
	} else {
		dmIds = obtainHeldDmIds(exludedDmIds);
	}
	return updateMessages(urlStr, tokenStr, dmIds,
	    DS_UPDATE_STORED == operation);
}
