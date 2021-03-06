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

#include <openssl/crypto.h>
#include <openssl/opensslv.h>
#include <QSslSocket>

#include "src/crypto/crypto_version.h"
#include "src/datovka_shared/log/log.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

  #define _SSLEAY_VERSION_NUMBER \
	SSLEAY_VERSION_NUMBER

  /*
   * SSLeay() returns long; hope that treating this value as unsigned will
   * cause no problems.
   */
  #define _SSLeay() \
	SSLeay()

  #define _SSLeay_version_str() \
	SSLeay_version(SSLEAY_VERSION)

#else /* OPENSSL_VERSION_NUMBER >= 0x10100000L */

  #define _SSLEAY_VERSION_NUMBER \
	OPENSSL_VERSION_NUMBER

  #define _SSLeay() \
	OpenSSL_version_num()

  #define _SSLeay_version_str() \
	OpenSSL_version(OPENSSL_VERSION)

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

/* Version of OpenSSL the application has been compiled against. */
static
const unsigned long appSslBuildVerNum = _SSLEAY_VERSION_NUMBER;
static
const QString appSslBuildVerStr(OPENSSL_VERSION_TEXT);

/*!
 * @brief Version structure for convenience purposes.
 */
struct ssl_ver_nums {
	int major;
	int minor;
	int fix;
	int patch;
};

/*!
 * @brief Reads the OpenSSL version number and sets the structure content.
 *
 * @param[out] ver_nums Version number structure.
 * @param[in] ssl_ver Version number.
 * @retval  0 On success.
 * @retval -1 On error.
 */
static
int set_ssl_ver_nums(struct ssl_ver_nums *ver_nums, unsigned long ssl_ver)
{
	if (Q_UNLIKELY(NULL == ver_nums)) {
		Q_ASSERT(0);
		return -1;
	}

	/* See manual page OPENSSL_VERSION_NUMBER(3). */

	/* Ignoring versions prior 0.9.5. */

	/* Ignore status number. */
	ssl_ver = (ssl_ver >> 4);
	ver_nums->patch = ssl_ver & 0x0ff; ssl_ver = (ssl_ver >> 8);
	ver_nums->fix = ssl_ver & 0x0ff; ssl_ver = (ssl_ver >> 8);
	ver_nums->minor = ssl_ver & 0x0ff; ssl_ver = (ssl_ver >> 8);
	ver_nums->major = ssl_ver & 0x0ff;

	return 0;
}

/*!
 * @brief Compares the versions for compatibility.
 *
 * @param[in] run_ver Run-time version number.
 * @param[in] cmp_ver Compile-time version number.
 * @retval  1 If versions are expected to be incompatible.
 * @retval  0 If versions are likely to be compatible.
 * @retval -1 If an error occurred.
 */
static
int lib_ver_compatible(const struct ssl_ver_nums *run_ver,
    const struct ssl_ver_nums *cmp_ver)
{
	if (Q_UNLIKELY((run_ver == NULL) || (cmp_ver == NULL))) {
		Q_ASSERT(0);
		return -1;
	}

	if ((run_ver->major != cmp_ver->major) ||
	    (run_ver->minor != cmp_ver->minor)) {
		return 1;
	}

	if (run_ver->fix < cmp_ver->fix) {
		return 1;
	}

	if ((run_ver->fix == cmp_ver->fix) &&
	    (run_ver->patch < cmp_ver->patch)) {
		return 1;
	}

	return 0;
}

int crypto_lib_ver_compatible(unsigned long run_num, unsigned long cmp_num)
{
	struct ssl_ver_nums run_ver, cmp_ver;

	if (Q_UNLIKELY(0 != set_ssl_ver_nums(&run_ver, run_num))) {
		return -1;
	}
	if (Q_UNLIKELY(0 != set_ssl_ver_nums(&cmp_ver, cmp_num))) {
		return -1;
	}

	return lib_ver_compatible(&run_ver, &cmp_ver);
}

int crypto_compiled_lib_ver_check(void)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	unsigned long qt_cmp_ssl_ver =
	    QSslSocket::sslLibraryBuildVersionNumber();
	struct ssl_ver_nums qt_cmp_nums;
#else /* < Qt-5.4 */
#  warning "Compiling against version < Qt-5.4 which does not provide version number of the SSL library in use at Qt compile time."
#endif /* >= Qt-5.4 */

	unsigned long run_ssl_ver = _SSLeay();
	struct ssl_ver_nums app_cmp_nums, run_nums;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	logInfoNL("Qt compile-time OpenSSL version 0x%lx (%s).", qt_cmp_ssl_ver,
	    QSslSocket::sslLibraryBuildVersionString().toUtf8().constData());
#endif /* >= Qt-5.4 */
	logInfoNL("Application compile-time OpenSSL version 0x%lx (%s).",
	    appSslBuildVerNum, appSslBuildVerStr.toUtf8().constData());
	logInfoNL("Run-time OpenSSL version 0x%lx (%s).",
	    run_ssl_ver, _SSLeay_version_str());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	if (Q_UNLIKELY(0 != set_ssl_ver_nums(&qt_cmp_nums, qt_cmp_ssl_ver))) {
		return -1;
	}
#endif /* >= Qt-5.4 */
	if (Q_UNLIKELY(0 != set_ssl_ver_nums(&app_cmp_nums, appSslBuildVerNum))) {
		return -1;
	}
	if (Q_UNLIKELY(0 != set_ssl_ver_nums(&run_nums, run_ssl_ver))) {
		return -1;
	}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
	int ret = lib_ver_compatible(&run_nums, &qt_cmp_nums);
	if (ret != 0) {
		return ret;
	}
#endif /* >= Qt-5.4 */

	return lib_ver_compatible(&run_nums, &app_cmp_nums);
}
