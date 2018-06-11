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

#include "src/datovka_shared/isds/box_interface.h"
#include "src/isds/to_text_conversion.h"

/*!
 * @brief Checks whether string contains only white-space characters.
 *
 * @param[in] str String to be checked.
 * @return True if string is empty or contains only white-space characters.
 */
static inline
bool isBlankString(const QString &str)
{
	return str.isEmpty() || str.trimmed().isEmpty();
}

QString Isds::textAddressWithoutIc(const Address &address)
{
	QString addrStr;
	if (!address.isNull()) {
		const QString &street(address.street());
		const QString &adNumberInStreet(address.numberInStreet());
		const QString &adNumberInMunicipality(
		    address.numberInMunicipality());

		if (isBlankString(street)) {
			addrStr = address.city();
		} else {
			addrStr = street;
			if (isBlankString(adNumberInStreet)) {
				addrStr += " " + adNumberInMunicipality +
				    ", " + address.city();
			} else if (isBlankString(adNumberInMunicipality)) {
				addrStr += " " + adNumberInStreet +
				    ", " + address.city();
			} else {
				addrStr += " " + adNumberInMunicipality +
				    "/" + adNumberInStreet +
				    ", " + address.city();
			}
		}
	}
	return addrStr;
}

QString Isds::textOwnerName(const DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return QString();
	}

	QString name;
	{
		const PersonName &personName(dbOwnerInfo.personName());
		if (!personName.isNull()) {
			switch (dbOwnerInfo.dbType()) {
			case Isds::Type::BT_FO:
				name = personName.firstName() +
				    " " + personName.lastName();
				break;
			case Isds::Type::BT_PFO:
			case Isds::Type::BT_PFO_ADVOK:
			case Isds::Type::BT_PFO_DANPOR:
			case Isds::Type::BT_PFO_INSSPR:
			case Isds::Type::BT_PFO_AUDITOR:
				{
					const QString &firmName(dbOwnerInfo.firmName());
					if (isBlankString(firmName)) {
						name = personName.firstName() +
						    " " + personName.lastName();
					} else {
						name = firmName;
					}
				}
				break;
			default:
				break;
			}
		}
	}
	if (name.isEmpty()) { /* Defaults. */
		name = dbOwnerInfo.firmName();
	}
	return name;
}
