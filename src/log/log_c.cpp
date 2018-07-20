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

#include <cstdarg>
#include <cstdint>

#include "src/global.h"
#include "src/log/log.h"
#include "src/log/log_c.h"

void q_debug_call(const char *fmt, ...)
{
	std::va_list argp;

	va_start(argp, fmt);

	qDebugCallV(fmt, argp);

	va_end(argp);
}

int glob_log_verbosity(void)
{
	return GlobInstcs::logPtr->logVerbosity();
}

int glob_debug_verbosity(void)
{
	return GlobInstcs::logPtr->debugVerbosity();
}

int glob_log(enum LogSource source, enum LogLevel level, const char *fmt, ...)
{
	std::va_list argp;

	va_start(argp, fmt);

	GlobInstcs::logPtr->logVlog(source, level, fmt, argp);

	va_end(argp);

	return 0;
}

int glob_log_ml(enum LogSource source, enum LogLevel level, const char *fmt,
    ...)
{
	std::va_list argp;

	va_start(argp, fmt);

	GlobInstcs::logPtr->logVlogMl(source, level, fmt, argp);

	va_end(argp);

	return 0;
}
