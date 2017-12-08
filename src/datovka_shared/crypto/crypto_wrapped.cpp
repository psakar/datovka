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

#include "src/datovka_shared/crypto/crypto_pin.h"
#include "src/datovka_shared/crypto/crypto_pwd.h"
#include "src/datovka_shared/crypto/crypto_wrapped.h"

QByteArray randomSalt(unsigned int len)
{
	QByteArray salt;

	/* Make sure that random generator is initialised. */

	for (unsigned int i = 0; i < len; ++i) {
		salt.append(qrand() % 256);
	}

	return salt;
}

QByteArray computePinCode(const QString &pinVal, const QString &pinAlg,
    const QByteArray &pinSalt, int codeSize)
{
	unsigned char hash[PIN_HASH_MAX];

	if (pinVal.isEmpty()) {
		return QByteArray();
	}

	/* Currently there is only one algorithm. */
	if (pinAlg != pbkdf2_sha256.name) {
		return QByteArray();
	}

	if (codeSize > PIN_HASH_MAX) {
		return QByteArray();
	}

	/* Compute PIN code. */
	QByteArray pin(pinVal.toUtf8());
	if (0 != pbkdf2_sha256.func(pin.constData(), pin.size(),
	             pinSalt.constData(), pinSalt.size(),
	             pbkdf2_sha256.iter, hash, codeSize)) {
		return QByteArray();
	}
	return QByteArray((char *)hash, codeSize);
}

/*!
 * @brief Computes cipher key.
 *
 * @param[in] pinVal PIN value.
 * @param[in] pwdSalt Salt value.
 * @param[in] pwdAlgDesc Cipher descriptor (contains key length).
 */
static
QByteArray computeCiphKey(const QString &pinVal, const QByteArray &pwdSalt,
    const struct pwd_alg *pwdAlgDesc)
{
	if (pinVal.isEmpty() || pwdSalt.isEmpty() || pwdAlgDesc == NULL) {
		return QByteArray();
	}

	return computePinCode(pinVal, pbkdf2_sha256.name,
	    pwdSalt, pwdAlgDesc->key_len);
}

QByteArray encryptPwd(const QString &plainPwd, const QString &pinVal,
    const QString &pwdAlg, const QByteArray &pwdSalt, const QByteArray &pwdIV)
{
	/* Currently only one cryptographic algorithm is supported. */
	const struct pwd_alg *pwdAlgDesc = &aes256_cbc;
	unsigned char *encrypted = 0;
	QByteArray pPwd(plainPwd.toUtf8());
	unsigned int encryptedMax = 0;
	int encryptedSize = 0;

	QByteArray retVal;

	if (pPwd.isEmpty() || pinVal.isEmpty()) {
		return QByteArray();
	}

	if (pwdAlg != pwdAlgDesc->name) {
		return QByteArray();
	}

	/* Generate cipher key. */
	QByteArray ciphKey(computeCiphKey(pinVal, pwdSalt, pwdAlgDesc));
	if (ciphKey.isEmpty()) {
		return QByteArray();
	}

	if ((pwdIV.size() != 0) && (pwdIV.size() < pwdAlgDesc->iv_len)) {
		return QByteArray();
	}

	encryptedMax = max_encrypted_len(pwdAlgDesc, pPwd.size());
	encrypted = new (std::nothrow) unsigned char[encryptedMax];
	if (encrypted == 0) {
		goto fail;
	}

	encryptedSize = pwdAlgDesc->encr_func(
	    (const unsigned char *)pPwd.constData(), pPwd.size(),
	    (const unsigned char *)ciphKey.constData(),
	    pwdIV.size() ? (const unsigned char *)pwdIV.constData() : NULL,
	    encrypted, encryptedMax);
	if (encryptedSize <= 0) {
		goto fail;
	}

	retVal = QByteArray((char *)encrypted, encryptedSize);
	delete[] encrypted;
	return retVal;

fail:
	delete[] encrypted;
	return QByteArray();
}

QString decryptPwd(const QByteArray &encrPwd, const QString &pinVal,
    const QString &pwdAlg, const QByteArray &pwdSalt, const QByteArray &pwdIV)
{
	/* Currently only one cryptographic algorithm is supported. */
	const struct pwd_alg *pwdAlgDesc = &aes256_cbc;
	unsigned char *plain = 0;
	unsigned int plainMax = 0;
	int plainSize = 0;

	QString retVal;

	if (encrPwd.isEmpty() || pinVal.isEmpty()) {
		return QByteArray();
	}

	if (pwdAlg != pwdAlgDesc->name) {
		return QByteArray();
	}

	/* Generate cipher key. */
	QByteArray ciphKey(computeCiphKey(pinVal, pwdSalt, pwdAlgDesc));
	if (ciphKey.isEmpty()) {
		return QByteArray();
	}

	if ((pwdIV.size() != 0) && (pwdIV.size() < pwdAlgDesc->iv_len)) {
		return QByteArray();
	}

	plainMax = encrPwd.size() + 1;
	plain = new (std::nothrow) unsigned char[plainMax];
	if (plain == 0) {
		goto fail;
	}

	plainSize = pwdAlgDesc->decr_func(
	    (const unsigned char *)encrPwd.constData(), encrPwd.size(),
	    (const unsigned char *)ciphKey.constData(),
	    pwdIV.size() ? (const unsigned char *)pwdIV.constData() : NULL,
	    plain, plainMax);

	retVal = QString::fromUtf8(QByteArray((char *)plain, plainSize));
	delete[] plain;
	return retVal;

fail:
	delete[] plain;
	return QByteArray();
}
