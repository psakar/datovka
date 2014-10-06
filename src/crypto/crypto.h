
#ifndef _CRYPTO_H_
#define _CRYPTO_H_


/*
 * @brief Initialises cryptographic back-end.
 */
int init_crypto(void);

/*!
 * @brief Verifies signed message signature.
 *
 * @return  1 if signature is valid,
 *          0 is signature is invalid,
 *         -1 on other errors.
 */
int verify_raw_message_signature(const void *raw, size_t raw_len);


#endif /* _CRYPTO_H_ */
