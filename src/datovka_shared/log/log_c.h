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

#ifndef _LOG_C_H_
#define _LOG_C_H_

#include <stdint.h>

#include "src/datovka_shared/log/log_common.h"

/*!
 * @brief Generates location debug information.
 */
#ifdef DEBUG
#  define debug_func_call() \
	do { \
		/* q_debug_call("<FUNC> %s() '%s'", __func__, __FILE__); */ \
		log_debug_lv2_nl("<FUNC> %s() '%s'", __func__, __FILE__); \
	} while(0)
#else
   /* Forces the semicolon after the macro. */
#  define debug_func_call() do {} while(0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Debugging using Qt-defined output.
 *
 * @param[in] fmt Format string.
 */
void q_debug_call(const char *fmt, ...);

/*!
 * @brief Get logging verbosity.
 *
 * @return Logging verbosity.
 */
int glob_log_verbosity(void);

/*!
 * @brief Get debug verbosity.
 *
 * @return Debug verbosity.
 */
int glob_debug_verbosity(void);

/*!
 * @brief Log message.
 *
 * @param[in] source Source identifier.
 * @param[in] level Message urgency level.
 * @param[in] fmt Format of the log message -- follows printf(3) format.
 * @return -1 if error, 0 else.
 */
int glob_log(enum LogSource source, enum LogLevel level, const char *fmt, ...);

/*!
 * @brief Log multi-line message.
 *
 * Every new line is merged with the same prefix.
 *
 * @param[in] source Source identifier.
 * @param[in] level Message urgency level.
 * @param[in] fmt Format of the log message -- follows printf(3) format.
 * @return -1 if error, 0 else.
 */
int glob_log_ml(enum LogSource source, enum LogLevel level, const char *fmt,
    ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*!
 * @brief A macro which logs a function name followed with given debugging
 * information. The data are logged only when the global verbosity variable
 * exceeds the given threshold.
 *
 * @param[in] verb_thresh Verbosity threshold.
 * @param[in] format Format of the messages, follows printf syntax.
 * @param[in] ... Variadic arguments.
 *
 * @note This macro works only if the DEBUG macro is defined. If no DEBUG macro
 * is defined then it won't generate any code.
 */
#if DEBUG
#define log_debug_nl(verb_thresh, format, ...) \
	if (glob_debug_verbosity() > verb_thresh) { \
		if (glob_log_verbosity() > 0) { \
			glob_log(LOGSRC_DFLT, LOG_DEBUG, \
			    format " (%s:%d, %s())\n", \
			    __VA_ARGS__, __FILE__, __LINE__, __func__); \
		} else { \
			glob_log(LOGSRC_DFLT, LOG_DEBUG, \
			    format "\n", __VA_ARGS__); \
		} \
	}
#else /* !DEBUG */
#define log_debug_nl(verb_rhresh, format, ...) \
	(void) 0
#endif /* DEBUG */

/*!
 * @brief Logs the debugging information even if the threshold was not set.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_debug_lv0_nl(format, ...) \
	log_debug_nl(-1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 0.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_debug_lv1_nl(format, ...) \
	log_debug_nl(0, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 1.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_debug_lv2_nl(format, ...) \
	log_debug_nl(1, format, __VA_ARGS__)

/*!
 * @brief Logs the debugging information only if the verbosity exceeds 2.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_debug_lv3_nl(format, ...) \
	log_debug_nl(2, format, __VA_ARGS__)

/*!
 * @brief Logs information message.
 *
 * @param[in] format Format of the message.
 * @param[in] ...    Variadic arguments.
 */
#define log_info(format, ...) \
	do { \
		glob_log(LOGSRC_DFLT, LOG_INFO, format, __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs warning message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_warning(format, ...) \
	do { \
		glob_log(LOGSRC_DFLT, LOG_WARNING, format, __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_error(format, ...) \
	do { \
		glob_log(LOGSRC_DFLT, LOG_ERR, format, __VA_ARGS__); \
	} while (0)

/*!
 * @brief Logs multi-line error message.
 *
 * @param[in] format Format of the message, follows printf syntax.
 * @param[in] ... Variadic arguments.
 */
#define log_error_ml(format, ...) \
	do { \
		glob_log_ml(LOGSRC_DFLT, LOG_ERR, format, __VA_ARGS__); \
	} while (0)

#endif /* _LOG_C_H_ */
