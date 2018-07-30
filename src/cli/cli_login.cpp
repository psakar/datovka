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

#include "src/common.h" /* ISDS_CONNECT_TIMEOUT_MS */
#include "src/cli/cli_login.h"
#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/io/account_db.h"
#include "src/io/isds_helper.h"
#include "src/io/isds_login.h"
#include "src/settings/preferences.h"

/*!
 * @brief Performs a ISDS log-in operation.
 *
 * @param[in,out] isdsSessions Sessions container reference.
 * @param[in]     acntSettings Account settings reference.
 * @param[in]     pwd User password is used when no password in settings.
 * @param[in]     key Either certificate key or OTP key.
 * @return True on successful login.
 */
static
bool logInCLI(IsdsSessions &isdsSessions, AcntSettings &acntSettings,
    const QString &pwd, const QString &key)
{
	if (!acntSettings.isValid()) {
		return false;
	}

	{
		const QString userName(acntSettings.userName());
		if (userName.isEmpty()) {
			Q_ASSERT(0);
			return false;
		}
		/* Create clean session if session doesn't exist. */
		if (!isdsSessions.holdsSession(userName)) {
			isdsSessions.createCleanSession(userName,
			    ISDS_CONNECT_TIMEOUT_MS);
		}
	}

	enum IsdsLogin::ErrorCode errCode;
	IsdsLogin loginCtx(isdsSessions, acntSettings);

	/* SMS-authentication is not supported. */

	do {
		errCode = loginCtx.logIn();

		switch (errCode) {
		case IsdsLogin::EC_OK:
			/* Do nothing. */
			break;
		case IsdsLogin::EC_NO_PWD:
			if (!pwd.isEmpty()) {
				acntSettings.setPassword(pwd);
			} else {
				logErrorNL("Missing password for account '%s'.",
				    acntSettings.accountName().toUtf8().constData());
				return false;
			}
			break;
		case IsdsLogin::EC_NO_CRT:
			logErrorNL(
			    "Missing or bad certificate for account '%s'.",
			    acntSettings.accountName().toUtf8().constData());
			return false;
			break;
		case IsdsLogin::EC_NO_CRT_PWD:
			if (acntSettings.p12File().isEmpty()) {
				logErrorNL(
				    "Missing or bad certificate for account '%s'.",
				    acntSettings.accountName().toUtf8().constData());
				return false;
			}
			if (acntSettings.password().isEmpty()) {
				if (!pwd.isEmpty()) {
					acntSettings.setPassword(pwd);
				} else {
					logErrorNL(
					    "Missing password for account '%s'.",
					    acntSettings.accountName().toUtf8().constData());
					return false;
				}
			}
			break;
		case IsdsLogin::EC_NO_CRT_PPHR:
			if (!key.isNull()) {
				acntSettings._setPassphrase(key);
			} else {
				logErrorNL(
				    "Unknown certificate pass-phrase for account '%s'.",
				    acntSettings.accountName().toUtf8().constData());
				return false;
			}
			break;
		case IsdsLogin::EC_NO_OTP:
			if (!key.isEmpty()) {
				acntSettings._setOtp(key);
			} else {
				logErrorNL("Missing OTP code for account '%s'.",
				    acntSettings.accountName().toUtf8().constData());
				return false;
			}
			break;
		default:
			logErrorNL(
			    "Received log-in error code %d for account '%s'.",
			    errCode,
			    acntSettings.accountName().toUtf8().constData());
			return false;
			break;
		}
	} while (errCode != IsdsLogin::EC_OK);

	if (errCode != IsdsLogin::EC_OK) {
		Q_ASSERT(0);
		return false;
	}

	return true;
}

bool connectToIsdsCLI(IsdsSessions &isdsSessions, AcntSettings acntSettings,
    const QString &pwd, const QString &key)
{
	if (!logInCLI(isdsSessions, acntSettings, pwd, key)) {
		return false;
	}

	/* Logged in. */

	const QString userName(acntSettings.userName());
	if (userName.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	IsdsHelper::getOwnerInfoFromLogin(userName);
	IsdsHelper::getUserInfoFromLogin(userName);
	IsdsHelper::getPasswordInfoFromLogin(userName);

	/* Check password expiration. */
	if (!acntSettings._pwdExpirDlgShown()) {
		/* Notify only once. */
		acntSettings._setPwdExpirDlgShown(true);

		int daysTo = GlobInstcs::accntDbPtr->pwdExpiresInDays(
		    AccountDb::keyFromLogin(userName),
		    PWD_EXPIRATION_NOTIFICATION_DAYS);

		if (daysTo >= 0) {
			logWarningNL(
			    "Password for user '%s' of account '%s' expires in %d days.",
			    userName.toUtf8().constData(),
			    acntSettings.accountName().toUtf8().constData(),
			    daysTo);
		}
	}

	/* Set longer time-out. */
	isdsSessions.setSessionTimeout(userName,
	    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);

	return true;
}
