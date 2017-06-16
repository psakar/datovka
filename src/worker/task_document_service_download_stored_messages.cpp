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

#include <QSet>
#include <QThread>

#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_document_service_download_stored_messages.h"

#define IGNORE_SSL_ERRORS true

TaskDocumentServiceDownloadStoredMessages::TaskDocumentServiceDownloadStoredMessages(
    const QString &urlStr, const QString &tokenStr, MessageDbSet *dbSet,
    const QList<qint64> &exludedDmIds)
    : m_result(DS_DSM_ERR),
    m_dsc(IGNORE_SSL_ERRORS),
    m_dbSet(dbSet),
    m_exludedDmIds(exludedDmIds)
{
	Q_ASSERT(!urlStr.isEmpty() && !tokenStr.isEmpty());
	Q_ASSERT(Q_NULLPTR != m_dbSet);

	m_dsc.setConnection(urlStr, tokenStr);
}

void TaskDocumentServiceDownloadStoredMessages::run(void)
{
	if (Q_NULLPTR == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting download stored messages from service info in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = downloadStoredMessages(m_dbSet, m_dsc, m_exludedDmIds);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

static
QList<qint64> obtainDmIds(MessageDbSet *dbSet,
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

static
enum TaskDocumentServiceDownloadStoredMessages::Result updateMessages(
    DocumentServiceConnection &dsc, const QList<qint64> &dmIds)
{
	if (dmIds.isEmpty()) {
		return TaskDocumentServiceDownloadStoredMessages::DS_DSM_SUCCESS;
	}
}

enum TaskDocumentServiceDownloadStoredMessages::Result
TaskDocumentServiceDownloadStoredMessages::downloadStoredMessages(
    MessageDbSet *dbSet, DocumentServiceConnection &dsc,
    const QList<qint64> &exludedDmIds)
{
	if (Q_NULLPTR == dbSet) {
		Q_ASSERT(0);
		return DS_DSM_ERR;
	}

	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return DS_DSM_DB_INS_ERR;
	}

	QList<qint64> dmIds(obtainDmIds(dbSet, exludedDmIds));
	return updateMessages(dsc, dmIds);
}
