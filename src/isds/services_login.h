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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QString>

#include "src/datovka_shared/isds/types.h"

namespace Isds {

	/* Forward declaration. */
	class Error;
	class Session;

	/*!
	 * @brief Encapsulates ISDS login operations.
	 */
	class Login {
		Q_DECLARE_TR_FUNCTIONS(Login)

	private:
		/*!
		 * @brief Private constructor.
		 */
		Login(void);

	public:
		/*!
		 * @brief Log in to ISDS using a user name and password.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     userName Login.
		 * @param[in]     pwd Password.
		 * @param[in]     testingSession Set to true to log into
		 *                               testing environment.
		 * @return Error description.
		 */
		static
		Error loginUserName(Session *ctx, const QString &userName,
		    const QString &pwd, bool testingSession);

		/*!
		 * @brief Log in to ISDS using a system certificate.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     certPath Path to certificate file.
		 * @param[in]     passphrase Certificate passphrase.
		 * @param[in]     testingSession Set to true to log into
		 *                               testing environment.
		 * @return Error description.
		 */
		static
		Error loginSystemCert(Session *ctx, const QString &certPath,
		    const QString &passphrase, bool testingSession);

		/*!
		 * @brief Log in to ISDS using a user certificate without
		 *     a password.
		 *
		 * @note It needs the data-box identifier instead of the username.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dbId Data-box identifier.
		 * @param[in]     certPath Path to certificate file.
		 * @param[in]     passphrase Certificate passphrase.
		 * @param[in]     testingSession Set to true to log into
		 *                               testing environment.
		 * @return Error description.
		 */
		static
		Error loginUserCert(Session *ctx, const QString &dbId,
		    const QString &certPath, const QString &passphrase,
		    bool testingSession);

		/*!
		 * @brief Log in to ISDS using a user certificate with
		 *     a password.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     userName Login.
		 * @param[in]     pwd Password.
		 * @param[in]     certPath Path to certificate file.
		 * @param[in]     passphrase Certificate passphrase.
		 * @param[in]     testingSession Set to true to log into
		 *                               testing environment.
		 * @return Error description.
		 */
		static
		Error loginUserCertPwd(Session *ctx, const QString &userName,
		    const QString &pwd, const QString &certPath,
		    const QString &passphrase, bool testingSession);

		/*!
		 * @brief Log in to ISDS using a username, password and OTP.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     userName Login.
		 * @param[in]     pwd Password.
		 * @param[in]     testingSession Set to true to log into
		 *                               testing environment.
		 * @param[in]     otpMethod OTP authentication method.
		 * @param[in]     otpCode OTP code.
		 * @param[out]    res OTP resolution status.
		 * @return Error description.
		 */
		static
		Error loginUserOtp(Session *ctx, const QString &userName,
		    const QString &pwd, bool testingSession,
		    enum Type::OtpMethod otpMethod, const QString &otpCode,
		    enum Type::OtpResolution &res);
	};

}
