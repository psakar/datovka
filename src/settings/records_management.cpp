/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include "src/crypto/crypto_pwd.h"
#include "src/crypto/crypto_wrapped.h"
#include "src/log/log.h"
#include "src/settings/records_management.h"

/* Records management configuration entry names. */
namespace RMNames {
	const QLatin1String rmGroup("records_management");

	const QLatin1String url("location_url");
	const QLatin1String token("access_token");
	const QLatin1String tokenAlg("access_token_alg");
	const QLatin1String tokenSalt("access_token_salt");
	const QLatin1String tokenIv("access_token_iv");
	const QLatin1String tokenCode("access_token_code");
}

RecordsManagementSettings globRecordsManagementSet;

RecordsManagementSettings::RecordsManagementSettings(void)
    : m_url(),
    m_token(),
    m_tokenAlg(),
    m_tokenSalt(),
    m_tokenIv(),
    m_tokenCode()
{
}

bool RecordsManagementSettings::isValid(void) const
{
	/* URL and plain or encrypted token. */
	return !m_url.isEmpty() && (!m_token.isEmpty() ||
	    (!m_tokenAlg.isEmpty() && !m_tokenSalt.isEmpty() &&
	     !m_tokenIv.isEmpty() && !m_tokenCode.isEmpty()));
}

const QString &RecordsManagementSettings::url(void) const
{
	return m_url;
}

void RecordsManagementSettings::setUrl(const QString &url)
{
	m_url = url;
}

const QString &RecordsManagementSettings::token(void) const
{
	return m_token;
}

void RecordsManagementSettings::setToken(const QString &token)
{
	m_token = token;
}

const QString &RecordsManagementSettings::tokenAlg(void) const
{
	return m_tokenAlg;
}

void RecordsManagementSettings::setTokenAlg(const QString &tokenAlg)
{
	m_tokenAlg = tokenAlg;
}

const QByteArray &RecordsManagementSettings::tokenSalt(void) const
{
	return m_tokenSalt;
}

void RecordsManagementSettings::setTokenSalt(const QByteArray &tokenSalt)
{
	m_tokenSalt = tokenSalt;
}

const QByteArray &RecordsManagementSettings::tokenIv(void) const
{
	return m_tokenIv;
}

void RecordsManagementSettings::setTokenIv(const QByteArray &tokenIv)
{
	m_tokenIv = tokenIv;
}

const QByteArray &RecordsManagementSettings::tokenCode(void) const
{
	return m_tokenCode;
}

void RecordsManagementSettings::setTokenCode(const QByteArray &tokenCode)
{
	m_tokenCode = tokenCode;
}

void RecordsManagementSettings::decryptToken(const QString &oldPin)
{
	if (!m_token.isEmpty()) {
		/* Token already stored in decrypted form. */
		logDebugLv0NL("%s",
		    "Records management service token already held in decrypted form.");
		return;
	}

	if (oldPin.isEmpty()) {
		/*
		 * Old PIN not given, token already should be in plain format.
		 */
		logDebugLv0NL("%s",
		    "No PIN supplied to decrypt records management service token.");
		return;
	}

	if (!m_tokenAlg.isEmpty() && !m_tokenSalt.isEmpty() &&
	    !m_tokenCode.isEmpty()) {
		logDebugLv0NL("%s",
		    "Decrypting records management service token.");
		QString decrypted(decryptPwd(m_tokenCode, oldPin, m_tokenAlg,
		    m_tokenSalt, m_tokenIv));
		if (decrypted.isEmpty()) {
			logWarningNL("%s",
			    "Failed decrypting records management service token.");
		}

		/* Store token. */
		if (!decrypted.isEmpty()) {
			m_token = decrypted;
		}
	}
}

/*!
 * @brief Restores token value,
 *
 * @note The PIN value may not be known when the settings are read. Therefore
 *     token decryption is performed somewhere else.
 *
 * @param[in,out] rmData Records management data to store token into.
 * @param[in]     settings Settings structure.
 * @param[in]     groupName Settings group name.
 */
static
void readTokenData(RecordsManagementSettings &rmData, const QSettings &settings,
    const QString &groupName)
{
	QString prefix;
	if (!groupName.isEmpty()) {
		prefix = groupName + QLatin1String("/");
	}

	{
		QString token(settings.value(prefix + RMNames::token,
		    QString()).toString());
		rmData.setToken(
		    QString::fromUtf8(QByteArray::fromBase64(token.toUtf8())));
	}

	rmData.setTokenAlg(settings.value(prefix + RMNames::tokenAlg,
	    QString()).toString());

	rmData.setTokenSalt(QByteArray::fromBase64(
	    settings.value(prefix + RMNames::tokenSalt,
	        QString()).toString().toUtf8()));

	rmData.setTokenIv(QByteArray::fromBase64(
	    settings.value(prefix + RMNames::tokenIv,
	        QString()).toString().toUtf8()));

	rmData.setTokenCode(QByteArray::fromBase64(
	    settings.value(prefix + RMNames::tokenCode,
	        QString()).toString().toUtf8()));

	if (!rmData.token().isEmpty() && !rmData.tokenCode().isEmpty()) {
		logWarningNL("%s",
		    "Records management has both encrypted and unencrypted token set.");
	}
}

void RecordsManagementSettings::loadFromSettings(const QSettings &settings)
{
	const QString prefix(RMNames::rmGroup + QLatin1String("/"));

	m_url = settings.value(prefix + RMNames::url, QString()).toString();
	readTokenData(*this, settings, RMNames::rmGroup);

	if (!isValid()) {
		m_url.clear();
		m_token.clear();
		m_tokenAlg.clear();
		m_tokenSalt.clear();
		m_tokenIv.clear();
		m_tokenCode.clear();
	}
}

/*!
 * @brief Stores encrypted token into settings.
 *
 * @param[in]     pinVal PIN value to be used for token encryption.
 * @param[in,out] settings Settings structure.
 * @param[in]     rmData records management data to be stored.
 * @param[in]     token Token to be stored.
 */
static
bool storeEncryptedToken(const QString &pinVal, QSettings &settings,
    const RecordsManagementSettings &rmData, const QString &token)
{
	/* Currently only one cryptographic algorithm is supported. */
	const struct pwd_alg *pwdAlgDesc = &aes256_cbc;

	/* Ignore the algorithm settings. */
	const QString tokenAlg(aes256_cbc.name);
	QByteArray tokenSalt(rmData.tokenSalt());
	QByteArray tokenIV(rmData.tokenIv());

	if (tokenSalt.size() < pwdAlgDesc->key_len) {
		tokenSalt = randomSalt(pwdAlgDesc->key_len);
	}

	if (tokenIV.size() < pwdAlgDesc->iv_len) {
		tokenIV = randomSalt(pwdAlgDesc->iv_len);
	}

	QByteArray tokenCode = encryptPwd(token, pinVal, pwdAlgDesc->name,
	    tokenSalt, tokenIV);
	if (tokenCode.isEmpty()) {
		return false;
	}

	settings.setValue(RMNames::tokenAlg, tokenAlg);
	settings.setValue(RMNames::tokenSalt,
	    QString::fromUtf8(tokenSalt.toBase64()));
	settings.setValue(RMNames::tokenIv,
	    QString::fromUtf8(tokenIV.toBase64()));
	settings.setValue(RMNames::tokenCode,
	    QString::fromUtf8(tokenCode.toBase64()));

	return true;
}

void RecordsManagementSettings::saveToSettings(const QString &pinVal,
    QSettings &settings) const
{
	if (!m_url.isEmpty() && !m_token.isEmpty()) {
		settings.beginGroup(RMNames::rmGroup);

		settings.setValue(RMNames::url, m_url);

		bool writePlainToken = pinVal.isEmpty();
		if (!writePlainToken) {
			writePlainToken = !storeEncryptedToken(pinVal, settings,
			    *this, m_token);
		}
		if (writePlainToken) { /* Only when plain or encryption fails. */
			/* Store unencrypted token. */
			settings.setValue(RMNames::token,
			    QString(m_token.toUtf8().toBase64()));
		}

		settings.endGroup();
	}
}
