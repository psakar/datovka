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

#include "src/datovka_shared/log/log.h"
#include "src/isds/session.h"

Isds::Session::Session(void)
    : m_ctx(NULL),
    m_mutex()
{
}

Isds::Session::~Session(void)
{
	if (m_ctx != NULL) {
		isds_error status = isds_logout(m_ctx);
		if (status != IE_SUCCESS) {
			logWarningNL("%s", "Error in ISDS logout procedure.");
		}

		status = isds_ctx_free(&m_ctx);
		if (status != IE_SUCCESS) {
			logWarningNL("%s", "Error freeing ISDS session.");
		}
	}
}

struct isds_ctx *Isds::Session::ctx(void)
{
	return m_ctx;
}

QMutex *Isds::Session::mutex(void)
{
	return &m_mutex;
}

bool Isds::Session::setTimeout(unsigned int timeoutMs)
{
	if (Q_UNLIKELY(m_ctx == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	isds_error status = isds_set_timeout(m_ctx, timeoutMs);
	if (Q_UNLIKELY(IE_SUCCESS != status)) {
		logErrorNL("%s", "Error setting time-out.");
		return false;
	}

	return true;
}

/*!
 * @brief Create new session.
 *
 * @param[in] connectionTimeoutMs Connection timeout in milliseconds.
 * @return Pointer to new session or NULL on failure.
 */
static
struct isds_ctx *createIsdsCtx(unsigned int connectionTimeoutMs)
{
	struct isds_ctx *iCtx = isds_ctx_create();
	if (Q_UNLIKELY(iCtx == NULL)) {
		return NULL;
	}

	isds_error status = isds_set_timeout(iCtx, connectionTimeoutMs);
	if (Q_UNLIKELY(IE_SUCCESS != status)) {
		logErrorNL("%s", "Error setting ISDS context time-out.");
		goto fail;
	}

	return iCtx;

fail:
	if (iCtx != NULL) {
		status = isds_ctx_free(&iCtx);
		if (status != IE_SUCCESS) {
			logWarningNL("%s", "Error freeing ISDS context.");
		}
	}
	return NULL;
}

Isds::Session *Isds::Session::createSession(unsigned int connectionTimeoutMs)
{
	Session *session = new (std::nothrow) Session();
	if (Q_UNLIKELY(session == Q_NULLPTR)) {
		return Q_NULLPTR;
	}

	session->m_ctx = createIsdsCtx(connectionTimeoutMs);
	if (Q_UNLIKELY(session->m_ctx == NULL)) {
		delete session; session = Q_NULLPTR;
		return Q_NULLPTR;
	}

	return session;
}
