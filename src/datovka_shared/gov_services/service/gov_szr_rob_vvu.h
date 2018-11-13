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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */

#include "src/datovka_shared/gov_services/service/gov_service_form_field.h"
#include "src/datovka_shared/gov_services/service/gov_service.h"

namespace Gov {

	/*!
	 * GOV SZR service
	 * Name: Žádost o výpis o využití údajů z registru obyvatel
	 * DS: 4h8cxph - Automat ZR (Správa základních registrů)
	 * XML: zrds.xml - do xml je třeba doplnit časové rozmezí od-do.
	 * Source DS type: FO
	 */
	class SrvcSzrRobVvu : public Service {
		Q_DECLARE_TR_FUNCTIONS(SrvcSzrRobVvu)

	public:
		/*!
		 * @brief Constructor.
		 */
		SrvcSzrRobVvu(void);

		virtual
		Service *createNew(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &internalId(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &fullName(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &instituteName(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &boxId(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &dmAnnotation(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &dmSenderIdent(void) const Q_DECL_OVERRIDE;

		virtual
		const QString &dmFileDescr(void) const Q_DECL_OVERRIDE;

		virtual
		bool canSend(enum Isds::Type::DbType dbType) const Q_DECL_OVERRIDE;

		virtual
		bool setFieldVal(const QString &key, const QString &val) Q_DECL_OVERRIDE;

		virtual
		bool setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo) Q_DECL_OVERRIDE;

		virtual
		bool haveAllValidFields(QString *errDescr = Q_NULLPTR) Q_DECL_OVERRIDE;

		virtual
		QByteArray binaryXmlContent(void) const Q_DECL_OVERRIDE;
	};

}
