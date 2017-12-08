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

#include <stdlib.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @nrief Maximal size of buffer to be used to generate any PIN hash into.
 */
#define PIN_HASH_MAX 128

/*!
 * @brief Hash function prototype.
 *
 * @param[in]  in Password/PIN or any value to be hashed.
 * @param[in]  in_len Input length.
 * @param[in]  salt Salt.
 * @param[in]  salt_len Salt length.
 * @param[in]  iter Number of iterations (should be >= 1, RFC 2898 suggests >= 1000).
 * @param[out] out Buffer to write generated password/hash into.
 * @param[in]  out_len Size of output buffer.
 *
 * @retval  0 Success.
 * @retval -1 Error.
 */
typedef int (*key_hash_func_t)(const void *in, size_t in_len,
    const void *salt, size_t salt_len, int iter, void *out, size_t out_len);

/*!
 * @brief Describes the PIN hashing algorithm.
 */
struct pin_alg {
	const char *name; /*!< Algorithm name. */
	int iter; /*!< Brief Number of iterations. */
	int out_len; /*!< Generated output length. */

	key_hash_func_t func; /*!< Hash function. */
};

extern const struct pin_alg pbkdf2_sha256;

#ifdef __cplusplus
} /* extern "C" */
#endif
