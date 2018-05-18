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

#include <QCoreApplication> // Q_DECLARE_TR_FUNCTIONS
#include <QString>

/*!
 * @brief Provides conversion functions for ISDS types.
 */
class IsdsConversion {
	Q_DECLARE_TR_FUNCTIONS(IsdsConversion)

private:
	/*!
	 * @brief Private constructor.
	 */
	IsdsConversion(void);

public:
	/*!
	 * @brief Convert data box type to string.
	 *
	 * @param[in] val Data box type value as used by libisds.
	 * @return Data box type description.
	 */
	static
	const QString &boxTypeToStr(int val);

	/*!
	 * @brief Convert data box type string to int as used by libisds.
	 *
	 * @param[in] val Data box type string.
	 * @return Data box type integer value.
	 */
	static
	int boxTypeStrToInt(const QString &val);

	/*!
	 * @brief Return localised sender data box type description string.
	 *
	 * @param[in] val Sender data box type value as used by libisds.
	 * @return Sender data box type description.
	 */
	static
	QString senderBoxTypeToText(int val);

	/*!
	 * @brief Translates message type to localised text.
	 *
	 * @param[in] val Message type value as used by libisds.
	 * @return Message type description.
	 */
	static
	QString dmTypeToText(const QString &val);

	/*!
	 * @brief Converts libisds message state to database value as used by
	 *     Datovka.
	 *
	 * @param[in] val Message status value as used by libisds.
	 * @return Datovka status value as stored in database.
	 */
	static
	int msgStatusIsdsToDbRepr(int val);

	/*!
	 * @brief Convert sender type to string identifier.
	 *
	 * @param[in] val Sender type value as used by libisds.
	 * @return Sender type string identifier.
	 */
	static
	const QString &senderTypeToStr(int val);

	/*!
	 * @brief Translates sender type string to localised text.
	 *
	 * @param[in] val Sender type string.
	 * @return Sender type description.
	 */
	static
	QString senderTypeStrToText(const QString &val);

	/*!
	 * @brief Return privileges as html string from number representation.
	 *
	 * @param[in] val Privilege value flags.
	 * @return Privilege description.
	 */
	static
	QString userPrivilsToText(int val);
};
