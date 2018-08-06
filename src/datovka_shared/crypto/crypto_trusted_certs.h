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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
 * @brief Holds a PEM encoded certificate string and a certificate name.
 */
struct pem_str {
	const char *name; /*!< Certificate name. */
	const char *pem; /*!< PEM-encoded certificate. */
};

/*
 * Holds NULL-terminated list of PEM-encoded certificate files.
 */
extern const char *pem_files[];

/*!
 * @brief Holds NULL-terminated list of PEM-encoded certificates.
 */
extern const struct pem_str all_pem_strs[];

/*!
 * @brief Holds NULL-terminated list of PEM-encoded root certificates.
 *
 * @note These certificates are needed for the validation of message signatures.
 */
extern const struct pem_str msg_pem_strs[];

/*!
 * @brief Holds NULL-terminated list of PEM-encoded root certificates.
 *
 * @note These certificates are needed for the verification of the HTPPS
 *     connection to ISDS.
 */
extern const struct pem_str conn_pem_strs[];

/*!
 * @brief Holds locations of the CRL files.
 */
struct crl_location {
	const char *file_name; /*!< CRL file name. */
	const char **urls; /*!< NULL-terminated list of URLs. */
};

/*!
 * @brief NULL-terminated list of CRL files.
 */
extern const struct crl_location crl_locations[];

#ifdef __cplusplus
} /* extern "C" */
#endif
