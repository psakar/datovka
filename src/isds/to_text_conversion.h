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

#include <QString>

namespace Isds {

	/* Forward class declaration. */
	class Address;
	class DbOwnerInfo;

	/*!
	 * @brief Build full address without IC.
	 *
	 * @param[in] address Address.
	 * @return String containing address, null string on error.
	 */
	QString textAddressWithoutIc(const Address &address);

	/*!
	 * @brief Build full owner name.
	 *
	 * @param[in] ownerInfo Owner info.
	 * @return String containing full owner name, null string on error.
	 */
	QString textOwnerName(const DbOwnerInfo &ownerInfo);

}
