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

#pragma once

/* Cannot use namespaces here as the following is also used in C code. */

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Identifies the source of the log message.
 */
enum LogSource {
	LOGSRC_ANY = -1, /*!< Identifies all log sources. */
	LOGSRC_DFLT = 0 /*!< Default log message source. */
};

/*
 * @brief Log level.
 *
 * Inspired by syslog.h
 */
enum LogLevel {
	LOG_EMERG = 0, /*!< System is unusable. */
	LOG_ALERT = 1, /*!< Action must be taken immediately. */
	LOG_CRIT = 2, /*!< Critical conditions. */
	LOG_ERR = 3, /*!< Error conditions. */
	LOG_WARNING = 4, /*!< Warning conditions. */
	LOG_NOTICE = 5, /*!< Normal but significant condition. */
	LOG_INFO = 6, /*!< Informational. */
	LOG_DEBUG = 7 /*!< Debug-level messages. */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#define LOG_MASK(lvl) (1 << (lvl)) /* Mask for one level. */
#define LOG_UPTO(lvl) ((1 << ((lvl) + 1)) - 1) /* All levels through lvl. */
