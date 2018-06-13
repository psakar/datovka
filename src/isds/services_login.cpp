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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <cstdlib> // malloc
#include <cstring> // memcpy
#include <isds.h>
#include <QFileInfo>

#include "src/datovka_shared/isds/error.h"
#include "src/isds/account_conversion.h" /* Isds::otp_free */
#include "src/isds/error_conversion.h"
#include "src/isds/internal_type_conversion.h"
#include "src/isds/services_internal.h"
#include "src/isds/services_login.h"

Isds::Error Isds::Login::loginUserName(struct isds_ctx *ctx,
    const QString &userName, const QString &pwd, bool testingSession)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || userName.isEmpty() || pwd.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	isds_error ret = isds_login(ctx,
	    testingSession ? isds_testing_locator : isds_locator,
	    userName.toUtf8().constData(), pwd.toUtf8().constData(),
	    NULL, NULL);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		return err;
	}

	err.setCode(Type::ERR_SUCCESS);

	return err;
}

Isds::Error Isds::Login::loginSystemCert(struct isds_ctx *ctx,
    const QString &certPath, const QString &passphrase, bool testingSession)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || certPath.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_pki_credentials *pki_cred = NULL;
	isds_error ret = IE_SUCCESS;

	pki_cred = (struct isds_pki_credentials *)std::malloc(sizeof(*pki_cred));
	if (Q_UNLIKELY(pki_cred == NULL)) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient memory."));
		return err;
	}
	std::memset(pki_cred, 0, sizeof(*pki_cred));

	const QString ext(QFileInfo(certPath).suffix().toUpper());

	if (ext == QStringLiteral("PEM")) {
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else if (ext == QStringLiteral("DER")) {
		pki_cred->certificate_format = PKI_FORMAT_DER;
		pki_cred->key_format = PKI_FORMAT_DER;
	} else if (ext == QStringLiteral("P12")) {
		/* TODO - convert p12 to pem */
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		goto fail;
	}

	pki_cred->engine = NULL;
	pki_cred->certificate = strdup(certPath.toUtf8().constData());
	pki_cred->key = strdup(certPath.toUtf8().constData());
	pki_cred->passphrase = strdup(passphrase.toUtf8().constData());

	ret = isds_login(ctx,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    NULL, NULL, pki_cred, NULL);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		goto fail;
	}

	err.setCode(Type::ERR_SUCCESS);

fail:
	if (pki_cred != NULL) {
		isds_pki_credentials_free(&pki_cred);
	}

	return err;
}

Isds::Error Isds::Login::loginUserCert(struct isds_ctx *ctx,
    const QString &dbId, const QString &certPath, const QString &passphrase,
    bool testingSession)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || dbId.isEmpty() || certPath.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_pki_credentials *pki_cred = NULL;
	isds_error ret = IE_SUCCESS;

	pki_cred = (struct isds_pki_credentials *)std::malloc(sizeof(*pki_cred));
	if (Q_UNLIKELY(pki_cred == NULL)) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient memory."));
		return err;
	}
	std::memset(pki_cred, 0, sizeof(*pki_cred));

	const QString ext(QFileInfo(certPath).suffix().toUpper());

	if (ext == QStringLiteral("PEM")) {
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else if (ext == QStringLiteral("DER")) {
		pki_cred->certificate_format = PKI_FORMAT_DER;
		pki_cred->key_format = PKI_FORMAT_DER;
	} else if (ext == QStringLiteral("P12")) {
		/* TODO - convert p12 to pem */
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		goto fail;
	}

	pki_cred->engine = NULL;
	pki_cred->certificate = strdup(certPath.toUtf8().constData());
	pki_cred->key = strdup(certPath.toUtf8().constData());
	pki_cred->passphrase = strdup(passphrase.toUtf8().constData());

	ret = isds_login(ctx,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    dbId.toUtf8().constData(), NULL, pki_cred, NULL);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		goto fail;
	}

	err.setCode(Type::ERR_SUCCESS);

fail:
	if (pki_cred != NULL) {
		isds_pki_credentials_free(&pki_cred);
	}

	return err;
}

Isds::Error Isds::Login::loginUserCertPwd(struct isds_ctx *ctx,
    const QString &userName, const QString &pwd, const QString &certPath,
    const QString &passphrase, bool testingSession)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || userName.isEmpty() || pwd.isEmpty() ||
	               certPath.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	isds_pki_credentials *pki_cred = NULL;
	isds_error ret = IE_SUCCESS;

	pki_cred = (struct isds_pki_credentials *)std::malloc(sizeof(*pki_cred));
	if (Q_UNLIKELY(pki_cred == NULL)) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient memory."));
		return err;
	}
	std::memset(pki_cred, 0, sizeof(*pki_cred));

	const QString ext(QFileInfo(certPath).suffix().toUpper());

	if (ext == QStringLiteral("PEM")) {
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else if (ext == QStringLiteral("DER")) {
		pki_cred->certificate_format = PKI_FORMAT_DER;
		pki_cred->key_format = PKI_FORMAT_DER;
	} else if (ext == QStringLiteral("P12")) {
		/* TODO - convert p12 to pem */
		pki_cred->certificate_format = PKI_FORMAT_PEM;
		pki_cred->key_format = PKI_FORMAT_PEM;
	} else {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		goto fail;
	}

	pki_cred->engine = NULL;
	pki_cred->certificate = strdup(certPath.toUtf8().constData());
	pki_cred->key = strdup(certPath.toUtf8().constData());
	pki_cred->passphrase = strdup(passphrase.toUtf8().constData());

	ret = isds_login(ctx,
	    testingSession ? isds_cert_testing_locator : isds_cert_locator,
	    userName.toUtf8().constData(), pwd.toUtf8().constData(),
	    pki_cred, NULL);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		goto fail;
	}

	err.setCode(Type::ERR_SUCCESS);

fail:
	if (pki_cred != NULL) {
		isds_pki_credentials_free(&pki_cred);
	}

	return err;
}

Isds::Error Isds::Login::loginUserOtp(struct isds_ctx *ctx,
    const QString &userName, const QString &pwd, bool testingSession,
    enum Type::OtpMethod otpMethod, const QString &otpCode,
    enum Type::OtpResolution &res)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || userName.isEmpty() || pwd.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	bool ok = false;
	struct isds_otp *otp = NULL;
	isds_error ret = IE_SUCCESS;

	otp = (struct isds_otp *)std::malloc(sizeof(*otp));
	if (Q_UNLIKELY(otp == NULL)) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient memory."));
		return err;
	}
	std::memset(otp, 0, sizeof(*otp));

	otp->method = IsdsInternal::otpMethod2libisdsOtpMethod(otpMethod, &ok);
	if (!ok) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
		goto fail;
	}

	if (!otpCode.isEmpty()) {
		const QByteArray optCodeBytes(otpCode.toLocal8Bit());
		const char *old_str =
		    optCodeBytes.constData(); /* '\0' terminated */
		size_t len = strlen(old_str) + 1;
		otp->otp_code = (char *)std::malloc(len);
		if (Q_UNLIKELY(otp->otp_code == NULL)) {
			err.setCode(Type::ERR_ERROR);
			err.setLongDescr(tr("Insufficient memory."));
			goto fail;
		}
		std::memcpy(otp->otp_code, old_str, len);
	}

	ret = isds_login(ctx,
	    testingSession ? isds_otp_testing_locator : isds_otp_locator,
	    userName.toUtf8().constData(), pwd.toUtf8().constData(),
	    NULL, otp);
	/* May return partial success. */
	err.setCode(libisds2Error(ret));
	if (ret != IE_SUCCESS) {
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
	}

	/* Ignoring the conversion status here. */
	res = IsdsInternal::libisdsOtpResolution2OtpResolution(otp->resolution);

fail:
	if (otp != NULL) {
		Isds::otp_free(&otp);
	}

	return err;
}
