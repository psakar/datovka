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

#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_send_message_mojeid.h"


TaskSendMessageMojeId::TaskSendMessageMojeId(
    int accountID, const QList<JsonLayer::Recipient> &recipientList,
    const JsonLayer::Envelope &envelope, const QList<JsonLayer::File> &fileList,
    QList<JsonLayer::SendResult> &resultList)
    :
    m_accountID(accountID),
    m_recipientList(recipientList),
    m_envelope(envelope),
    m_fileList(fileList),
    m_resultList(resultList)
{
}


void TaskSendMessageMojeId::run(void)
{
	logDebugLv0NL("Starting send message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	sendMessage(m_accountID, m_recipientList, m_envelope,
	    m_fileList, m_resultList, PL_SEND_MESSAGE);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Send message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskSendMessageMojeId::Result TaskSendMessageMojeId::sendMessage(
    int accountID, const QList<JsonLayer::Recipient> &recipientList,
    const JsonLayer::Envelope &envelope, const QList<JsonLayer::File> &fileList,
    QList<JsonLayer::SendResult> &resultList, const QString &progressLabel)
{
	QString errStr;

	emit globMsgProcEmitter.progressChange(progressLabel, 0);

	JsonLayer ljsonlayer;
	ljsonlayer.sendMessage(accountID, recipientList, envelope,
	    fileList, resultList, errStr);

	emit globMsgProcEmitter.progressChange(progressLabel, 100);

	return SM_SUCCESS;
}
