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
#include <cstring> // memcpy
#include <isds.h>

#include "src/datovka_shared/isds/account_interface.h"
#include "src/datovka_shared/isds/error.h"
#include "src/datovka_shared/isds/internal_conversion.h"
#include "src/isds/account_conversion.h"
#include "src/isds/error_conversion.h"
#include "src/isds/internal_type_conversion.h"
#include "src/isds/services.h"
#include "src/isds/services_internal.h"

Isds::Error Isds::Service::changeISDSPassword(struct isds_ctx *ctx,
    const QString &oldPwd, const QString &newPwd, Otp &otp, QString &refNum)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	bool ok = false;
	isds_error ret = IE_SUCCESS;
	struct isds_otp *iOtp = otp2libisds(otp, &ok);
	char *iRefNum = NULL;
	if (!ok) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
		goto fail;
	}

	ret = isds_change_password(ctx, oldPwd.toUtf8().constData(),
	    newPwd.toUtf8().constData(), iOtp, &iRefNum);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		goto fail;
	}

	if (iOtp != NULL) {
		otp.setResolution(
		    IsdsInternal::libisdsOtpResolution2OtpResolution(
		        iOtp->resolution, &ok));
		if (Q_UNLIKELY(!ok)) {
			/* Conversion should not fail if operation succeeded. */
			Q_ASSERT(0);
		}
	}

	refNum = (iRefNum != NULL) ? QString(iRefNum) : QString();

	err.setCode(Type::ERR_SUCCESS);

fail:
	if (iOtp != NULL) {
		otp_free(&iOtp);
	}
	if (iRefNum != NULL) {
		std::free(iRefNum);
	}

	return err;
}

Isds::Error Isds::Service::getPasswordInfo(struct isds_ctx *ctx,
    QDateTime &pswExpDate)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct timeval *iPswExpDate = NULL;

	isds_error ret = isds_get_password_expiration(ctx, &iPswExpDate);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(IsdsInternal::isdsLongMessage(ctx));
		goto fail;
	}

	pswExpDate = (iPswExpDate != NULL) ?
	    dateTimeFromStructTimeval(iPswExpDate) : QDateTime();

	err.setCode(Type::ERR_SUCCESS);

fail:
	if (iPswExpDate != NULL) {
		std::free(iPswExpDate); iPswExpDate = NULL;
	}

	return err;
}
