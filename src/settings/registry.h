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

#pragma once

/*!
 * @brief Encapsulates registry settings on Windows.
 *
 * @note Registry values are only read.
 */
class RegPreferences {

public:
	/*!< Registry entries. */
	enum Entry {
		ENTR_NEW_VER_NOTIF
	};

private:
	/*!
	 * @brief Private constructor.
	 */
	RegPreferences(void);

public:
	/*!
	 * @brief Check presence of system registry settings.
	 *
	 * @param[in] entry Entry to search for.
	 * @return True if the entry exists in registry.
	 */
	static
	bool haveSys(enum Entry entry);

	/*!
	 * @brief Check presence of user registry settings.
	 *
	 * @param[in] entry Entry to search for.
	 * @return True if the entry exists in registry.
	 */
	static
	bool haveUsr(enum Entry entry);

	/*!
	 * @brief System settings.
	 *
	 * @note The value must be present. Check the presence of the setting
	 * before reading it!
	 */
	static
	bool sysNewVersionNotification(void);
	/*!
	 * @brief User settings.
	 *
	 * @note The value must be present. Check the presence of the setting
	 * before reading it!
	 */
	static
	bool usrNewVersionNotification(void);
};
