

#ifndef _CRYPTO_H_
#define _CRYPTO_H_


#ifdef __cplusplus
#  include <ctime>
#else
#  include <time.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Opaque structures to hide cryptographic implementation details. */
//struct cms_msg;
//struct cms_tst;
struct x509_crt;


/*
 * @brief Initialises cryptographic back-end.
 */
int init_crypto(void);


/*!
 * @brief Verifies signature of raw signed message.
 *
 * @param[in] der      Buffer containing DER encoded CMS.
 * @param[in] der_size DER size.
 * @return  1 if signature is valid,
 *          0 if signature is invalid,
 *         -1 on other errors.
 */
int raw_msg_verify_signature(const void *der, size_t der_size);


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
 * @brief Get information about the certificate issuer.
 *
 * @param[in]  x509_crt X509 certificate.
 * @param[out] o        Organisation string.
 * @param[out] ou       Organisation unit.
 * @param[out] n        Name.
 * @param[out] c        Country.
 * @return 0 on success, -1 on error.
 *
 * @note Use free() to free all returned strings.
 */
int x509_crt_issuer_info(struct x509_crt *x509_crt,
    char **o, char **ou, char **n, char **c);


/*!
 * @brief Verify certificate.
 *
 * @param[in] x509_crt X509 certificate.
 * @return  1 if certificate valid,
 *          0 if certificate invalid,
 *         -1 on other errors.
 */
int x509_crt_verify(struct x509_crt *x509_crt);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _CRYPTO_H_ */
