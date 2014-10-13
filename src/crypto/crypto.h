
#ifndef _CRYPTO_H_
#define _CRYPTO_H_


#include <ctime>


/*
 * @brief Initialises cryptographic back-end.
 */
int init_crypto(void);

/*!
 * @brief Verifies signed message signature.
 *
 * @param[in] raw     Raw signed message buffer.
 * @param[in] raw_len Signed message size.
 * @return  1 if signature is valid,
 *          0 if signature is invalid,
 *         -1 on other errors.
 */
int verify_raw_message_signature(const void *raw, size_t raw_len);

/*!
 * @brief Verifies whether message signature was valid at given date.
 *
 * @param[in] raw       Raw signed message buffer.
 * @param[in] raw_len   Signed message size.
 * @param[in] date      Date against to check the certificate.
 * @param[in] crl_check Whether to check the CRL if available.
 * @return  1 if signature is valid at given date,
 *          0 if signature is invalid at given date,
 *         -1 on other errors.
 */
int verify_raw_message_signature_date(const void *raw, size_t raw_len,
    time_t utc_time, int crl_check);

/*!
 * @brief Verifies qualified time-stamp signature and parses the time-stamp
 *     value. Time-stamp format follows RFC 3161.
 *
 * @param[in] data      Times-tamp data.
 * @param[in] data_len  Size of data in buffer.
 * @param[out] utc_time Value of time-stamp if valid.
 * @return  1 if signature is valid,
 *          0 if signature is invalid,
 *         -1 on other errors.
 */
int verify_qualified_timestamp(const void *data, size_t data_len,
    time_t *utc_time);

#endif /* _CRYPTO_H_ */
