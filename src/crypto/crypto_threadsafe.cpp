/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <QMutex>

#include "src/crypto/crypto.h"
#include "src/crypto/crypto_nonthreadsafe.h"
#include "src/crypto/crypto_threadsafe.h"


/*
 * @brief OpenSSL guarding mutex.
 *
 * @note OpenSSL has its own mechanisms to guarantee thread safety. This
 *     implementation relies on Qt to provide the locking mechanism rather
 *     than relying on OpenSSL.
 *     TODO -- Check the usage OpenSSL threads.
 */
static
QMutex cryptoMutex(QMutex::NonRecursive);


/* ========================================================================= */
int cryptoInit(void)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = crypto_init();

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int cryptoCertificatesLoaded(void)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = crypto_certificates_loaded();

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int cryptoAddCrl(const void *der, size_t der_size)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = crypto_add_crl(der, der_size);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int rawMsgVerifySignature(const void *der, size_t der_size, int verify_cert,
    int crl_check)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = raw_msg_verify_signature(der, der_size, verify_cert, crl_check);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int rawMsgVerifySignatureDate(const void *der, size_t der_size,
    time_t utc_time, int crl_check)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = raw_msg_verify_signature_date(der, der_size, utc_time,
	    crl_check);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int rawTstVerify(const void *der, size_t der_size, time_t *utc_time)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = raw_tst_verify(der, der_size, utc_time);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
struct x509_crt * rawCmsSigningCert(const void *der, size_t der_size)
/* ========================================================================= */
{
	struct x509_crt *ret;

	cryptoMutex.lock();

	ret = raw_cms_signing_cert(der, der_size);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
void x509CrtDestroy(struct x509_crt *x509_crt)
/* ========================================================================= */
{
	/* No locking needed. */

	x509_crt_destroy(x509_crt);
}


/* ========================================================================= */
int x509CrtToDer(struct x509_crt *x509_crt, void **der_out,
    size_t *out_size)
/* ========================================================================= */
{
	/* No locking needed. */

	return x509_crt_to_der(x509_crt, der_out, out_size);
}


/* ========================================================================= */
struct x509_crt * x509CrtFromDer(const void *der, size_t der_size)
/* ========================================================================= */
{
	/* No locking needed. */

	return x509_crt_from_der(der, der_size);
}


/* ========================================================================= */
void crtIssuerInfoInit(struct crt_issuer_info *cii)
/* ========================================================================= */
{
	/* No locking needed. */

	crt_issuer_info_init(cii);
}


/* ========================================================================= */
void crtIssuerInfoClear(struct crt_issuer_info *cii)
/* ========================================================================= */
{
	/* No locking needed. */

	crt_issuer_info_clear(cii);
}


/* ========================================================================= */
int x509CrtIssuerInfo(struct x509_crt *x509_crt,
    struct crt_issuer_info *cii)
/* ========================================================================= */
{
	/* No locking needed. */

	return x509_crt_issuer_info(x509_crt, cii);
}


/* ========================================================================= */
int x509CrtAlgorithmInfo(struct x509_crt *x509_crt, char **sa_id,
    char **sa_name)
/* ========================================================================= */
{
	/* No locking needed. */

	return x509_crt_algorithm_info(x509_crt, sa_id, sa_name);
}


/* ========================================================================= */
int x509CrtDateInfo(struct x509_crt *x509_crt, time_t *utc_inception,
    time_t *utc_expiration)
/* ========================================================================= */
{
	/* No locking needed. */

	return x509_crt_date_info(x509_crt, utc_inception, utc_expiration);
}


/* ========================================================================= */
int x509CrtVerify(struct x509_crt *x509_crt)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = x509_crt_verify(x509_crt);

	cryptoMutex.unlock();

	return ret;
}


/* ========================================================================= */
int x509CrtTrackVerification(struct x509_crt *x509_crt,
    struct crt_verif_outcome *cvo)
/* ========================================================================= */
{
	int ret;

	cryptoMutex.lock();

	ret = x509_crt_track_verification(x509_crt, cvo);

	cryptoMutex.unlock();

	return ret;
}
