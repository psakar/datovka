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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QList>
#include <QString>

#include "src/datovka_shared/gov_services/service/gov_service_form_field.h"
#include "src/datovka_shared/isds/box_interface.h"
#include "src/datovka_shared/isds/message_interface.h"
#include "src/datovka_shared/isds/types.h"

/* Správa základních registrů */
#define GOV_SZR_FULL_NAME "Automat ZR (Správa základních registrů)"
#define GOV_SZR_DB_ID "4h8cxph"
#define GOV_SZR_XML_FILE_NAME "zrds.xml"
/* Ministerstvo vnitra */
#define GOV_MV_FULL_NAME "Automat CzP (Ministerstvo vnitra)"
#define GOV_MV_DB_ID "xhzkdqv"
#define GOV_MV_XML_FILE_NAME "czpathome.xml"

#define GOV_DATE_FORMAT "yyyy-MM-dd"

namespace Gov {

	/*!
	 * @brief Encapsulates the general e-gov service request.
	 */
	class Service {
		Q_DECLARE_TR_FUNCTIONS(Service)

	public:
		/*!
		 * @brief Constructor.
		 */
		Service(void);

		/*!
		 * @brief Destructor.
		 */
		virtual
		~Service(void);

		/*!
		 * @brief Allocate a new service object.
		 *
		 * @return Newly allocated object or a Q_NULLPTR on any error.
		 */
		virtual
		Service *createNew(void) const = 0;

		/*!
		 * @brief Return application-internal gov service identifier.
		 *
		 * @return Internal service identifier.
		 */
		virtual
		const QString &internalId(void) const = 0;

		/*!
		 * @brief Return gov service full name.
		 *
		 * @return Full name of service.
		 */
		virtual
		const QString &fullName(void) const = 0;

		/*!
		 * @brief Return full name of the gov institute providing
		 *     the service.
		 *
		 * @return Full name of gov institute.
		 */
		virtual
		const QString &instituteName(void) const = 0;

		/*!
		 * @brief Return data-box ID of the target data box
		 *     of the gov institute.
		 *
		 * @return Data-box ID.
		 */
		virtual
		const QString &boxId(void) const = 0;

		/*!
		 * @brief Return gov service data message subject.
		 *
		 * @return Message annotation.
		 */
		virtual
		const QString &dmAnnotation(void) const = 0;

		/*!
		 * @brief Return gov service sender identification.
		 *
		 * @return Message sender identification.
		 */
		virtual
		const QString &dmSenderIdent(void) const = 0;

		/*!
		 * @brief Return gov service attachment file name.
		 *
		 * @return Message attachment file name.
		 */
		virtual
		const QString &dmFileDescr(void) const = 0;

		/*!
		 * @brief Check whether a box of the given type can send
		 *     this request.
		 *
		 * @return True if the box of the type can send the request.
		 */
		virtual
		bool canSend(enum Isds::Type::DbType dbType) const = 0;

		/*!
		 * @brief Return list of required fields needed for the gov
		 *     service request.
		 *
		 * @return List of required fields.
		 */
		virtual
		const QList<FormField> &fields(void) const;
		virtual
		QList<FormField> &fields(void);

		/*!
		 * @brief Get value associated with key.
		 *
		 * @param[in]  key Key identifying the data.
		 * @param[out] ok Set to false if key does not exist in the form fields.
		 * @return Value, null string if key does not exist.
		 */
		virtual
		QString fieldVal(const QString &key, bool *ok = Q_NULLPTR) const;

		/*!
		 * @brief Set service form field data.
		 *
		 * @param[in] key Key identifying the data.
		 * @param[in] val Value to be set.
		 * @return True if key found and data were set, false on any error.
		 */
		virtual
		bool setFieldVal(const QString &key, const QString &val);

		/*!
		 * @brief Set form data that can be acquired from the owner info.
		 *
		 * @param[in] dbOwnerInfo Box owner info.
		 * @return True if data were set, false on any error.
		 */
		virtual
		bool setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo) = 0;

		/*!
		 * @brief Check whether all mandatory data are set.
		 *
		 * @return True if all mandatory fields are set.
		 */
		virtual
		bool haveAllMandatoryFields(void) const;

		/*!
		 * @brief Return true if service requires mandatory data that
		 *     must be provided by the user.
		 *
		 * @return True if at least one such field exists.
		 */
		virtual
		bool containsMandatoryUserFields(void) const;

		/*!
		 * @brief Return true if service requires data that must be
		 *     acquired from the data box.
		 *
		 * @return true if at least on such field is in the model.
		 */
		virtual
		bool containsBoxOwnerDataFields(void) const;

		/*!
		 * @brief Check whether fields contain valid data.
		 *
		 * @param[out] errDescr Non-empty string with error description,
		 *                      empty string if no error found.
		 * @return True on success.
		 */
		virtual
		bool haveAllValidFields(QString *errDescr = Q_NULLPTR) = 0;

		/*!
		 * @brief Create ISDS message containing the request.
		 *
		 * @return Message structure, null structure on any error.
		 */
		virtual
		Isds::Message dataMessage(void) const;

		/*!
		 * @brief Create and fill XML attachment content.
		 *
		 * @return XML string for attachment.
		 */
		virtual
		QByteArray binaryXmlContent(void) const = 0;

	protected:
		/*!
		 * @brief Get index of the entry with given key.
		 *
		 * @param[in] key Key identifying the data.
		 * Return non-negative index, if key found, -1 else.
		 */
		int fieldIndex(const QString &key) const;

		/*!
		 * @brief Removes white spaces. Checks whether contains a valid
		 *     date in GOV_DATE_FORMAT.
		 *
		 * @param[in] key Key identifying the data.
		 * @param[out] errDescr Error description if value invalid.
		 * @return True on success.
		 */
		bool checkDate(const QString &key,
		    QString *errDescr = Q_NULLPTR);

		/*!
		 * @brief removes white spaces. Checks whether sting contains
		 *     a valid IC.
		 *
		 * @param[in] key Key identifying the data.
		 * @param[out] errDescr Error description if value invalid.
		 * @return True on success.
		 */
		bool checkIc(const QString &key, QString *errDescr = Q_NULLPTR);

		/*!
		 * @brief removes all white spaces. Checks whether remaining
		 *     sting is non-empty.
		 *
		 * @param[in] key Key identifying the data.
		 * @param[out] errDescr Error description if value invalid.
		 * @return True on success.
		 */
		bool checkStrRemoveWhiteSpace(const QString &key,
		    QString *errDescr = Q_NULLPTR);

		/*!
		 * @brief removes leading and trailing white spaces. Checks
		 *     whether remaining sting is non-empty.
		 *
		 * @param[in] key Key identifying the data.
		 * @param[out] errDescr Error description if value invalid.
		 * @return True on success.
		 */
		bool checkStrTrimmed(const QString &key,
		    QString *errDescr = Q_NULLPTR);

		QList<FormField> m_formFields; /*!< Data within the form. */
	};

}
