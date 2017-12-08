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

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/datovka_shared/crypto/crypto_pwd.h"

//EVP_MAX_KEY_LENGTH
//EVP_MAX_IV_LENGTH;

#define AES256_CBC_NAME "AES-256-CBC"
/* Values taken from OpenSSL sources. */
#define AES256_CBC_BS 16
#define AES256_CBC_KL 32
#define AES256_CBC_IVL 16

/*!
 * @brief AES-256-CBC encryption.
 */
static
int aes256_cbc_encr(const unsigned char *in, int in_len,
    const unsigned char *key, const unsigned char *iv, unsigned char *out,
    int out_max)
{
	EVP_CIPHER_CTX *ctx = NULL;

	int len;
	int ciphertext_len;

	if (in == NULL || in_len == 0 || key == NULL || out == NULL) {
		goto fail;
	}

	if (out_max < max_encrypted_len(&aes256_cbc, in_len)) {
		/* Insufficient space to write to. */
		goto fail;
	}

	ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}

	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
		//ERR_print_errors_fp(stderr); // TODO
		return -1;
	}

	if (1 != EVP_EncryptUpdate(ctx, out, &len, in, in_len)) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}
	ciphertext_len = len;

	if (1 != EVP_EncryptFinal_ex(ctx, out + len, &len)) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}
	ciphertext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;

fail:
	if (ctx != NULL) {
		EVP_CIPHER_CTX_free(ctx);
	}
	return -1;
}

/*!
 * @brief AES-256-CBC decryption.
 */
static
int aes256_cbc_decr(const unsigned char *in, int in_len, const unsigned char *key,
    const unsigned char *iv, unsigned char *out, int out_max)
{
	EVP_CIPHER_CTX *ctx = NULL;

	int len;
	int plaintext_len;

	if (in == NULL || in_len == 0 || key == NULL || out == NULL) {
		goto fail;
	}

	if (out_max < in_len) {
		/* Insufficient space to write to. */
		goto fail;
	}

	ctx = EVP_CIPHER_CTX_new();
	if(ctx == NULL) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}

	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}

	if (1 != EVP_DecryptUpdate(ctx, out, &len, in, in_len)) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}
	plaintext_len = len;

	if (1 != EVP_DecryptFinal_ex(ctx, out + len, &len)) {
		//ERR_print_errors_fp(stderr); // TODO
		goto fail;
	}
	plaintext_len += len;

	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len;

fail:
	if (ctx != NULL) {
		EVP_CIPHER_CTX_free(ctx);
	}
	return -1;
}

const struct pwd_alg aes256_cbc = {
	.name = AES256_CBC_NAME,
	.block_size = AES256_CBC_BS,
	.key_len = AES256_CBC_KL,
	.iv_len = AES256_CBC_IVL,

	.encr_func = aes256_cbc_encr,
	.decr_func = aes256_cbc_decr
};
