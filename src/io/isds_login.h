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
		EC_NO_PWD, /*!< Password is missing. */
		EC_NO_CRT, /*! No certificate. */
		EC_NO_CRT_PPHR, /*!< No certificate pass-phrase supplied. */
		EC_NOT_IMPL, /*!< Login method not implemented. */
		EC_NOT_LOGGED_IN, /*!< Login failed. */
		EC_PARTIAL_SUCCESS, /*!< Additional data required. */
		EC_ISDS_ERR, /*!< Generic ISDS error. */
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
	 * @brief Performs a log-in using only a user name and a password.
	 *
	 * @return Error code.
	 */
	enum ErrorCode userNamePwd(void);

	/*!
	 * @brief Performs a log-in using certificate only.
	 *
	 * @return Error code.
	 */
	enum ErrorCode certOnly(void);

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

	IsdsSessions &m_isdsSessions; /*!< Reference to ISDS sessions. */
	AcntSettings &m_acntSettings; /*!< Reference to account properties. */

	int m_isdsErr; /*!< ISDS error. */
	QString m_isdsErrStr; /*!< ISDS error string. */
	QString m_isdsLongErrMsg; /*!< ISDS error message. */
};

#endif /* _ISDS_LOGIN_H_ */
