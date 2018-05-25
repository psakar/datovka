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

#include "src/isds/error_conversion.h"
#include "src/isds/error.h"
#include "src/isds/box_conversion.h"
#include "src/isds/box_interface.h"
#include "src/isds/services.h"

/*!
 * @brief Wraps the isds_long_message().
 *
 * @param[in] ctx LIbisds context.
 */
static inline
QString isdsLongMessage(const struct isds_ctx *ctx)
{
#ifdef WIN32
	/* The function returns strings in local encoding. */
	return QString::fromLocal8Bit(isds_long_message(ctx));
	/*
	 * TODO -- Is there a mechanism how to force the local encoding
	 * into libisds to be UTF-8?
	 */
#else /* !WIN32 */
	return QString::fromUtf8(isds_long_message(ctx));
#endif /* WIN32 */
}

Isds::Error Isds::Service::getOwnerInfoFromLogin(struct isds_ctx *ctx,
    DbOwnerInfo &ownerInfo)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_DbOwnerInfo *oInfo = NULL;
	bool ok = true;

	isds_error ret = isds_GetOwnerInfoFromLogin(ctx, &oInfo);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	ownerInfo = (oInfo != NULL) ?
	    libisds2dbOwnerInfo(oInfo, &ok) : DbOwnerInfo();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (oInfo != NULL) {
		isds_DbOwnerInfo_free(&oInfo);
	}

	return err;
}
