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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QString>

#include "src/isds/types.h"

namespace Isds {

	/*!
	 * @brief Contains description of used types.
	 */
	class Description {
		Q_DECLARE_TR_FUNCTIONS(Description)

	private:
		/*!
		 * @brief Private constructor.
		 */
		Description(void);

	public:
		/*!
		 * @brief Returns localised data box type description.
		 *
		 * @param[in] type Box type.
		 * @return Localised box type description.
		 */
		static
		QString descrDbType(enum Type::DbType type);

		/*!
		 * @brief Returns localised data box state description text.
		 *
		 * @param[in] state Box state.
		 * @return Localised box state description.
		 */
		static
		QString descrDbState(enum Type::DbState state);

		/*!
		 * @brief Returns localised message status description text.
		 *
		 * @param[in] state Message status value.
		 * @return Localised message status description.
		 */
		static
		QString descrDmState(enum Type::DmState state);

		/*!
		 * @brief Returns localised message type description.
		 *
		 * @param[in] typeStr String containing single character
		 *                    describing the message type.
		 * @return Localised message type description.
		 */
		static
		QString descrDmTypeChar(const QString &typeStr);

		/*!
		 * @brief Returns localised error description text.
		 *
		 * @param[in] err Error code.
		 * @return Localised error description.
		 */
		static
		QString descrError(enum Type::Error err);
	};

}
