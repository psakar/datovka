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

#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/io/isds_helper.h"
#include "src/worker/task_download_owner_info.h"
#include "src/worker/task_download_password_info.h"
#include "src/worker/task_download_user_info.h"

bool IsdsHelper::getOwnerInfoFromLogin(const QString &userName)
{
	debugFuncCall();

	TaskDownloadOwnerInfo *task =
	    new (std::nothrow) TaskDownloadOwnerInfo(userName);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return false;
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
}

bool IsdsHelper::getUserInfoFromLogin(const QString &userName)
{
	debugFuncCall();

	TaskDownloadUserInfo *task =
	    new (std::nothrow) TaskDownloadUserInfo(userName);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return false;
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
}

bool IsdsHelper::getPasswordInfoFromLogin(const QString &userName)
{
	debugFuncCall();

	TaskDownloadPasswordInfo *task =
	    new (std::nothrow) TaskDownloadPasswordInfo(userName);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return false;
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	bool result = task->m_success;
	delete task;

	return result;
}
