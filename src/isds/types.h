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

/*
 * wsdl2h -s -o dmBaseTypes.h dmBaseTypes.xsd
 * wsdl2h -s -o dbTypes.h dbTypes.xsd
 */

namespace Isds {

/*!
 * @brief Provides enumeration types based on dbTypes.xsd .
 */
class Type : public QObject {
	Q_OBJECT

private:
	/*!
	 * @brief Private constructor.
	 */
	Type(QObject *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Nullable bool.
	 *
	 * @note Should NULL value be treated as false?
	 */
	enum NilBool {
		BOOL_NULL = -1, /*!< Convenience value, converted from/to NULL. */
		BOOL_FALSE = 0,
		BOOL_TRUE = 1
	};

	/*!
	 * @brief Data box type.
	 *
	 * @note Defined in dbTypes.xsd. Described in
	 *     pril_3/WS_ISDS_Sprava_datovych_schranek.pdf
	 *     (section 2.1 CreateDataBox).
	 */
	enum DbType {
		BT_NULL = -1, /*!< Convenience value, converted from/to NULL. */
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
		BT_PFO_DANPOR = 32,
		BT_PFO_INSSPR = 33,
		BT_PFO_AUDITOR = 34,
		BT_FO = 40
	};

	/*!
	 * @brief Data box accessibility state.
	 *
	 * @note Described in pril_3/WS_ISDS_Sprava_datovych_schranek.pdf
	 *     (appendix 4).
	 */
	enum DbState {
		BS_ERROR = 0, /* Error value, see documentation. */ /* Also converted ftom/to NULL. */
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
		MS_NULL = -1, /*!< Convenience value, converted from/to NULL. */
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

	/*!
	 * @brief Message filter status as used by GetListOfSentMessages and
	 *     GetListOfReceivedMessages.
	 *
	 * @note Usage described in
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     (section 2.8.2).
	 */
	enum DmFiltState {
		MFS_POSTED = 0x02,
		MFS_STAMPED = 0x04,
		MFS_INFECTED = 0x08,
		MFS_DELIVERED = 0x10,
		MFS_ACCEPTED_FICT = 0x20,
		MFS_ACCEPTED = 0x40,
		MFS_READ = 0x80,
		MFS_UNDELIVERABLE = 0x0100,
		MFS_REMOVED = 0x0200,
		MFS_IN_VAULT = 0x0400,
		MFS_ANY = 0x07fe /* Convenience value. */
	};
	Q_DECLARE_FLAGS(DmFiltStates, DmFiltState)
	Q_FLAG(DmFiltStates)

	/*!
	 * @brief Describes message type.
	 *
	 * @note See pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf.
	 */
	enum DmType {
		MT_UNKNOWN = -1, /*!< Convenience value, converted from/to NULL. */
		MT_I = 'I', /* Initiatory. */
		MT_K = 'K', /* Commercial. */
		MT_O = 'O', /* Commercial response paid by sender of initiatory. */
		MT_V = 'V', /* Non-commercial government message. */
		MT_A = 'A', /* Subsidised initiatory commercial, can pay a response. */
		MT_B = 'B', /* Subsidised initiatory commercial, has already paid the response. */
		MT_C = 'C', /* Subsidised initiatory commercial, response offer expired. */
		MT_D = 'D', /* Externally subsidised initiatory commercial. */
		MT_E = 'E', /* Stamp-prepaid commercial. */
		MT_G = 'G', /* Sponsor-prepaid commercial. */
		MT_X = 'X', /* Initiatory commercial, response offer expired. */
		MT_Y = 'Y', /* Initiatory commercial, has already paid the response. */
		MT_Z = 'Z'
	};

	/*!
	 * @brief User types tUserType (dbTypes.xsd).
	 *
	 * @note Described in
	 *     pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 2.4).
	 */
	enum UserType {
		UT_NULL = -1, /*!< Convenience value, converted from/to NULL. */
		UT_PRIMARY,
		UT_ENTRUSTED,
		UT_ADMINISTRATOR,
		UT_OFFICIAL,
		UT_OFFICIAL_CERT,
		UT_LIQUIDATOR,
		UT_RECEIVER,
		UT_GUARDIAN
	};

	/*!
	 * @brief Sender type as mentioned in response description of
	 *     GetMessageAuthor.
	 *
	 * @note Described in pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     (section 2.9).
	 *     The values differ slightly from enum UserType.
	 */
	enum SenderType {
		ST_NULL = -1, /*!< Convenience value, converted from/to NULL. */
		ST_PRIMARY,
		ST_ENTRUSTED,
		ST_ADMINISTRATOR,
		ST_OFFICIAL,
		ST_VIRTUAL,
		ST_OFFICIAL_CERT,
		ST_LIQUIDATOR,
		ST_RECEIVER,
		ST_GUARDIAN
	};

	/*!
	 * @brief Hash algorithm type.
	 *
	 * @todo Find definition in documentation.
	 */
	enum HashAlg {
		HA_UNKNOWN = -1, /* Convenience value. */
		HA_MD5,
		HA_SHA_1,
		HA_SHA_224,
		HA_SHA_256,
		HA_SHA_384,
		HA_SHA_512
	};

	/*!
	 * @brief Message event.
	 *
	 * @note Described in pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf,
	 *     section 2.7.1.
	 */
	enum Event {
		EV_UNKNOWN = -1, /* Convenience value. */
		EV_ENTERED = 0, /* Message came into being. */
		EV_DELIVERED = 5, /* Message was delivered into recipient's box. */
		EV_ACCEPTED_LOGIN = 1, /* Before 27.10.210, message was accepted by recipient logging in. */
		EV_PRIMARY_LOGIN = 11, /* Primary/competent (Czech: opravneny) user with capability to read has logged in. */
		EV_ENTRUSTED_LOGIN = 12, /* Entrusted (Czech: povereny) user with capability to read has logged in. */
		EV_SYSCERT_LOGIN = 13, /* Application authenticated using a system certificate has logged in. */
		EV_ACCEPTED_FICTION = 2, /* Message has been accepted by fiction. */
		EV_UNDELIVERABLE = 3, /* Recipient box made inaccessible, message is undeliverable. */
		EV_ACCEPTED_BY_RECIPIENT = 4, /* Before 11.2011, message has been delivered and accepted by recipient action. */
		EV_UNDELIVERED_AV_CHECK = 8 /* Message didn't mass antivirus check, message has been rejected. */
	};

	/*!
	 * @brief Attachment type.
	 *
	 * @note Mentioned in pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     section 2.1.
	 */
	enum FileMetaType {
		FMT_UNKNOWN = -1, /* Convenience value. */
		FMT_MAIN, /* First in list of documents. */
		FMT_ENCLOSURE, /* Attachment. */
		FMT_SIGNATURE, /* Digital signature of another document. */
		FMT_META /* Special XML data for ESS (records management service). */
	};

	/*!
	 * @brief Raw type. Convenience value, taken from libisds.
	 */
	enum RawType {
		RT_UNKNOWN = -1, /* Convenience value. */
		RT_INCOMING_MESSAGE,
		RT_PLAIN_SIGNED_INCOMING_MESSAGE,
		RT_CMS_SIGNED_INCOMING_MESSAGE,
		RT_PLAIN_SIGNED_OUTGOING_MESSAGE,
		RT_CMS_SIGNED_OUTGOING_MESSAGE,
		RT_DELIVERYINFO,
		RT_PLAIN_SIGNED_DELIVERYINFO,
		RT_CMS_SIGNED_DELIVERYINFO
	};

	/*!
	 * @brief Full-text search type.
	 *
	 * @note Mentioned in pril_2/WS_ISDS_Vyhledavani_datovych_schranek.pdf
	 *     secrion 2.2.
	 */
	enum FulltextSearchType {
		FST_GENERAL, /*!< Search in all fields. */
		FST_ADDRESS, /*!< Search in address. */
		FST_IC, /*!< Search in organisation identifier. */
		FST_BOX_ID /*!< Search in data-box identifier. */
	};

	/*!
	 * @brief Error value. Taken from libisds for compatibility.
	 */
	enum Error {
		ERR_SUCCESS = 0, /* No error. */
		ERR_ERROR, /* Unspecified error. */
		ERR_NOTSUP,
		ERR_INVAL,
		ERR_INVALID_CONTEXT,
		ERR_NOT_LOGGED_IN,
		ERR_CONNECTION_CLOSED,
		ERR_TIMED_OUT,
		ERR_NOEXIST,
		ERR_NOMEM,
		ERR_NETWORK,
		ERR_HTTP,
		ERR_SOAP,
		ERR_XML,
		ERR_ISDS,
		ERR_ENUM,
		ERR_DATE,
		ERR_2BIG,
		ERR_2SMALL,
		ERR_NOTUNIQ,
		ERR_NOTEQUAL,
		ERR_PARTIAL_SUCCESS,
		ERR_ABORTED,
		ERR_SECURITY
	};
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Type::Privileges)
Q_DECLARE_OPERATORS_FOR_FLAGS(Type::DmFiltStates)

}
