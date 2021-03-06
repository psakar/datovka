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

/*
 * This header file must not be included in other header files.
 *
 * Functions in this compilation unit serve for converting types
 * defined in libisds.
 */

#pragma once

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <isds.h>

#include "src/datovka_shared/isds/types.h"

namespace IsdsInternal {

	/*!
	 * @brief Converts data box types.
	 */
	enum Isds::Type::DbType libisdsDbType2DbType(const isds_DbType ibt,
	    bool *ok = Q_NULLPTR);
	isds_DbType dbType2libisdsDbType(enum Isds::Type::DbType bt,
	    bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts OTP method.
	 */
	enum Isds::Type::OtpMethod libisdsOtpMethod2OtpMethod(
	    isds_otp_method iom, bool *ok = Q_NULLPTR);
	isds_otp_method otpMethod2libisdsOtpMethod(
	    enum Isds::Type::OtpMethod om, bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts OTP resolution.
	 */
	enum Isds::Type::OtpResolution libisdsOtpResolution2OtpResolution(
	    isds_otp_resolution ior, bool *ok = Q_NULLPTR);
	isds_otp_resolution otpResolution2libisdsOtpResolution(
	    enum Isds::Type::OtpResolution ores, bool *ok = Q_NULLPTR);

}
