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

#pragma once

#include <QString>

#include "src/datovka_shared/isds/account_interface.h"
#include "src/isds/types.h"
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
	 * @param[in] oldPwd Old password.
	 * @param[in] newPwd New password.
	 * @param[in] otpData OTP data.
	 */
	explicit TaskChangePwd(const QString &userName, const QString &oldPwd,
	    const QString &newPwd, const Isds::Otp &otpData = Isds::Otp());

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Isds::Type::Error m_errorCode; /*!< Returned error code. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

	QString m_refNumber; /*!< Reference number as returned by ISDS. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskChangePwd(const TaskChangePwd &);
	TaskChangePwd &operator=(const TaskChangePwd &);

	/*!
	 * @brief Change password action.
	 *
	 * @param[in]     userName Account identifier (user login name).
	 * @param[in]     oldPwd Old password.
	 * @param[in]     newPwd New password.
	 * @param[in,out] otp OTP structure.
	 * @param[out]    refNumber Reference number.
	 * @param[out]    error Error description.
	 * @param[out]    longError Long error description.
	 * @return Error code.
	 */
	static
	enum Isds::Type::Error changePassword(const QString &userName,
	    const QString &oldPwd, const QString &newPwd, Isds::Otp &otp,
	    QString &refNumber, QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_oldPwd; /*!< Old password. */
	const QString m_newPwd; /*!< New password. */
	Isds::Otp m_otp; /*!< OTP data. */
};
