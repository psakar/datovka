/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "src/log/log_c.h"


/* ========================================================================= */
/*
 * Debugging using Qt-defined output.
 */
void q_debug_call(const char *fmt, ...)
/* ========================================================================= */
{
	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);
}


/* ========================================================================= */
/*
 * Get debug verbosity.
 */
int glob_debug_verbosity(void)
/* ========================================================================= */
{
	return 0;
}


/* ========================================================================= */
/*
 * Log message.
 */
int glob_log(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
{
	(void) source;
	(void) level;

	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);

	return 0;
}


/* ========================================================================= */
/*
 * Log multi-line message.
 */
int glob_log_ml(int source, uint8_t level, const char *fmt, ...)
/* ========================================================================= */
{
	(void) source;
	(void) level;

	va_list argp;

	va_start(argp, fmt);

	vfprintf(stderr, fmt, argp);

	va_end(argp);

	return 0;
}
