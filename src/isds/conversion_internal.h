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

/*
 * This header file must not be included in other header files.
 *
 * It provides functions needed for internal conversion.
 */

#pragma once

namespace IsdsInternal {

	/*!
	 * @brief Performs a crude function pointer comparison
	 *    (i.e. both pointers must be null or non-null).
	 *
	 * @note We cannot compare function pointers directly causes problems
	 *     especially on Windows when the pointers are pointing to different
	 *     instances of the same function (e.g. into a DLL).
	 *
	 * @param[in] first First function pointer.
	 * @param[in] second Second function pointer.
	 * @return True if condition is met, false else.
	 */
	template <class T>
	static inline
	bool crudeEqual(T first, T second)
	{
		return ((first != NULL) && (second != NULL)) ||
		    ((first == NULL) && (second == NULL));
	}

}
