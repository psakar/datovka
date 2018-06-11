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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <cstdlib> // malloc
#include <cstring> // memset
#include <isds.h>

#include "src/datovka_shared/isds/internal_conversion.h"
#include "src/isds/account_conversion.h"
#include "src/isds/internal_type_conversion.h"

/*!
 * @brief Converts otp method.
 */
static
enum Isds::Type::OtpMethod libisdsOtpMethod2OtpMethod(isds_otp_method iom,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	enum Isds::Type::OtpMethod method = Isds::Type::OM_UNKNOWN;

	switch (iom) {
	case OTP_HMAC: method = Isds::Type::OM_HMAC; break;
	case OTP_TIME: method = Isds::Type::OM_TIME; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return method;
}

Isds::Otp Isds::libisds2otp(const struct isds_otp *io, bool *ok)
{
	if (Q_UNLIKELY(io == NULL)) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Otp();
	}

	bool iOk = false;
	Otp otp;

	otp.setMethod(libisdsOtpMethod2OtpMethod(io->method, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	otp.setOtpCode(Isds::fromCStr(io->otp_code));
	otp.setResolution(
	    IsdsInternal::libisdsOtpResolution2OtpResolution(io->resolution, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return otp;

fail:
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return Otp();
}

/*!
 * @brief Converts otp method.
 */
static
isds_otp_method otpMethod2libisdsOtpMethod(enum Isds::Type::OtpMethod om,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	isds_otp_method iom = OTP_HMAC;

	switch (om) {
	/*
	 * Isds::Type::OM_UNKNOWN same as default.
	 */
	case Isds::Type::OM_HMAC: iom = OTP_HMAC; break;
	case Isds::Type::OM_TIME: iom = OTP_TIME; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return iom;
}

/*!
 * @brief Set libisds otp structure according to the otp.
 */
static
bool setLibisdsOtpContent(struct isds_otp *tgt, const Isds::Otp &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	bool iOk = false;

	tgt->method = otpMethod2libisdsOtpMethod(src.method(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->otp_code, src.otpCode()))) {
		return false;
	}
	tgt->resolution = IsdsInternal::otpResolution2libisdsOtpResolution(
	    src.resolution(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}

	return true;
}

struct isds_otp *Isds::otp2libisds(const Otp &o, bool *ok)
{
	if (Q_UNLIKELY(o.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_otp *io = (struct isds_otp *)std::malloc(sizeof(*io));
	if (Q_UNLIKELY(io == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(io, 0, sizeof(*io));

	if (Q_UNLIKELY(!setLibisdsOtpContent(io, o))) {
		otp_free(&io);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return io;
}

void Isds::otp_free(struct isds_otp **io)
{
	if ((io == NULL) || (*io == NULL)) {
		return;
	}

	std::free((*io)->otp_code);
	std::free(*io); *io = NULL;
}
