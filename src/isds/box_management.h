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

#include <QDate>
#include <QString>

#include "src/isds/types.h"

/* Structures originating from pril_3/WS_ISDS_Sprava_datovych_schranek.pdf. */

namespace Isds {

	/*!
	 * @brief Exists as address element group gAddress (dbTypes.xsd).
	 *     // DbOwnerInfo, DbUserInfo
	 */
	class Address {
	public:
		Address(void)
		    : m_adCity(), m_adStreet(), m_adNumberInStreet(),
		    m_adNumberInMunicipality(), m_adZipCode(), m_adState()
		{ }

		Address(const Address &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Address(Address &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */

		/* adCity */
		QString city(void) const { return m_adCity; }
		void setCity(const QString &c) { m_adCity = c; }
		/* adStreet */
		QString street(void) const { return m_adStreet; }
		void setStreet(const QString &s) { m_adStreet = s; }
		/* adNumberInStreet */
		QString numberInStreet(void) const { return m_adNumberInStreet; }
		void setNumberInStreet(const QString &nis) { m_adNumberInStreet = nis; }
		/* adNumberInMunicipality */
		QString numberInMunicipality(void) const { return m_adNumberInMunicipality; }
		void setNumberInMunicipality(const QString &nim) { m_adNumberInMunicipality = nim; }
		/* adZipCode */
		QString zipCode(void) const { return m_adZipCode; }
		void setZipCode(const QString &zc) { m_adZipCode = zc; }
		/* adState */
		QString state(void) const { return m_adState; }
		void setState(const QString &s) { m_adState = s; }

		Address &operator=(const Address &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Address &operator=(Address &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QString m_adCity;
		QString m_adStreet;
		QString m_adNumberInStreet;
		QString m_adNumberInMunicipality;
		QString m_adZipCode;
		QString m_adState;
	};

	/*!
	 * @brief Exists as birth info element group gBirthInfo (dbTypes.xsd).
	 *     // DbOwnerInfo
	 */
	class BirthInfo {
	public:
		BirthInfo(void)
		    : m_biDate(), m_biCity(), m_biCounty(), m_biState()
		{ }

		BirthInfo(const BirthInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		BirthInfo(BirthInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */

		/* biDate */
		QDate date(void) const { return m_biDate; }
		void setDate(const QDate &d) { m_biDate = d; }
		/* biCity */
		QString city(void) const { return m_biCity; }
		void setCity(const QString &c) { m_biCity = c; }
		/* biCounty */ /* cz: okres; de: Bezirk, Kreis; en: council area, county, district */
		QString county(void) const { return m_biCounty; }
		void setCounty(const QString &c) { m_biCounty = c; }
		/* biState */ /* cz: stat; de: Land, Staat; en: Country */
		QString state(void) const { return m_biState; }
		void setState(const QString &s) { m_biState = s; }

		BirthInfo &operator=(const BirthInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		BirthInfo &operator=(BirthInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QDate m_biDate;
		QString m_biCity;
		QString m_biCounty;
		QString m_biState;
	};

	/*!
	 * @brief Exists as name element group gPersonName (dbTypes.xsd).
	 *     // DbOwnerInfo, DbUserInfo
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6)
	 */
	class PersonName {
	public:
		PersonName(void)
		    : m_pnFirstName(), m_pnMiddleName(), m_pnLastName(),
		    m_pnLastNameAtBirth()
		{ }

		PersonName(const PersonName &other);
#ifdef Q_COMPILER_RVALUE_REFS
		PersonName(PersonName &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */

		/* pnFirstName */
		QString firstName(void) const { return m_pnFirstName; }
		void setFirstName(const QString &fn) { m_pnFirstName = fn; }
		/* pnMiddleName */
		QString middleName(void) const { return m_pnMiddleName; }
		void setMiddleName(const QString &mn) { m_pnMiddleName = mn; }
		/* pnLastName */
		QString lastName(void) const { return m_pnLastName; }
		void setLastName(const QString &ln) { m_pnLastName = ln; }
		/* pnLastNameAtBirth */
		QString lastNameAtBirth(void) const { return m_pnLastNameAtBirth; }
		void setLastNameAtBirth(const QString &lnab) { m_pnLastNameAtBirth = lnab; }

		PersonName &operator=(const PersonName &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		PersonName &operator=(PersonName &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QString m_pnFirstName;
		QString m_pnMiddleName;
		QString m_pnLastName;
		QString m_pnLastNameAtBirth;
	};

	/*!
	 * @brief Exists as type tDbOwnerInfo (dbTypes.xsd).
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6.1)
	 */
	class DbOwnerInfo {
	public:
		DbOwnerInfo(void);
		~DbOwnerInfo(void);

		/* dbID */
		QString dbID(void) const;
		void setDbID(const QString &bi);
		/* dbType */
		enum Type::DbType dbType(void) const;
		void setDbType(enum Type::DbType bt);
		/* ic */
		QString ic(void) const;
		void setIc(const QString &ic);
		/* pnFirstName, pnMiddleName, pnLastName, pnLastNameAtBirth */
		PersonName personName(void) const;
		void setPersonName(const PersonName &pn);
		/* firmName */
		QString firmName(void) const;
		void setFirmName(const QString &fn);
		/* biDate, biCity, biCounty, biState */
		BirthInfo birthInfo(void) const;
		void setBirthInfo(const BirthInfo &bi);
		/* adCity, adStreet, adNumberInStreet, adNumberInMunicipality, adZipCode, adState */
		Address address(void) const;
		void setAddress(const Address &a);
		/* Nationality */
		QString nationality(void) const;
		void setNationality(const QString &n);
		/* email */
		QString email(void) const;
		void setEmail(const QString &e);
		/* telNumber */
		QString telNumber(void) const;
		void setTelNumber(const QString &tn);
		/* identifier */
		QString identifier(void) const;
		void setIdentifier(const QString &i);
		/* registryCode */
		QString registryCode(void) const;
		void setRegistryCode(const QString &rc);
		/* dbState */
		enum Type::DbState dbState(void) const;
		void setDbState(enum Type::DbState bs);
		/* dbEffectiveOVM */
		enum Type::NilBool dbEffectiveOVM(void) const;
		void setDbEffectiveOVM(enum Type::NilBool eo);
		/* dbOpenAddressing */
		enum Type::NilBool dbOpenAddressing(void) const;
		void setDbOpenAddressing(enum Type::NilBool oa);

	private:
		void *m_dataPtr;
	};
}
