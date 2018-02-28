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
 * @brief The class holds pointers to all globally accessible structures.
 */
class GlobInstcs {

public:
	static
	class GlobPreferences *prefsPtr; /*!< Preferences. */
	static
	class ProxiesSettings *proxSetPtr; /*!< Proxy settings. */
	static
	class PinSettings *pinSetPtr; /*!< PIN settings. */
	static
	class RecordsManagementSettings *recMgmtSetPtr; /*!< Records management settings. */

	static
	class IsdsSessions *isdsSessionsPtr; /*!< ISDS session container. */

	static
	class AccountDb *accntDbPtr; /*!< Account database. */
	static
	class DbContainer *msgDbsPtr; /*!< Message database container. */
	static
	class TagDb *tagDbPtr; /*!< Tag database. */
	static
	class RecordsManagementDb *recMgmtDbPtr; /*!< Records management database. */

	/*!
	 * @brief Holds account data related to account.
	 *
	 * @note Key is userName. The user name is held by the user name list.
	 */
	static
	class AccountsMap *acntMapPtr;

private:
	/*!
	 * @brief Private constructor.
	 */
	GlobInstcs(void);
};
