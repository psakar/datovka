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

#include <QFile>
#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_authenticate_message.h"

TaskAuthenticateMessage::TaskAuthenticateMessage(const QString &userName,
    const QString &fileName)
    : m_result(AUTH_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_data()
{
	Q_ASSERT(!m_userName.isEmpty());

	QFile file(fileName);

	if (file.exists()) {
		if (file.open(QIODevice::ReadOnly)) {
			m_data = file.readAll();
			file.close();
		} else {
			logErrorNL("Couldn't open file '%s'.",
			    fileName.toUtf8().constData());
		}
	}
}

TaskAuthenticateMessage::TaskAuthenticateMessage(const QString &userName,
    const QByteArray &data)
    : m_result(AUTH_ERR),
    m_isdsError(),
    m_isdsLongError(),
    m_userName(userName),
    m_data(data)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskAuthenticateMessage::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting authenticate message task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = authenticateMessage(m_userName, m_data, m_isdsError,
	    m_isdsLongError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Authenticate message task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskAuthenticateMessage::Result TaskAuthenticateMessage::authenticateMessage(
    const QString &userName, const QByteArray &data, QString &error,
    QString &longError)
{
	Q_ASSERT(!userName.isEmpty());

	if (data.isEmpty()) {
		return AUTH_DATA_ERROR;
	}

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return AUTH_ERR;
	}

	isds_error status = isds_authenticate_message(session, data.data(),
	    data.size());

	if (IE_NOTEQUAL == status) {
		return AUTH_NOT_EQUAL;
	} else if (IE_SUCCESS != status) {
		logErrorNL("%s", "Error authenticating message.");
		error = isds_error(status);
		longError = isds_long_message(session);
		return AUTH_ISDS_ERROR;
	}

	return AUTH_SUCCESS;
}
