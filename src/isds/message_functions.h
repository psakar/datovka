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

#include <QByteArray>
#include <QString>

#include "src/datovka_shared/isds/message_interface.h"

namespace Isds {

	/*!
	 * @brief ZFO type.
	 */
	enum LoadType {
		LT_ANY, /*!< All types. */
		LT_MESSAGE, /*!< Data message. */
		LT_DELIVERY /*!< Delivery info. */
	};

	/*!
	 * @brief Creates a message from supplied raw CMS data.
	 *
	 * @param[in] rawMsgData Raw message data.
	 * @param[in] zfoType Message or delivery info (enum Imports::Type).
	 * @return Return null message on error.
	 */
	Message messageFromData(const QByteArray &rawMsgData,
	    enum LoadType zfoType);

	/*!
	 * @brief Create a isds message from zfo file.
	 *
	 * @param[in] fName File name.
	 * @param[in] zfoType Message or delivery info
	 *                    (enum ImportZFODialog::ZFOtype).
	 * @return Return null message on error.
	 */
	Message messageFromFile(const QString &fName, enum LoadType zfoType);

}
