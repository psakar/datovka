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

#include <assert.h>
#include <limits.h>
#include <openssl/evp.h>

#include "src/datovka_shared/crypto/crypto_pin.h"

/* Do not change these value as it will break PIN checking. */
#define PBKDF2_SHA256_NAME "PBKDF2-HMAC-SHA-256-64"
#define PBKDF2_SHA256_ITER 1000
#define PBKDF2_SHA256_OUT 64

/*!
 * @brief PBKDF2-SHA256.
 */
static
int pbkdf2_sha256_func(const void *in, size_t in_len,
    const void *salt, size_t salt_len, int iter, void *out, size_t out_len)
{
	if ((in_len > INT_MAX) || (salt_len > INT_MAX) || (out_len > INT_MAX)) {
		/*
		 * These arguments are passes as int into underlying function.
		 */
		return -1;
	}

	return PKCS5_PBKDF2_HMAC(in, in_len, salt, salt_len, iter,
	    EVP_sha256(), out_len, out) == 1 ? 0 : -1;
}

const struct pin_alg pbkdf2_sha256 = {
	.name = PBKDF2_SHA256_NAME,
	.iter = PBKDF2_SHA256_ITER,
	.out_len = PBKDF2_SHA256_OUT,

	.func = pbkdf2_sha256_func
};
