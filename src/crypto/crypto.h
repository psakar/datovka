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

#pragma once

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

/*!
 * @brief Certificate issuer information.
 */
struct crt_issuer_info {
	char *o; /*!< Organisation name. */
	char *ou; /*!< Organisation unit name. */
	char *n; /*!< Common name. */
	char *c; /*!< Country name. */
};

/*!
 * @brief Certificate verification outcome structure.
 */
struct crt_verif_outcome {
	int parent_crt_not_found;
	int time_validity_fail;
	int crt_revoked;
	int crt_signature_invalid;
};

#ifdef __cplusplus
} /* extern "C" */
#endif
