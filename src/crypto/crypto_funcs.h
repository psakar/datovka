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


#ifndef _CRYPTO_FUNCS_H_
#define _CRYPTO_FUNCS_H_


#ifdef __cplusplus
#  include <ctime>
#else
#  include <time.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief Initialises cryptographic back-end.
 *
 * @return  0 on success,
 *         -1 else.
 */
int crypto_init(void);


/*!
 * @brief Check whether certificates have been loaded.
 *
 * @return 1 if certificates have been loaded,
 *         0 if certificates have not been loaded.
 */
int crypto_certificates_loaded(void);


/*!
 * @brief Set certificate revocation list.
 *
 * @param[in] der      Buffer containing DER encoded signed CRL.
 * @param[in] der_size DER size.
 * @return  0 on success,
 *         -1 else.
 */
int crypto_add_crl(const void *der, size_t der_size);


/*!
 * @brief Verifies signature of raw signed message.
 *
 * @param[in] der         Buffer containing DER encoded CMS.
 * @param[in] der_size    DER size.
 * @param[in] verify_cert Set 0 if you don't want to verify certificate.
 * @param[in] crl_check   Set 0 if you don't want to perform CRL check.
 * @return  1 if signature is valid,
 *          0 if signature is invalid,
 *         -1 on other errors.
 */
int raw_msg_verify_signature(const void *der, size_t der_size, int verify_cert,
    int crl_check);


/*!
 * @brief Verifies whether raw message signature is valid at given date.
 *
 * @param[in] der       Buffer containing DER encoded CMS.
 * @param[in] der_size  DER size.
 * @param[in] utc_time  Date against to check the certificate.
 * @param[in] crl_check Whether to check the CRL if available.
 * @return  1 if signature is valid at given date,
 *          0 if signature is invalid at given date,
 *         -1 on other errors.
 */
int raw_msg_verify_signature_date(const void *der, size_t der_size,
    time_t utc_time, int crl_check);


/*!
 * @brief Verifies signature of raw qualified time-stamp and parses
 *     the time-stamp value. Time-stamp format follows RFC 3161.
 *
 * @param[in]  der      Buffer containing DER encoded CMS.
 * @param[in]  der_size DER size.
 * @param[out] utc_time Value of time-stamp if valid.
 * @return  1 if signature is valid,
 *          0 if signature is invalid,
 *         -1 on other errors.
 *
 * @note RFC3161 (ASN.1 encoded).
 *     ISDS provozni rad, appendix 2 -- Manipulace s datovymi
 *     zpravami.
 */
int raw_tst_verify(const void *der, size_t der_size, time_t *utc_time);


/*!
 * @brief Returns X509 certificate structure from certificate obtained from
 *     supplied raw CMS message.
 *
 * @param[in] der      Buffer containing DER encoded CMS.
 * @param[in] der_size DER size.
 * @return Pointer to new certificate structure, NULL on failure.
 */
struct x509_crt * raw_cms_signing_cert(const void *der, size_t der_size);


/*!
 * @brief Destroy certificate structure.
 *
 * @param[in,out] x509_crt X09 certificate structure to destroy.
 */
void x509_crt_destroy(struct x509_crt *x509_crt);


/*!
 * @brief Write X509 certificate in DER format into a buffer.
 *
 * @param[in] x509_crt  X509 certificate.
 * @param[out] der_out  Output buffer.
 * @param[out] out_size Generated buffer size.
 * @return  0 on success,
 *         -1 else.
 */
int x509_crt_to_der(struct x509_crt *x509_crt, void **der_out,
    size_t *out_size);


/*!
 * @brief Read X509 certificate from DER buffer.
 *
 * @param[in] der      DER buffer containing X509 certificate.
 * @param[in] der_size DER size.
 * @return Pointer to new certificate structure, NULL on failure.
 */
struct x509_crt * x509_crt_from_der(const void *der, size_t der_size);


/*!
 * @brief Initialises values inside the structure.
 *
 * @param[in,out] cii Certificate issuer information.
 */
void crt_issuer_info_init(struct crt_issuer_info *cii);


/*!
 * @brief Clear values inside the structure.
 *
 * @param[in,out] cii Certificate issuer information.
 */
void crt_issuer_info_clear(struct crt_issuer_info *cii);


/*!
 * @brief Get information about the certificate issuer.
 *
 * @param[in]  x509_crt X509 certificate.
 * @param[out] cii      Certificate issuer information to be set.
 * @return 0 on success, -1 on error.
 *
 * @note Don;t forget to free the returned value. The function is
 *     implemented because QSslCertificate somehow ignores OU.
 */
int x509_crt_issuer_info(struct x509_crt *x509_crt,
    struct crt_issuer_info *cii);


/*!
 * @brief Get signature algorithm information.
 *
 * @param[in]  x509_crt X509 certificate.
 * @param[out] sa_id    Signature algorithm identifier string.
 * @param[out] sa_name  Signature algorithm name.
 * @return 0 on success, -1 on error.
 *
 * @note Use free() to free all returned values. The function is
 *     implemented because QSslCertificate does not support this functionality.
 */
int x509_crt_algorithm_info(struct x509_crt *x509_crt, char **sa_id,
    char **sa_name);


/*!
 * @brief Read inception and expiry date from X509 certificate.
 *
 * @param[in] x509_crt X509 certificate.
 * @param[out] utc_inception UTC inception time.
 * @param[out] utc_expiration UTC expiration time.
 * @return 0 on success, -1 on error.
 */
int x509_crt_date_info(struct x509_crt *x509_crt, time_t *utc_inception,
    time_t *utc_expiration);


/*!
 * @brief Verify certificate.
 *
 * @param[in] x509_crt X509 certificate.
 * @return  1 if certificate valid,
 *          0 if certificate invalid,
 *         -1 on other errors.
 */
int x509_crt_verify(struct x509_crt *x509_crt);


/*!
 * @brief Tracks certificate verification.
 *
 * @param[in]  x509_crt X509 certificate.
 * @param[out] cvo      Why the validation fails.
 * @return  1 if tracking ended without any error,
 *         -1 on errors.
 */
int x509_crt_track_verification(struct x509_crt *x509_crt,
    struct crt_verif_outcome *cvo);


/*!
 * @brief Converts certificate file from PKCS #12 to PEM format.
 *
 * @note If the PKCS #12 was protected by password, then the generated
 *     PEM file will also be encrypted used the same password.
 *
 * @param[in]  p12       PKCS #12 data containing certificate and private key.
 * @param[in]  p12_size  Size of input data portion.
 * @param[in]  pwd       Password protecting the PKCS #12 data.
 * @param[out] pem_out   Newly generated data in PEM format.
 * @param[out] out_size  Size of the PEM data.
 * @return -1 on error.
 */
int p12_to_pem(const void *p12, size_t p12_size, const char *pwd,
    void **pem_out, size_t *out_size);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _CRYPTO_FUNCS_H_ */
