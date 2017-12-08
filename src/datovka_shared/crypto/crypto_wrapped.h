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

#pragma once

#include <QByteArray>
#include <QString>

/*!
 * @brief Returns random salt of given length.
 *
 * @param[in] len Salt length.
 * @return Array of random bytes.
 */
QByteArray randomSalt(unsigned int len);

/*!
 * @brief Computes encoded PIN (password derived from PIN value).
 *
 * @param[in] pinVal User-entered PIN value.
 * @param[in] pinAlg Algorithm identifier to be used to compute encoded PIN.
 * @param[in] pinSalt Salt used to compute encoded PIN.
 * @param[in] codeSize Desired code size.
 * @return Encoded PIN on success, null byte array on error.
 */
QByteArray computePinCode(const QString &pinVal, const QString &pinAlg,
    const QByteArray &pinSalt, int codeSize);

/*!
 * @brief Computes encrypted password using supplied PIN value.
 *
 * @param[in] plainPwd Plain-text password.
 * @param[in] pinVal User-entered PIN value.
 * @param[in] pwdAlg Algorithm identifier to be used to generate cipher key.
 * @param[in] pwdSalt Salt used to compute cipher key.
 * @param[in] pwdIV Cipher initialisation vector data.
 * @return Encrypted password, null byte array on error.
 */
QByteArray encryptPwd(const QString &plainPwd, const QString &pinVal,
    const QString &pwdAlg, const QByteArray &pwdSalt, const QByteArray &pwdIV);

/*!
 * @brief Computes plain-text password from encrypted password.
 *
 * @param[in] encrPwd Encrypted password.
 * @param[in] pinVal User-entered PIN value.
 * @param[in] pwdAlg Algorithm identifier to be used to generate cipher key.
 * @param[in] pwdSalt Salt used to compute cipher key.
 * @param[in] pwdIV Cipher initialisation vector data.
 * @return Plain-text password, null string on error.
 */
QString decryptPwd(const QByteArray &encrPwd, const QString &pinVal,
    const QString &pwdAlg, const QByteArray &pwdSalt, const QByteArray &pwdIV);
