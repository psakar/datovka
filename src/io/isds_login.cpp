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

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "src/crypto/crypto_funcs.h"
#include "src/io/isds_login.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"

IsdsLogin::IsdsLogin(IsdsSessions &isdsSessions, AcntSettings &acntSettings)
    : m_isdsSessions(isdsSessions),
    m_acntSettings(acntSettings),
    m_isdsErr(IE_ERROR),
    m_isdsErrStr(),
    m_isdsLongErrMsg(),
    m_totpState(TS_START)
{
}

enum IsdsLogin::ErrorCode IsdsLogin::logIn(void)
{
	m_isdsErr = IE_ERROR;
	m_isdsErrStr.clear();
	m_isdsLongErrMsg.clear();

	if (!m_acntSettings.isValid()) {
		logErrorNL("%s", "Invalid account.");
		return EC_ERR;
	}

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	if (!m_isdsSessions.holdsSession(userName)) {
		logErrorNL("Non-existent session for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_ERR;
	}

	if (m_isdsSessions.isConnectedToIsds(userName)) {
		logErrorNL("User '%s' not connected into ISDS.",
		    userName.toUtf8().constData());
		return EC_OK;
	}

	switch (m_acntSettings.loginMethod()) {
	case AcntSettings::LIM_UNAME_PWD:
		return userNamePwd();
		break;
	case AcntSettings::LIM_UNAME_CRT:
		return certOnly();
		break;
	case AcntSettings::LIM_UNAME_PWD_CRT:
		return certUsrPwd();
		break;
	case AcntSettings::LIM_UNAME_PWD_HOTP:
		return hotp();
		break;
	case AcntSettings::LIM_UNAME_PWD_TOTP:
		return totp();
		break;
	default:
		logErrorNL("%s", "Log-in method not implemented.");
		return EC_NOT_IMPL;
		break;
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
		        "It is also possible that the ISDS server is inoperative or busy. "
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
		    tr("This may be caused by a missing certificate for the SSL communication or the application cannot open an SSL socket.") +
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
		    tr("This may be caused by a missing SSL certificate needed for communication with the server or it was not possible to establish a secure connection with the ISDS server.") +
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
		    tr("This may be caused by an error in SOAP or the XML content for this web service is invalid.") +
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
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	const QString pwd(m_acntSettings.password());
	if (pwd.isEmpty()) {
		logWarningNL("Missing password for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_PWD;
	}

	m_isdsErr = isdsLoginUserName(m_isdsSessions.session(userName),
	    userName, pwd, m_acntSettings.isTestAccount());
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::certOnly(void)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_CRT);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	QString certPath(m_acntSettings.p12File());
	if (certPath.isEmpty()) {
		logWarningNL("Missing certificate for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_CRT;
	}

	const QString passphrase(m_acntSettings._passphrase());
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (passphrase.isNull()) {
		logWarningNL(
		    "Missing certificate pass-phrase for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_CRT_PPHR;
	}

	if (!convertAndCheckCert(certPath, passphrase, userName)) {
		return EC_NO_CRT_AGAIN;
	}

	m_isdsErr = isdsLoginSystemCert(m_isdsSessions.session(userName),
	    certPath, passphrase, m_acntSettings.isTestAccount());
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::certUsrPwd(void)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_CRT);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	const QString pwd(m_acntSettings.password());
	QString certPath(m_acntSettings.p12File());
	if (pwd.isEmpty() || certPath.isEmpty()) {
		logWarningNL(
		    "Missing user password or certificate for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_CRT_PWD;
	}

	const QString passphrase(m_acntSettings._passphrase());
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (passphrase.isNull()) {
		logWarningNL(
		    "Missing certificate pass-phrase for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_CRT_PPHR;
	}

	if (!convertAndCheckCert(certPath, passphrase, userName)) {
		return EC_NO_CRT_AGAIN;
	}

	m_isdsErr = isdsLoginUserCertPwd(m_isdsSessions.session(userName),
	    userName, pwd, certPath, passphrase,
	    m_acntSettings.isTestAccount());
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::otpLogIn(const QString &userName,
    const QString &pwd, const QString &otpCode)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_HOTP ||
	    m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_TOTP);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());
	Q_ASSERT(!otpCode.isNull());

	isds_otp_resolution otpRes = OTP_RESOLUTION_SUCCESS;

	m_isdsErr = isdsLoginUserOtp(m_isdsSessions.session(userName),
	    userName, pwd, m_acntSettings.isTestAccount(),
	    m_acntSettings.loginMethod(), otpCode, otpRes);
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	if (m_isdsErr == IE_NOT_LOGGED_IN) {
		switch (otpRes) {
		case OTP_RESOLUTION_BAD_AUTHENTICATION:
			logWarningNL(
			    "OTP security code for user name '%s' has not been accepted.",
			    userName.toUtf8().constData());
			break;
		default:
			break;
		}
	}

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::hotp(void)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_HOTP);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	const QString pwd(m_acntSettings.password());
	if (pwd.isEmpty()) {
		logWarningNL("Missing password for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_PWD;
	}

	const QString otpCode(m_acntSettings._otp());
	/*
	 * Don't test for isEmpty()!
	 * See difference between isNull() and isEmpty().
	 */
	if (otpCode.isNull()) {
		logWarningNL("Missing OTP code for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_OTP;
	}

	return otpLogIn(userName, pwd, otpCode);
}

enum IsdsLogin::ErrorCode IsdsLogin::totpRequestSMS(const QString &userName,
    const QString &pwd)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_TOTP);
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!pwd.isEmpty());

	isds_otp_resolution otpRes = OTP_RESOLUTION_SUCCESS;

	m_isdsErr = isdsLoginUserOtp(m_isdsSessions.session(userName),
	    userName, pwd, m_acntSettings.isTestAccount(),
	    m_acntSettings.loginMethod(), QString(), otpRes);
	m_isdsErrStr = isdsStrError((isds_error)m_isdsErr);
	m_isdsLongErrMsg = isdsLongMessage(m_isdsSessions.session(userName));

	return isdsErrorToCode(m_isdsErr);
}

enum IsdsLogin::ErrorCode IsdsLogin::totp(void)
{
	Q_ASSERT(m_acntSettings.loginMethod() == AcntSettings::LIM_UNAME_PWD_TOTP);

	const QString userName(m_acntSettings.userName());
	Q_ASSERT(!userName.isEmpty());

	const QString pwd(m_acntSettings.password());
	if (pwd.isEmpty()) {
		logWarningNL("Missing password for user name '%s'.",
		    userName.toUtf8().constData());
		return EC_NO_PWD;
	}

	const QString otpCode(m_acntSettings._otp());

	enum IsdsLogin::ErrorCode ec;

	switch (m_totpState) {
	case TS_START:
		m_totpState = TS_AWAIT_USR_ACK;
		return EC_NEED_TOTP_ACK;
		break;
	case TS_AWAIT_USR_ACK:
		if (otpCode.isNull() || !otpCode.isEmpty()) {
			/* Must be empty to proceed. */
			logWarningNL("%s",
			    "Require empty OTP to confirm SMS sending.");
			return EC_NEED_TOTP_ACK;
		}
		ec = totpRequestSMS(userName, pwd);
		if (ec == EC_PARTIAL_SUCCESS) {
			/* TOTP request successfully sent. */
			m_totpState = TS_AWAIT_TOTP;
		}
		return ec;
		break;
	case TS_AWAIT_TOTP:
		if (otpCode.isNull()) {
			logWarningNL("Missing OTP code for user name '%s'.",
			    userName.toUtf8().constData());
			return EC_PARTIAL_SUCCESS;
		}
		ec = otpLogIn(userName, pwd, otpCode);
		if (ec == EC_NOT_LOGGED_IN) {
			/* Try again. */
			ec = EC_PARTIAL_SUCCESS_AGAIN;
		}
		return ec;
		break;
	default:
		Q_ASSERT(0);
		return EC_ERR;
		break;
	}
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

bool IsdsLogin::p12CertificateToPem(const QString &p12Path,
    const QString &certPwd, QString &pemPath, const QString &userName)
{
	QByteArray p12Data;
	QByteArray pemData;

	pemPath = QString();

	{
		/* Read the data. */
		QFile p12File(p12Path);
		if (!p12File.open(QIODevice::ReadOnly)) {
			return false;
		}

		p12Data = p12File.readAll();
		p12File.close();
	}

	void *pem = NULL;
	size_t pem_size;
	if (0 != p12_to_pem((void *) p12Data.constData(), p12Data.size(),
	        certPwd.toUtf8().constData(), &pem, &pem_size)) {
		return false;
	}

	QFileInfo fileInfo(p12Path);
	QString pemTmpPath = globPref.confDir() + QDir::separator() +
	    userName + "_" + fileInfo.fileName() + "_.pem";

	QFile pemFile(pemTmpPath);
	if (!pemFile.open(QIODevice::WriteOnly)) {
		free(pem); pem = NULL;
		return false;
	}

	if ((long) pem_size != pemFile.write((char *) pem, pem_size)) {
		free(pem); pem = NULL;
		return false;
	}

	free(pem); pem = NULL;

	if (!pemFile.flush()) {
		return false;
	}

	pemFile.close();

	pemPath = pemTmpPath;

	return true;
}

bool IsdsLogin::convertAndCheckCert(QString &certPath,
    const QString &passphrase, const QString &userName)
{
	Q_ASSERT(!certPath.isEmpty());
	Q_ASSERT(!passphrase.isNull());
	Q_ASSERT(!userName.isEmpty());

	const QString ext(QFileInfo(certPath).suffix().toUpper());
	if ("P12" == ext) {
		/* Read PKCS #12 file and convert to PEM. */
		QString createdPemPath;
		if (p12CertificateToPem(certPath, passphrase, createdPemPath,
		        userName)) {
			certPath = createdPemPath;
		} else {
			 /*
			  * The certificate file cannot be decoded by using
			  * the supplied password.
			  */
			logErrorNL("%s", "Cannot decode certificate.");
			return false; /* TODO -- Better error specification. */
		}
	} else if ("PEM" == ext) {
		/* TODO -- Check the pass-phrase. */
	} else {
		/*
		 * The certificate file suffix does not match one of the
		 * supported file formats. Supported suffixes are: p12, pem.
		 */
		logErrorNL("%s", "Certificate format not supported.");
		return false; /* TODO -- Better error specification. */
	}

	return true;
}
