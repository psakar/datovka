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


#ifndef _CRYPTO_THREADSAFE_H_
#define _CRYPTO_THREADSAFE_H_


#include <ctime>


/*
 * For function documentation see C sources.
 */


int cryptoInit(void);


int cryptoCertificatesLoaded(void);


int cryptoAddCrl(const void *der, size_t der_size);


int rawMsgVerifySignature(const void *der, size_t der_size, int verify_cert,
    int crl_check);


int rawMsgVerifySignatureDate(const void *der, size_t der_size,
    time_t utc_time, int crl_check);


int rawTstVerify(const void *der, size_t der_size, time_t *utc_time);


struct x509_crt * rawCmsSigningCert(const void *der, size_t der_size);


void x509CrtDestroy(struct x509_crt *x509_crt);


int x509CrtToDer(struct x509_crt *x509_crt, void **der_out,
    size_t *out_size);


struct x509_crt * x509CrtFromDer(const void *der, size_t der_size);


void crtIssuerInfoInit(struct crt_issuer_info *cii);


void crtIssuerInfoClear(struct crt_issuer_info *cii);


int x509CrtIssuerInfo(struct x509_crt *x509_crt,
    struct crt_issuer_info *cii);


int x509CrtAlgorithmInfo(struct x509_crt *x509_crt, char **sa_id,
    char **sa_name);


int x509CrtDateInfo(struct x509_crt *x509_crt, time_t *utc_inception,
    time_t *utc_expiration);


int x509CrtVerify(struct x509_crt *x509_crt);


int x509CrtTrackVerification(struct x509_crt *x509_crt,
    struct crt_verif_outcome *cvo);


#endif /* _CRYPTO_THREADSAFE_H_ */
