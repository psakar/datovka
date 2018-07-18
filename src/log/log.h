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

#ifndef _LOG_H_
#define _LOG_H_

#include <cstdarg>
#include <QString>

#include "src/global.h" /* GlobInstcs::logPtr */
#include "src/log/log_common.h"
#include "src/log/log_device.h"

/*!
 * @brief Message output function.
 *
 * @param[in] type Type of message sent to message handler.
 * @param[in] context Additional information about a log message.
 * @param[in] msg Message to be handled.
 */
void globalLogOutput(enum QtMsgType type, const QMessageLogContext &context,
    const QString &msg);

/*!
 * @brief Debugging using Qt-defined output.
 *
 * @param[in] fmt Format string.
 */
void qDebugCall(const char *fmt, ...);

/*!
 * @brief Debugging using Qt-defined output.
 *
 * @param[in] fmt Format string.
 * @param[in,out] ap Variable arguments list.
 */
void qDebugCallV(const char *fmt, va_list ap);

/*!
 * @brief Generates location debug information.
 */
#ifdef DEBUG
#  define debugSlotCall() \
	do { \
		/* qDebugCall("<SLOT> %s() '%s'", __func__, __FILE__); */ \
		logDebugLv1NL("<SLOT> %s() '%s'", __func__, __FILE__); \
	} while(0)
#else
   /* Forces the semicolon after the macro. */
#  define debugSlotCall() do {} while(0)
#endif

/*!
 * @brief Generates location debug information.
 */
#ifdef DEBUG
#  define debugFuncCall() \
	do { \
		/* qDebugCall("<FUNC> %s() '%s'", __func__, __FILE__); */ \
		logDebugLv2NL("<FUNC> %s() '%s'", __func__, __FILE__); \
	} while(0)
#else
   /* Forces the semicolon after the macro. */
#  define debugFuncCall() do {} while(0)
#endif

/*!
 * @brief Logging macro used for internal purposes.
 *
 * @param[in] logVerbThresh Logging verbosity threshold.
 * @param[in] logSrc        Logging source.
 * @param[in] logUrg        Logging urgency.
 * @param[in] format        Format of the messages, follows printf syntax.
 * @param[in] ...           Variadic arguments.
 */
#define _internalLogNL(logVerbThresh, logSrc, logUrg, format, ...) \
	if (GlobInstcs::logPtr->logVerbosity() > (logVerbThresh)) { \
		GlobInstcs::logPtr->log((logSrc), (logUrg), \
		    format " (%s:%d, %s())\n", \
		    __VA_ARGS__, __FILE__, __LINE__, __func__); \
	} else { \
		GlobInstcs::logPtr->log((logSrc), (logUrg), format "\n", \
		    __VA_ARGS__); \
	}

/*!
 * @brief A macro which logs a function name followed with given debugging
 * information. The data are logged only when the global verbosity variable
 * exceeds the given threshold. Automatic newline is added.
 *
 * @param[in] verbThresh Verbosity threshold.
 * @param[in] format     Format of the messages, follows printf syntax.
 * @param[in] ...        Variadic arguments.
 *
 * @note This macro works only if the DEBUG macro is defined. If no DEBUG macro
 * is defined then it won't generate any code.
 */
#if DEBUG
#define logDebugNL(verbThresh, format, ...) \
	do { \
		if (GlobInstcs::logPtr->debugVerbosity() > (verbThresh)) { \
			_internalLogNL(0, LOGSRC_DFLT, LOG_DEBUG, \
			    format, __VA_ARGS__); \
		} \
	} while (0)
#else /* !DEBUG */
#define logDebugNL(verbThresh, format, ...) \
	(void) 0
#endif /* DEBUG */

/*!
 * @brief Logs the debugging information even if the threshold was not set.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv0NL(format, ...) \
	logDebugNL(-1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 0.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv1NL(format, ...) \
	logDebugNL(0, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 1.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv2NL(format, ...) \
	logDebugNL(1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 2.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugLv3NL(format, ...) \
	logDebugNL(2, format, __VA_ARGS__)

/*!
 * @brief Logs multi-line debugging message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugMl(verbThresh, format, ...) \
	do { \
		if (GlobInstcs::logPtr->debugVerbosity() > verbThresh) { \
			GlobInstcs::logPtr->logMl(LOGSRC_DFLT, LOG_DEBUG, \
			    format, __VA_ARGS__); \
		} \
	} while (0)

/*!
 * @brief Logs the debugging information even if the threshold was not set.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugMlLv0(format, ...) \
	logDebugMl(-1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 0.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugMlLv1(format, ...) \
	logDebugMl(0, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 1.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugMlLv2(format, ...) \
	logDebugMl(1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 2.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logDebugMlLv3(format, ...) \
	logDebugMl(2, format, __VA_ARGS__)

/*!
 * @brief Logs information message.
 *
 * @param[in] format Format of the message.
 * @param[in] ...    Varidic arguments.
 */
#define logInfo(format, ...) \
	do { \
		GlobInstcs::logPtr->log(LOGSRC_DFLT, LOG_INFO, format, \
		    __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs info message. Automatic newline is added.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logInfoNL(format, ...) \
	_internalLogNL(0, LOGSRC_DFLT, LOG_INFO, format, __VA_ARGS__)

/*!
 * @brief Logs multi-line information message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logInfoMl(format, ...) \
	do { \
		GlobInstcs::logPtr->logMl(LOGSRC_DFLT, LOG_INFO, format, \
		    __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs warning message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logWarning(format, ...) \
	do { \
		GlobInstcs::logPtr->log(LOGSRC_DFLT, LOG_WARNING, format, \
		    __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs warning message. Automatic newline is added.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logWarningNL(format, ...) \
	_internalLogNL(0, LOGSRC_DFLT, LOG_WARNING, format, __VA_ARGS__)

/*!
 * @brief Logs multi-line warning message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logWarningMl(format, ...) \
	do { \
		GlobInstcs::logPtr->logMl(LOGSRC_DFLT, LOG_WARNING, format, \
		    __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logError(format, ...) \
	do { \
		GlobInstcs::logPtr->log(LOGSRC_DFLT, LOG_ERR, format, \
		    __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs error message. Automatic newline is added.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logErrorNL(format, ...) \
	_internalLogNL(0, LOGSRC_DFLT, LOG_ERR, format, __VA_ARGS__)

/*!
 * @brief Logs multi-line error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ...    Variadic arguments.
 */
#define logErrorMl(format, ...) \
	do { \
		GlobInstcs::logPtr->logMl(LOGSRC_DFLT, LOG_ERR, format, \
		    __VA_ARGS__); \
	} while (0)

#endif /* _LOG_H_ */
