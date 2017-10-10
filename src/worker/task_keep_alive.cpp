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

#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/task_keep_alive.h"

TaskKeepAlive::TaskKeepAlive(const QString &userName)
    : m_isAlive(false),
    m_userName(userName)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskKeepAlive::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting keep-alive task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_isAlive = globIsdsSessionsPtr->isConnectedToIsds(m_userName);
	if (m_isAlive) {
		logInfo("%s\n", "Connection to ISDS is alive :)");
	} else {
		logWarning("%s\n", "Connection to ISDS is dead :(");
	}

	/* ### Worker task end. ### */

	logDebugLv0NL("Keep-alive task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}
