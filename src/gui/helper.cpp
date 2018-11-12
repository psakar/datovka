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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/common.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gui/datovka.h"
#include "src/gui/helper.h"
#include "src/io/isds_sessions.h"
#include "src/worker/task_keep_alive.h"

bool GuiHelper::isLoggedIn(QTimer &keepAliveTimer, MainWindow *const mw,
    const QString &username)
{
	if (Q_UNLIKELY(username.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	bool loggedIn = false;
	keepAliveTimer.stop();
	{
		TaskKeepAlive *task =
		    new (std::nothrow) TaskKeepAlive(username);
		if (Q_UNLIKELY(task == Q_NULLPTR)) {
			return false;
		}
		task->setAutoDelete(false);
		GlobInstcs::workPoolPtr->runSingle(task);

		loggedIn = task->m_isAlive;

		delete task;
	}
	if (!loggedIn) {
		if (Q_NULLPTR != mw) {
			loggedIn = mw->connectToIsds(username);
		}
	}
	keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	/* Check the presence of session. */
	if (!GlobInstcs::isdsSessionsPtr->holdsSession(username)) {
		logErrorNL("%s", "Missing ISDS session.");
		loggedIn = false;
	}

	return loggedIn;
}

MessageDbSet *GuiHelper::getDbSet(const QList<Task::AccountDescr> &acntDescrs,
    const QString &username)
{
	if (Q_UNLIKELY(username.isEmpty())) {
		Q_ASSERT(0);
		return Q_NULLPTR;
	}

	MessageDbSet *dbSet = Q_NULLPTR;
	foreach (const Task::AccountDescr &acnt, acntDescrs) {
		if (acnt.userName == username) {
			dbSet = acnt.messageDbSet;
			break;
		}
	}

	return dbSet;
}

void GuiHelper::pingIsdsServer(const QString &username)
{
	if (Q_UNLIKELY(username.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	TaskKeepAlive *task = new (std::nothrow) TaskKeepAlive(username);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return;
	}
	task->setAutoDelete(true);
	GlobInstcs::workPoolPtr->assignHi(task);
}
