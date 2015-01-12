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


#ifndef _COMPAT_WIN_H_
#define _COMPAT_WIN_H_

#ifdef WIN32

#ifdef __cplusplus
#  include <ctime>
#else
#  include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

time_t timegm_win(struct tm *tm);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIN32 */

#endif /* !_COMPAT_WIN_H_ */
