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

#include "src/isds/box_interface.h"

namespace Isds {

	Address libisds2address(const struct isds_Address *ia,
	    bool *ok = Q_NULLPTR);
	struct isds_Address *address2libisds(const Address &a,
	    bool *ok = Q_NULLPTR);

	BirthInfo libisds2birthInfo(const struct isds_BirthInfo *ibi,
	    bool *ok = Q_NULLPTR);
	struct isds_BirthInfo *birthInfo2libisds(const BirthInfo &bi,
	    bool *ok = Q_NULLPTR);

}
