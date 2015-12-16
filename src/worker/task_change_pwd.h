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

#ifndef _TASK_CHANGE_PWD_H_
#define _TASK_CHANGE_PWD_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing password change.
 */
class TaskChangePwd : public Task {
public:
	/*!
	 * @brief Constructor, does not use OTP.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] oldPwd   Old password.
	 * @param[in] newPwd   New password.
	 */
	explicit TaskChangePwd(const QString &userName, const QString &oldPwd,
	    const QString &newPwd);

	/*!
	 * @brief Constructor, uses OTP.
	 *
	 * @param[in] userName  Account identifier (user login name).
	 * @param[in] oldPwd    Old password.
	 * @param[in] newPwd    New password.
	 * @param[in] otpMethod OTP login method.
	 * @param[in] otpCode   OTP code.
	 */
	explicit TaskChangePwd(const QString &userName, const QString &oldPwd,
	    const QString &newPwd, int otpMethod, const QString &otpCode);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~TaskChangePwd(void);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	int m_isdsRetError; /*!< Returned error code. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

	QString m_refNumber; /*!< Reference number as returned by ISDS. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskChangePwd(const TaskChangePwd &);
	TaskChangePwd & operator=(const TaskChangePwd &);

	/*!
	 * @brief Change password action.
	 *
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in]     oldPwd    Old password.
	 * @param[in]     newPwd    New password.
	 * @param[in,out] otp       OTP structure.
	 * @param[out]    refNumber Reference number.
	 * @param[out]    error     Error description.
	 * @param[out]    longError Long error description.
	 */
	static
	int changePassword(const QString &userName, const QString &oldPwd,
	    const QString &newPwd, struct isds_otp *otp, QString &refNumber,
	    QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_oldPwd; /*!< Old password. */
	const QString m_newPwd; /*!< New password. */
	struct isds_otp *m_otp; /*!< OTP structure. */
};

#endif /* _TASK_CHANGE_PWD_H_ */
