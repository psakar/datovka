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

#include <QSettings>
#include <QString>

/*
 * @brief Encapsulates records management service settings.
 */
class RecordsManagementSettings {
public:
	/*!
	 * @brief Constructor.
	 */
	RecordsManagementSettings(void);

	/*!
	 * @brief Test whether records management service is set.
	 *
	 * @note This method does not actually check the service availability.
	 *
	 * @return True is URL and token are set.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Get service URL.
	 *
	 * @return Service URL.
	 */
	const QString &url(void) const;

	/*!
	 * @brief Set service URL.
	 *
	 * @param[in] url Service URL.
	 */
	void setUrl(const QString &url);

	/*!
	 * @brief Get service token.
	 *
	 * @return Service token.
	 */
	const QString &token(void) const;

	/*!
	 * @brief Set service token.
	 *
	 * @param[in] token Service token.
	 */
	void setToken(const QString &token);

	/*!
	 * @brief Get token encryption algorithm name.
	 *
	 * @return Algorithm name.
	 */
	const QString &tokenAlg(void) const;

	/*!
	 * @brief Set token encryption algorithm name.
	 *
	 * @param[in] tokenAlg Algorithm name.
	 */
	void setTokenAlg(const QString &tokenAlg);

	/*!
	 * @brief Get salt data.
	 *
	 * @return Salt.
	 */
	const QByteArray &tokenSalt(void) const;

	/*!
	 * @brief Set salt data.
	 *
	 * @param[in] tokenSalt data.
	 */
	void setTokenSalt(const QByteArray &tokenSalt);

	/*!
	 * @brief Get initialisation vector data.
	 *
	 * @return Initialisation vector.
	 */
	const QByteArray &tokenIv(void) const;

	/*!
	 * @brief Set initialisation vector data.
	 *
	 * @param[in] tokenIv Initialisation vector.
	 */
	void setTokenIv(const QByteArray &tokenIv);

	/*!
	 * @brief Get encrypted token data.
	 *
	 * @return Encrypted token.
	 */
	const QByteArray &tokenCode(void) const;

	/*!
	 * @brief Set encrypted token data.
	 *
	 * @param[in] tokenCode Encrypted token.
	 */
	void setTokenCode(const QByteArray &tokenCode);

	/*!
	 * @brief Used to decrypt the token.
	 *
	 * @param[in] oldPin PIN value used to decrypt old tokens.
	 */
	void decryptToken(const QString &oldPin);

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] settings Settings structure.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[in]  pinVal PIN value to be used for password encryption.
	 * @param[out] settings Settings structure.
	 */
	void saveToSettings(const QString &pinVal, QSettings &settings) const;

private:
	QString m_url; /*!< Service URL. */
	QString m_token; /*!< Service access token. */
	QString m_tokenAlg; /*!< Cryptographic algorithm used to encrypt the token. */
	QByteArray m_tokenSalt; /*!< Salt data. */
	QByteArray m_tokenIv; /*!< Initialisation vector data. */
	QByteArray m_tokenCode; /*!< Encrypted token. */
};
