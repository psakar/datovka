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

#include <QThread>

#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/isds/type_description.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_change_pwd.h"

TaskChangePwd::TaskChangePwd(const QString &userName, const QString &oldPwd,
    const QString &newPwd, const Isds::Otp &otpData)
    : m_errorCode(Isds::Type::ERR_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_refNumber(),
    m_userName(userName),
    m_oldPwd(oldPwd),
    m_newPwd(newPwd),
    m_otp(otpData)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskChangePwd::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting change password task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_errorCode = changePassword(m_userName, m_oldPwd, m_newPwd, m_otp,
	    m_refNumber, m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Change password task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum Isds::Type::Error TaskChangePwd::changePassword(const QString &userName,
    const QString &oldPwd, const QString &newPwd, Isds::Otp &otp,
    QString &refNumber, QString &error, QString &longError)
{
	Q_ASSERT(!userName.isEmpty());

	Isds::Session *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (Q_UNLIKELY(Q_NULLPTR == session)) {
		Q_ASSERT(0);
		return Isds::Type::ERR_ERROR;
	}

	refNumber.clear();

	Isds::Error err = Isds::Service::changeISDSPassword(session, oldPwd,
	    newPwd, otp, refNumber);
	if (err.code() != Isds::Type::ERR_SUCCESS) {
		error = Isds::Description::descrError(err.code());
		longError = err.longDescr();
		logErrorNL("%s", "Error setting new password.");
	}

	return err.code();
}
