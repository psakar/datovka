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

#pragma once

#include <QString>

#include "src/settings/accounts.h"

#define CREDENTIALS_FNAME "login_credentials.txt"

class LoginCredentials {
public:
	/*!
	 * @brief Constructor.
	 */
	LoginCredentials(void);

	QString boxName; /*!< Data box name. */
	enum AcntSettings::LogInMethod loginType; /*!< Type of log-in procedure. */
	QString userName; /*!< User login. */
	QString pwd; /*!< Password. */
	QString crtPath; /*!< Path to certificate. */
	QString passphrase; /*!< Certificate pass-phrase. */
	QString hotp; /*!< HOTP */
	QString totp; /*!< TOTP */

	/*!
	 * @brief Clear all values.
	 */
	void clearAll(void);

	/*!
	 * @brief Loads content from file containing login data.
	 *
	 * @param[in]  filePath Full name of file.
	 * @param[in]  lineNum Line number to read from
	 * @param[out] userName User name to be set.
	 * @param[out] pwd Password.
	 *
	 * @return true if data could be read
	 */
	bool loadLoginCredentials(const QString &filePath, unsigned lineNum);

private:
	/*!
	 * @brief Check whether file if present and readable.
	 *
	 * @param[in] filePath Path to file.
	 * @return True if file exists and is readable.
	 */
	static
	bool isReadableFile(const QString &filePath);

	/*!
	 * @brief Read given line. Line numbering starts from 0.
	 *
	 * @param[in] filePath Path to file to be read.
	 * @param[in] lineNum Line number.
	 * @preturn Line content or null string if line doe not exist.
	 */
	static
	QString readLine(const QString &filePath, unsigned lineNum);

	/*!
	 * @brief Convert string login type description to enum value.
	 *
	 * @param[in] typeStr Type description string.
	 * @return Login type.
	 */
	static
	enum AcntSettings::LogInMethod typeFromStr(const QString &typeStr);
};
