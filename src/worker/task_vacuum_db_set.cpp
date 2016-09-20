/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QObject>
#include <QThread>

#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_vacuum_db_set.h"

TaskVacuumDbSet::TaskVacuumDbSet(MessageDbSet *dbSet)
    : m_success(false),
    m_error(),
    m_dbSet(dbSet)
{
}

void TaskVacuumDbSet::run(void)
{
	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting vacuum task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = vacuumDbSet(m_dbSet, m_error);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Vacuum task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskVacuumDbSet::vacuumDbSet(MessageDbSet *dbSet, QString &error)
{
	Q_ASSERT(0 != dbSet);

	qint64 maxDbSize = dbSet->underlyingFileSize(MessageDbSet::SC_LARGEST);
	if (maxDbSize == 0) {
		/* Nothing to do. */
		return true;
	}

	/* TODO -- Test available free space. */

	bool ret = dbSet->vacuum();
	if (!ret) {
		error = QObject::tr("Calling vacuum on database set failed.");
	}

	return ret;
}
