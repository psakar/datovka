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

#include <isds.h>

#include "src/isds/error_conversion.h"

enum Isds::Type::Error Isds::libisds2descr(int iErr, bool *ok)
{
	bool iOk = true;
	enum Isds::Type::Error err = Type::ERR_ERROR;

	switch (iErr) {
	case IE_SUCCESS: err = Type::ERR_SUCCESS; break;
	case IE_ERROR: err = Type::ERR_ERROR; break;
	case IE_NOTSUP: err = Type::ERR_NOTSUP; break;
	case IE_INVAL: err = Type::ERR_INVAL; break;
	case IE_INVALID_CONTEXT: err = Type::ERR_INVALID_CONTEXT; break;
	case IE_NOT_LOGGED_IN: err = Type::ERR_NOT_LOGGED_IN; break;
	case IE_CONNECTION_CLOSED: err = Type::ERR_CONNECTION_CLOSED; break;
	case IE_TIMED_OUT: err = Type::ERR_TIMED_OUT; break;
	case IE_NOEXIST: err = Type::ERR_NOEXIST; break;
	case IE_NOMEM: err = Type::ERR_NOMEM; break;
	case IE_NETWORK: err = Type::ERR_NETWORK; break;
	case IE_HTTP: err = Type::ERR_HTTP; break;
	case IE_SOAP: err = Type::ERR_SOAP; break;
	case IE_XML: err = Type::ERR_XML; break;
	case IE_ISDS: err = Type::ERR_ISDS; break;
	case IE_ENUM: err = Type::ERR_ENUM; break;
	case IE_DATE: err = Type::ERR_DATE; break;
	case IE_2BIG: err = Type::ERR_2BIG; break;
	case IE_2SMALL: err = Type::ERR_2SMALL; break;
	case IE_NOTUNIQ: err = Type::ERR_NOTUNIQ; break;
	case IE_NOTEQUAL: err = Type::ERR_NOTEQUAL; break;
	case IE_PARTIAL_SUCCESS: err = Type::ERR_PARTIAL_SUCCESS; break;
	case IE_ABORTED: err = Type::ERR_ABORTED; break;
	case IE_SECURITY: err = Type::ERR_SECURITY; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return err;
}
