/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#ifndef _LOG_COMMON_H_
#define _LOG_COMMON_H_


/*!
 * @brief Identifies the default source.
 */
#define LOGSRC_DEF 0


/*!
 * @brief Identifies all sources.
 */
#define LOGSRC_ANY -1


/* Taken from syslog.h */
#define LOG_EMERG   0 /* system is unusable */
#define LOG_ALERT   1 /* action must be taken immediately */
#define LOG_CRIT    2 /* critical conditions */
#define LOG_ERR     3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE  5 /* normal but significant condition */
#define LOG_INFO    6 /* informational */
#define LOG_DEBUG   7 /* debug-level messages */

#define LOG_MASK(pri) (1 << (pri)) /* mask for one priority */
#define LOG_UPTO(pri) ((1 << ((pri)+1)) - 1) /* all priorities through pri */


#endif /* _LOG_COMMON_H_ */
