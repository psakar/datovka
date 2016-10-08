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

#include "src/io/isds_login.h"

IsdsLogin::IsdsLogin(IsdsSessions &isdsSessions, AcntSettings &acntSettings)
    : m_isdsSessions(isdsSessions),
    m_acntSettings(acntSettings),
    m_isdsErr(IE_ERROR),
    m_isdsErrStr(),
    m_isdsLongErrMsg()
{
}

enum IsdsLogin::ErrorCode IsdsLogin::logIn(void)
{
	m_isdsErr = IE_ERROR;
	m_isdsErrStr.clear();
	m_isdsLongErrMsg.clear();

	const QString userName(m_acntSettings.userName());
	if (userName.isEmpty()) {
		return EC_ERR;
	}

	if (!m_isdsSessions.holdsSession(userName)) {
		return EC_ERR;
	}

	if (m_isdsSessions.isConnectedToIsds(userName)) {
		return EC_OK;
	}

	if (m_acntSettings.loginMethod() == LIM_USERNAME) {
		return userNamePwd();
	} else {
		return EC_NOT_IMPL;
	}

	return EC_ERR;
}

const QString &IsdsLogin::isdsErrMsg(void) const
{
	return m_isdsLongErrMsg;
}

QPair<QString, QString> IsdsLogin::dialogueErrMsg(void) const
{
	const QString accountName(m_acntSettings.accountName());

	QString title(accountName + ": " +
	    tr("Error when connecting to ISDS server!"));
	QString message;

	switch (m_isdsErr) {
	case IE_NOT_LOGGED_IN:
		title = accountName + ": " +
		    tr("Error during authentication!");
		message =
		    tr("It was not possible to connect to your data box from account \"%1\".").arg(accountName) +
		    "<br/><br/>" + "<b>" +
		    tr("Authentication failed!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("Please check your credentials and login method together with your password.") +
		    " " +
		    tr("It is also possible that your password has expired - "
		        "in this case, you need to use the official ISDS web interface to change it.");
		break;

	case IE_PARTIAL_SUCCESS:
		title = accountName + ": " +
		    tr("Error during OTP authentication!");
		message =
		    tr("It was not possible to connect to your data box from account \"%1\".").arg(accountName) +
		    "<br/><br/>" + "<b>" +
		    tr("OTP authentication failed!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("Please check your credentials together with entered security/SMS code and try again.") +
		    " " +
		    tr("It is also possible that your password has expired - "
		        "in this case, you need to use the official ISDS web interface to change it.");
		break;

	case IE_TIMED_OUT:
		message =
		    tr("It was not possible to establish a connection within a set time.") +
		    "<br/><br/>" + "<b>" +
		    tr("Time-out for connection to server expired!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is either caused by an extremely slow and/or unstable connection or by an improper set-up.") +
		    " " +
		    tr("Please check your internet connection and try again.") +
		    "<br/><br/>" +
		    tr("It might be necessary to use a proxy to connect to the server. "
		        "Also is possible that the ISDS server is inoperative or busy. "
		        "Try again later.");
		break;

	case IE_HTTP:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("HTTPS problem occurred or redirection to server failed!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is usually caused by either lack of internet connectivity or by some problem with the ISDS server.") +
		    "<br/><br/>" +
		    tr("It is possible that the ISDS server is inoperative or busy. "
		        "Try again later.");
		break;

	case IE_ISDS:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("An ISDS server problem occurred or service was not found!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is usually caused by either lack of internet connectivity or by some problem with the ISDS server.") +
		    "<br/><br/>" +
		    tr("It is possible that the ISDS server is inoperative or busy. "
		        "Try again later.");
		break;

	case IE_NETWORK:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("The connection to server failed or a problem with the network occurred!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is usually caused by either lack of internet connectivity or by a firewall on the way.") +
		    " " +
		    tr("Please check your internet connection and try again.") +
		    "<br/><br/>" +
		    tr("It might be necessary to use a proxy to connect to the server. "
		        "If yes, please set it up in the proxy settings menu.");
		break;

	case IE_CONNECTION_CLOSED:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("Problem with HTTPS connection!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is maybe caused by a missing certificate for the SSL communication or the application cannot open an SSL socket.") +
		    "<br/><br/>" +
		    tr("It is also possible that some libraries (e.g. CURL, SSL) may be missing or may be incorrectly configured.");
		break;

	case IE_SECURITY:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("HTTPS problem or security problem!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This is maybe caused by a missing SSL certificate needed for communicating with the server or it was not possible establish a secure connection with the ISDS server.") +
		    "<br/><br/>" +
		    tr("It is also possible that the certificate has expired.");
		break;

	case IE_XML:
	case IE_SOAP:
		message =
		    tr("It was not possible to establish a connection between your computer and the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("SOAP problem or XML problem!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("This may be caused by an error in SOAP or the XML content for this web service is wrong.") +
		    "<br/><br/>" +
		    tr("It is also possible that the ISDS server is inoperative or busy. "
		        "Try again later.");
		break;

	default:
		title = accountName + ": " +
		    tr("Datovka internal error!");
		message =
		    tr("It was not possible to establish a connection to the ISDS server.") +
		    "<br/><br/>" + "<b>" +
		    tr("Datovka internal error!") +
		    "</b>" + "<br/><br/>" +
		    tr("Error: ") + m_isdsLongErrMsg +
		    "<br/><br/>" +
		    tr("An unexpected error occurred. Please restart the application and try again. "
		        "It this doesn't help then you should contact the support for this application.");
		break;
	}

	return QPair<QString, QString>(title, message);
}

enum IsdsLogin::ErrorCode IsdsLogin::userNamePwd(void)
{
	Q_ASSERT(m_acntSettings.loginMethod() == LIM_USERNAME);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	const QString pwd(m_acntSettings.password());
	if (m_acntSettings.password().isEmpty()) {
		return EC_NO_PWD;
	}

	m_isdsErr = isdsLoginUserName(m_isdsSessions.session(userName),
	    userName, pwd, m_acntSettings.isTestAccount());
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::isdsErrorToCode(int isdsErr)
{
	switch (isdsErr) {
	case IE_SUCCESS:
		return EC_OK;
		break;
	case IE_NOT_LOGGED_IN:
		return EC_NOT_LOGGED_IN;
		break;
	case IE_PARTIAL_SUCCESS:
		return EC_PARTIAL_SUCCESS;
		break;
	default:
		return EC_ISDS_ERR;
		break;
	}
}
