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

#include <QObject>

/* wsdl2h -s -o dbTypes.h dbTypes.xsd  */

/*!
 * @brief Provides enumeration types based on dbTypes.xsd .
 */
class IsdsType : public QObject {
	Q_OBJECT

private:
	/*!
	 * @brief Private constructor.
	 */
	IsdsType(QObject *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Data box type.
	 *
	 * @note Defined in dbTypes.xsd. Described in
	 *     pril_3/WS_ISDS_Sprava_datovych_schranek.pdf
	 *     (section 2.1 CreateDataBox).
	 */
	enum DbType {
		BT_SYSTEM = 0, /*!<
		                * This value is not listed in dbTypes.xsd but is mentioned in
		                * pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
		                * (section 2.4.1 MessageDownload).
		                */
		BT_OVM = 10, /*!< Public authority. */
		BT_OVM_NOTAR = 11, /* This type has been replaced with OVM_PFO. */
		BT_OVM_EXEKUT = 12, /* This type has been replaced with OVM_PFO. */
		BT_OVM_REQ = 13,
		BT_OVM_FO = 14,
		BT_OVM_PFO = 15,
		BT_OVM_PO = 16,
		BT_PO = 20,
		BT_PO_ZAK = 21, /* This type has been replaced with PO. */
		BT_PO_REQ = 22,
		BT_PFO = 30,
		BT_PFO_ADVOK = 31,
		BT_DBTYPE_PFO_DANPOR = 32,
		BT_DBTYPE_PFO_INSSPR = 33,
		BT_DBTYPE_PFO_AUDITOR = 34,
		BT_FO = 40
	};

	/*!
	 * @brief Data box accessibility state.
	 *
	 * @note Described in pril_3/WS_ISDS_Sprava_datovych_schranek.pdf
	 *     (appendix 4).
	 */
	enum DbState {
		BS_ERROR = 0, /* Error value, see documentation. */
		BS_ACCESSIBLE = 1,
		BS_TEMP_INACCESSIBLE = 2,
		BS_NOT_YET_ACCESSIBLE = 3,
		BS_PERM_INACCESSIBLE = 4,
		BS_REMOVED = 5,
		BS_TEMP_UNACCESSIBLE_LAW = 6
	};

	/*!
	 * @brief User permissions.
	 *
	 * @note Described in
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf (section 1.4),
	 *     pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 2.4 AddDataBoxUser)
	 */
	enum Privilege {
		PRIVIL_NONE = 0x00, /* Added for convenience. */
		PRIVIL_READ_NON_PERSONAL = 0x01,
		PRIVIL_READ_ALL = 0x02,
		PRIVIL_CREATE_DM = 0x04,
		PRIVIL_VIEW_INFO = 0x08,
		PRIVIL_SEARCH_DB = 0x10,
		PRIVIL_OWNER_ADM = 0x20,
		PRIVIL_READ_VAULT = 0x40, /* zrušeno od července 2012 */
		PRIVIL_ERASE_VAULT = 0x80
	};
	Q_DECLARE_FLAGS(Privileges, Privilege)
	Q_FLAG(Privileges)

	/*
	 * @brief Describes the message cycle.
	 *
	 * @note Described in
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf (section 1.5).
	 */
	enum DmState {
		MS_POSTED = 1,
		MS_STAMPED = 2,
		MS_INFECTED = 3,
		MS_DELIVERED = 4,
		MS_ACCEPTED_FICT = 5,
		MS_ACCEPTED = 6,
		MS_READ = 7,
		MS_UNDELIVERABLE = 8,
		MS_REMOVED = 9,
		MS_IN_VAULT = 10
	};
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IsdsType::Privileges)
