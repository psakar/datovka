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

#include <cstdlib> /* malloc(3) */
#include <cstring> /* memset(3) */
#include <isds.h>
#include <QFile>

#include "src/common.h"
#include "src/datovka_shared/isds/error.h"
#include "src/global.h"
#include "src/io/imports.h"
#include "src/io/isds_sessions.h"
#include "src/isds/services.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/settings/preferences.h"

/*!
 * @brief Returns logging facility name.
 *
 * @param[in] facility Logging facility identifier.
 * @return String with facility name.
 */
static
const char *logFacilityName(isds_log_facility facility)
{
	const char *str = "libisds unknown";

	switch (facility) {
	case ILF_NONE:
		str = "libisds none";
		break;
	case ILF_HTTP:
		str = "libisds http";
		break;
	case ILF_SOAP:
		str = "libisds soap";
		break;
	case ILF_ISDS:
		str = "libisds isds";
		break;
	case ILF_FILE:
		str = "libisds file";
		break;
	case ILF_SEC:
		str = "libisds sec";
		break;
	case ILF_XML:
		str = "libisds xml";
		break;
	default:
		break;
	}

	return str;
}

/*!
 * @brief Logging callback used in libisds.
 *
 * @param[in] facility Logging facility identifier.
 * @param[in] level Urgency level.
 * @param[in] message Null-terminated string.
 * @param[in] length Passed string length without terminating null character.
 * @param[in] data Pointer to data passed on every invocation of the callback.
 */
static
void logCallback(isds_log_facility facility, isds_log_level level,
    const char *message, int length, void *data)
{
	Q_UNUSED(data);
	Q_UNUSED(length);
	const char *logFac = logFacilityName(facility);

	switch (level) {
	case ILL_NONE:
	case ILL_CRIT:
	case ILL_ERR:
		logErrorMl("%s: %s", logFac, message);
		break;
	case ILL_WARNING:
		logWarningMl("%s: %s", logFac, message);
		break;
	case ILL_INFO:
		logInfoMl("%s: %s", logFac, message);
		break;
	case ILL_DEBUG:
		logDebugMlLv3("%s: %s", logFac, message);
		break;
	default:
		logError("Unknown ISDS log level %d.\n", level);
		break;
	}
}

IsdsSessions::IsdsSessions(void)
    : m_sessions()
{
	isds_error status;

	/* Initialise libisds. */
	status = isds_init();
	if (IE_SUCCESS != status) {
		Q_ASSERT(0);
		logError("%s\n", "Unsuccessful ISDS initialisation.");
		/* TODO -- What to do on failure? */
	}

	isds_set_log_callback(logCallback, NULL);

	/* Logging. */
#if !defined(Q_OS_WIN)
	isds_set_logging(ILF_ALL, ILL_ALL);
#else /* defined(Q_OS_WIN) */
	/*
	 * There is a issue related to logging when using libisds compiled
	 * with MinGW. See https://gitlab.labs.nic.cz/datovka/datovka/issues/233
	 * for more details.
	 */
	if (GlobInstcs::logPtr->logVerbosity() < 3) {
		/* Don't write transferred data into log. */
		isds_set_logging(ILF_ALL, ILL_INFO);
	} else {
		logInfoNL("%s",
		    "Allowing unrestricted logging from inside libisds.");
		isds_set_logging(ILF_ALL, ILL_ALL);
	}
#endif /* !defined(Q_OS_WIN) */
}

IsdsSessions::~IsdsSessions(void)
{
	isds_error status;
	struct isds_ctx *isdsSession = NULL;

	/* Free all contexts. */
	foreach (isdsSession, m_sessions) {
		Q_ASSERT(NULL != isdsSession);

		status = isds_logout(isdsSession);
		if (IE_SUCCESS != status) {
			logWarningNL("%s", "Error in ISDS logout procedure.");
		}

		status = isds_ctx_free(&isdsSession);
		if (IE_SUCCESS != status) {
			logWarningNL("%s", "Error freeing ISDS session.");
		}
	}

	status = isds_cleanup();
	if (IE_SUCCESS != status) {
		logWarningNL("%s", "Unsuccessful ISDS clean-up.");
	}
}

bool IsdsSessions::holdsSession(const QString &userName) const
{
	return NULL != m_sessions.value(userName, NULL);
}

struct isds_ctx *IsdsSessions::session(const QString &userName) const
{
	return m_sessions.value(userName, NULL);
}

bool IsdsSessions::isConnectedToIsds(const QString &userName)
{
	if (!holdsSession(userName)) {
		return false;
	}

	Isds::Error pingErr;

	setSessionTimeout(userName, ISDS_PING_TIMEOUT_MS);
	pingErr = Isds::Service::dummyOperation(session(userName));
	setSessionTimeout(userName,
	    GlobInstcs::prefsPtr->isdsDownloadTimeoutMs);

	return Isds::Type::ERR_SUCCESS == pingErr.code();
}

struct isds_ctx *IsdsSessions::createCleanSession(const QString &userName,
    unsigned int connectionTimeoutMs)
{
	isds_error status;
	struct isds_ctx *isds_session = NULL;

	/* User name should not exist. */
	Q_ASSERT(!holdsSession(userName));

	isds_session = isds_ctx_create();
	if (Q_UNLIKELY(NULL == isds_session)) {
		logErrorNL("Error creating ISDS session for user '%s'.",
		    userName.toUtf8().constData());
		goto fail;
	}

	status = isds_set_timeout(isds_session, connectionTimeoutMs);
	if (Q_UNLIKELY(IE_SUCCESS != status)) {
		logErrorNL("Error setting time-out for user '%s'.",
		    userName.toUtf8().constData());
		goto fail;
	}

	m_sessions.insert(userName, isds_session);
	return isds_session;

fail:
	if (NULL != isds_session) {
		status = isds_ctx_free(&isds_session);
		if (IE_SUCCESS != status) {
			logWarningNL(
			    "Error freeing ISDS session for user '%s'.",
			    userName.toUtf8().constData());
		}
	}
	return NULL;
}

bool IsdsSessions::setSessionTimeout(const QString &userName,
    unsigned int timeoutMs)
{
	struct isds_ctx *isds_session = m_sessions.value(userName, NULL);
	if (Q_UNLIKELY(NULL == isds_session)) {
		Q_ASSERT(0);
		return false;
	}

	isds_error status = isds_set_timeout(isds_session, timeoutMs);
	if (Q_UNLIKELY(IE_SUCCESS != status)) {
		logErrorNL("Error setting time-out for user '%s'.",
		    userName.toUtf8().constData());
		return false;
	}

	return true;
}
