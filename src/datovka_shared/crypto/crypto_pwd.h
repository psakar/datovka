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

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Encryption function prototype.
 *
 * @param[in] in Password or any value to be encrypted.
 * @param[in] in_len Input length.
 * @param[in] key Key value, (minimum) length is specified in descriptor structure.
 * @param[in] in Initialisation vector, can be NULL, (minimum) length is specified in descriptor structure.
 * @param[out] out Buffer to write output data into.
 * @param[in] out_max Output buffer size.
 * @return Number of bytes written to output, -1 on error.
 */
typedef int (*encr_func_t)(const unsigned char *in, int in_len,
    const unsigned char *key, const unsigned char *iv, unsigned char *out,
    int out_max);

/*!
 * @brief Decryption function prototype.
 *
 * @param[in] in Encrypted password or any value to be decrypted.
 * @param[in] in_len Input length.
 * @param[in] key Key value, (minimum) length is specified in descriptor structure.
 * @param[in] in Initialisation vector, can be NULL, (minimum) length is specified in descriptor structure.
 * @param[out] out Buffer to write output data into.
 * @param[in] out_max Output buffer size.
 * @return Number of bytes written to output, -1 on error.
 */
typedef int (*decr_func_t)(const unsigned char *in, int in_len,
    const unsigned char *key, const unsigned char *iv, unsigned char *out,
    int out_max);

/*!
 * @brief Describes the password cryptographic algorithm.
 */
struct pwd_alg {
	const char *name; /*!< Algorithm name. */
	int block_size; /*!< Cipher block size. */
	int key_len; /*!< Required key length. */
	int iv_len; /*!< Requires initialisation vector length. */

	encr_func_t encr_func; /*!< Encryption function. */
	decr_func_t decr_func; /*!< Decryption function. */
};

/*!
 * @brief Computes maximal encrypted output size based on the input and block
 *     size.
 *
 * @param[in] alg Algorithm description.
 * @param[in] in_len Input length.
 */
#define max_encrypted_len(alg, in_len) \
	((alg)->block_size + (in_len) - 1)

extern const struct pwd_alg aes256_cbc;

#ifdef __cplusplus
} /* extern "C" */
#endif
