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

#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_change_pwd.h"

TaskChangePwd::TaskChangePwd(const QString &userName, const QString &oldPwd,
    const QString &newPwd)
    : m_isdsRetError(IE_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_refNumber(),
    m_userName(userName),
    m_oldPwd(oldPwd),
    m_newPwd(newPwd),
    m_otp(NULL)
{
	Q_ASSERT(!m_userName.isEmpty());
}

TaskChangePwd::TaskChangePwd(const QString &userName, const QString &oldPwd,
    const QString &newPwd, int otpMethod, const QString &otpCode)
    : m_isdsRetError(IE_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_refNumber(),
    m_userName(userName),
    m_oldPwd(oldPwd),
    m_newPwd(newPwd),
    m_otp(NULL)
{
	Q_ASSERT(!m_userName.isEmpty());

	/* Creates OTP structure. */

	struct isds_otp *otp;
	otp = (struct isds_otp *) malloc(sizeof(struct isds_otp));
	if (NULL == otp) {
		goto fail;
	}
	memset(otp, 0, sizeof(struct isds_otp));

	switch (otpMethod) {
	case OTP_HMAC:
		otp->method = OTP_HMAC;
		break;
	case OTP_TIME:
		otp->method = OTP_TIME;
		break;
	default:
		Q_ASSERT(0);
		logErrorNL("Unsupported OTP login method '%d'.", otpMethod);
		goto fail;
		break;
	}

	if (!otpCode.isEmpty()) {
		otp->otp_code = strdup(otpCode.toUtf8().constData());
		if (NULL == otp->otp_code) {
			goto fail;
		}
	}

	m_otp = otp; otp = NULL;

fail:
	if (NULL != otp) {
		free(otp->otp_code);
		free(otp);
	}
}

TaskChangePwd::~TaskChangePwd(void)
{
	if (NULL != m_otp) {
		free(m_otp->otp_code);
		free(m_otp);
	}
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

	m_isdsRetError = changePassword(m_userName, m_oldPwd, m_newPwd, m_otp,
	    m_refNumber, m_isdsError, m_isdsLongError);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Change password task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

int TaskChangePwd::changePassword(const QString &userName,
    const QString &oldPwd, const QString &newPwd, struct isds_otp *otp,
    QString &refNumber, QString &error, QString &longError)
{
	isds_error ret = IE_ERROR;

	Q_ASSERT(!userName.isEmpty());

	struct isds_ctx *session = globIsdsSessions.session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return IE_ERROR;
	}

	char *reference = NULL;
	refNumber.clear();

	ret = isds_change_password(session, oldPwd.toUtf8().constData(),
	    newPwd.toUtf8().constData(), otp, &reference);

	if (IE_SUCCESS != ret) {
		error = isdsStrError(ret);
		longError = isds_long_message(session);
	}

	if (NULL != reference) {
		refNumber = reference;
		free(reference);
	}

	return ret;
}
