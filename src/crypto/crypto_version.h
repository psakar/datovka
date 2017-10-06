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

#ifndef _CRYPTO_VERSION_H_
#define _CRYPTO_VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The compatibility check is made on observations published on page:
 * https://abi-laboratory.pro/tracker/timeline/openssl/
 * We expect that the libraries are more or less compatible only if:
 * a) the major and minor version numbers match respectively,
 * and
 * b) the combination fix and patch numbers of the run-time library are
 *    higher than or equal to the combination of the respective number
 *    of the compile-time library.
 *
 * E.g.:
 *          compile-time   1.0.1a   1.0.1b   1.0.2a   1.1.0a
 * run-time              |        |        |        |
 * ----------------------+------------------------------------
 * 1.0.1a                |   ok   |   xx   |   xx   |   xx
 * 1.0.1b                |   ok   |   ok   |   xx   |   xx
 * 1.0.2a                |   ok   |   ok   |   ok   |   xx
 * 1.1.0a                |   xx   |   xx   |   xx   |   ok
 */

/*!
 * @brief Compares the versions for compatibility.
 *
 * @note The function is visible only because to make it available for testing.
 *
 * @param[in] run_num Run-time version number.
 * @param[in] cmp_num Compile-time version number.
 * @retval  1 If versions are expected to be incompatible.
 * @retval  0 If versions are likely to be compatible.
 * @retval -1 If an error occurred.
 */
int crypto_lib_ver_compatible(unsigned long run_num, unsigned long cmp_num);

/*!
 * @brief Checks whether the version of the SSL library used at compile time
 *     matches the version of libssl (ssleay32.dll).
 *
 * @retval  1 If versions are expected to be incompatible.
 * @retval  0 If versions are likely to be compatible.
 * @retval -1 If an error occurred.
 */
int crypto_compiled_lib_ver_check(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CRYPTO_VERSION_H_ */
