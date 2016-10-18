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

#ifndef _ISDS_LOGIN_H_
#define _ISDS_LOGIN_H_

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QPair>
#include <QString>

#include "src/io/isds_sessions.h"
#include "src/settings/accounts.h"

/*!
 * @brief Encapsulated ISDS log-in operations.
 */
class IsdsLogin {
	Q_DECLARE_TR_FUNCTIONS(IsdsLogin)
public:
	/*!
	 * @brief Error code.
	 */
	enum ErrorCode {
		EC_OK, /*!< Already logged in or successfully logged in. */
		EC_NO_PWD, /*!< User password is missing. */
		EC_NO_CRT, /*!< No certificate. */
		EC_NO_CRT_AGAIN, /*!< Bad certificate, ask again. */
		EC_NO_CRT_PWD, /*!< User password or certificate is missing. */
		EC_NO_CRT_PWD_AGAIN, /*!< Bad certificate, ask gain. */
		EC_NO_CRT_PPHR, /*!< No certificate pass-phrase supplied. */
		EC_NO_OTP, /*!< Missing OTP. */
		EC_NEED_TOTP_ACK, /*!<
		                   * The user has to confirm by setting an
		                   * empty OTP password that he's willing to
		                   * receive a SMS with a TOTP.
		                   */
		EC_NOT_LOGGED_IN, /*!< Login failed. */
		EC_PARTIAL_SUCCESS, /*!< Additional data required (from SMS). */
		EC_PARTIAL_SUCCESS_AGAIN, /*!< Login failed, try entering SMS code again. */
		EC_ISDS_ERR, /*!< Generic ISDS error. */
		EC_NOT_IMPL, /*!< Login method not implemented. */
		EC_ERR /*!< Generic error code. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param isdsSessions Reference to container holding ISDS sessions.
	 * @paran acntSettings Reference to account properties structure.
	 */
	explicit IsdsLogin(IsdsSessions &isdsSessions,
	    AcntSettings &acntSettings);

	/*!
	 * @brief Performs a login operation.
	 *
	 * @return Error code.
	 */
	enum ErrorCode logIn(void);

	/*!
	 * @brief Return ISDS error message.
	 */
	const QString &isdsErrMsg(void) const;

	/*!
	 * @brief Returns a error description suitable for dialogues.
	 *
	 * @return Pair with error description, first element contains a title,
	 *     second element is a more verbose description.
	 */
	QPair<QString, QString> dialogueErrMsg(void) const;

private:
	/*!
	 * @brief Performs a log-in operation using only a user name and a
	 *     password.
	 *
	 * @return Error code.
	 */
	enum ErrorCode userNamePwd(void);

	/*!
	 * @brief Performs a log-in operation using certificate only.
	 *
	 * @return Error code.
	 */
	enum ErrorCode certOnly(void);

	/*!
	 * @brief Performs a log-in operation using a password and a
	 *     certificate.
	 *
	 * @return Error code.
	 */
	enum ErrorCode certUsrPwd(void);

	/*!
	 * @brief Performs a OTL log-in operation.
	 *
	 * @param[in] userName User name.
	 * @param[in] pwd Password.
	 * @param[in] otpCode One-time password code.
	 * @return Error code.
	 */
	enum ErrorCode otpLogIn(const QString &userName,
	    const QString &pwd, const QString &otpCode);

	/*!
	 * @brief Performs a log-in operation using HOTP.
	 *
	 * @return Error code.
	 */
	enum ErrorCode hotp(void);

	/*!
	 * @brief Requests a SMS with TOTP code.
	 *
	 * @param[in] userName User name.
	 * @param[in] pwd Password.
	 * @return Error code.
	 */
	enum ErrorCode totpRequestSMS(const QString &userName,
	    const QString &pwd);

	/*!
	 * @brief Performs a log-in operation using TOTP.
	 *
	 * @return Error code.
	 */
	enum ErrorCode totp(void);

	/*!
	 * @brief preforms a simplification of the obtained ISDS error code.
	 *
	 * @param[in] isdsErr ISDS error code.
	 * @return Error code.
	 */
	static
	enum ErrorCode isdsErrorToCode(int isdsErr);

	/*!
	 * @brief Converts PKCS #12 certificate into PEM format.
	 *
	 * @note The function creates a new PEM file stored in
	 *     the configuration directory. The path is returned via
	 *     the second parameter.
	 *
	 * @param[in]  p12Path   Path to PKCS #12 certificate file.
	 * @param[in]  certPwd   Password protecting the certificate.
	 * @param[out] pemPath   Returned path to created PEM file.
	 * @param[in]  userName  Account user name, user to name PEM file.
	 * @return True on success, false on error
	 *     (e.g. file does not exist, password error, ...)
	 */
	static
	bool p12CertificateToPem(const QString &p12Path,
	    const QString &certPwd, QString &pemPath, const QString &userName);

	/*!
	 * @brief Performs a certificate format conversion if necessary.
	 *
	 * @param[in,out] certPath Path to certificate. Value is changed when
	 *                         conversion is preformed.
	 * @param[in]     passphrase Certificate password.
	 * @Param[in]     userName Account user name.
	 * @return False on error.
	 */
	static
	bool convertAndCheckCert(QString &certPath, const QString &passphrase,
	    const QString &userName);

	IsdsSessions &m_isdsSessions; /*!< Reference to ISDS sessions. */
	AcntSettings &m_acntSettings; /*!< Reference to account properties. */

	int m_isdsErr; /*!< ISDS error. */
	QString m_isdsErrStr; /*!< ISDS error string. */
	QString m_isdsLongErrMsg; /*!< ISDS error message. */

	/*!
	 * @brief Describes the state of a TOTP authentication.
	 */
	enum TotpState {
		TS_START, /*!< Initial state. */
		TS_AWAIT_USR_ACK, /*!<
		                   * The user has to pass empty non-null OTP
		                   * to proceed to next state.
		                   */
		TS_AWAIT_TOTP /*!<
		               * The user has to pass non-null OTP in order to
		               * finish the authentication process.
		               */
	};

	enum TotpState m_totpState; /*!<
	                             * Is true only when SMS code has been
	                             * successfully requested.
	                             */
};

#endif /* _ISDS_LOGIN_H_ */
