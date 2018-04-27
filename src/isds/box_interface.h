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
#include <QScopedPointer>
#include <QString>

#include "src/isds/types.h"

/*
 * Structures originating from pril_3/WS_ISDS_Sprava_datovych_schranek.pdf.
 */

namespace Isds {

	class AddressPrivate;
	/*!
	 * @brief Exists as address element group gAddress (dbTypes.xsd).
	 *     // DbOwnerInfo, DbUserInfo, DbUserInfoExt
	 */
	class Address {
		Q_DECLARE_PRIVATE(Address)

	public:
		Address(void);
		Address(const Address &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Address(Address &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Address(void);

		Address &operator=(const Address &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Address &operator=(Address &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Address &other) const;
		bool operator!=(const Address &other) const;

		friend void swap(Address &first, Address &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* adCity */
		const QString &city(void) const;
		void setCity(const QString &c);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCity(QString &&c);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adDistrict - not present in libisds-0.10.7 */
		/* adStreet */
		const QString &street(void) const;
		void setStreet(const QString &s);
#ifdef Q_COMPILER_RVALUE_REFS
		void setStreet(QString &&s);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adNumberInStreet */
		const QString &numberInStreet(void) const;
		void setNumberInStreet(const QString &nis);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNumberInStreet(QString &&nis);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adNumberInMunicipality */
		const QString &numberInMunicipality(void) const;
		void setNumberInMunicipality(const QString &nim);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNumberInMunicipality(QString &&nim);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adZipCode */
		const QString &zipCode(void) const;
		void setZipCode(const QString &zc);
#ifdef Q_COMPILER_RVALUE_REFS
		void setZipCode(QString &&zc);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adState */
		const QString &state(void) const;
		void setState(const QString &s);
#ifdef Q_COMPILER_RVALUE_REFS
		void setState(QString &&s);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adAMCode - not present in libisds-0.10.7 */

		friend Address libisds2address(const struct isds_Address *ia,
		    bool *ok);

	private:
		QScopedPointer<AddressPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Address &first, Address &second) Q_DECL_NOTHROW;

	class BirthInfoPrivate;
	/*!
	 * @brief Exists as birth info element group gBirthInfo (dbTypes.xsd).
	 *     // DbOwnerInfo
	 */
	class BirthInfo {
		Q_DECLARE_PRIVATE(BirthInfo)

	public:
		BirthInfo(void);
		BirthInfo(const BirthInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		BirthInfo(BirthInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~BirthInfo(void);

		BirthInfo &operator=(const BirthInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		BirthInfo &operator=(BirthInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const BirthInfo &other) const;
		bool operator!=(const BirthInfo &other) const;

		friend void swap(BirthInfo &first, BirthInfo &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* biDate */
		const QDate &date(void) const;
		void setDate(const QDate &bd);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDate(QDate &&bd);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* biCity */
		const QString &city(void) const;
		void setCity(const QString &c);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCity(QString &&c);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* biCounty */ /* cz: okres; de: Bezirk, Kreis; en: council area, county, district */
		const QString &county(void) const;
		void setCounty(const QString &c);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCounty(QString &&c);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* biState */ /* cz: stat; de: Land, Staat; en: Country */
		const QString &state(void) const;
		void setState(const QString &s);
#ifdef Q_COMPILER_RVALUE_REFS
		void setState(QString &&s);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend BirthInfo libisds2birthInfo(
		    const struct isds_BirthInfo *ibi, bool *ok);

	private:
		QScopedPointer<BirthInfoPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(BirthInfo &first, BirthInfo &second) Q_DECL_NOTHROW;

	class PersonNamePrivate;
	/*!
	 * @brief Exists as name element group gPersonName (dbTypes.xsd).
	 *     // DbOwnerInfo, DbUserInfo
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6)
	 */
	class PersonName {
		Q_DECLARE_PRIVATE(PersonName)

	public:
		PersonName(void);
		PersonName(const PersonName &other);
#ifdef Q_COMPILER_RVALUE_REFS
		PersonName(PersonName &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~PersonName(void);

		PersonName &operator=(const PersonName &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		PersonName &operator=(PersonName &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const PersonName &other) const;
		bool operator!=(const PersonName &other) const;

		friend void swap(PersonName &first, PersonName &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* pnFirstName */
		const QString &firstName(void) const;
		void setFirstName(const QString &fn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFirstName(QString &&fn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* pnMiddleName */
		const QString &middleName(void) const;
		void setMiddleName(const QString &mn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setMiddleName(QString &&mn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* pnLastName */
		const QString &lastName(void) const;
		void setLastName(const QString &ln);
#ifdef Q_COMPILER_RVALUE_REFS
		void setLastName(QString &&ln);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* pnLastNameAtBirth */
		const QString &lastNameAtBirth(void) const;
		void setLastNameAtBirth(const QString &lnab);
#ifdef Q_COMPILER_RVALUE_REFS
		void setLastNameAtBirth(QString &&lnab);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend PersonName libisds2personName(
		    const struct isds_PersonName *ipn, bool *ok);

	private:
		QScopedPointer<PersonNamePrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(PersonName &first, PersonName &second) Q_DECL_NOTHROW;

	/*!
	 * @brief Exists as type tDbOwnerInfo (dbTypes.xsd).
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6.1)
	 */
	class DbOwnerInfo {
	public:
		DbOwnerInfo(void);
		~DbOwnerInfo(void);

		DbOwnerInfo(const DbOwnerInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		DbOwnerInfo(DbOwnerInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */

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

		DbOwnerInfo &operator=(const DbOwnerInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		DbOwnerInfo &operator=(DbOwnerInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		void *m_dataPtr;
	};

	/*!
	 * @brief Exists as type tDbUserInfo, tDbUserInfoExt (dbTypes.xsd).
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6.2)
	 */
	class DbUserInfo {
	public:
		DbUserInfo(void);
		~DbUserInfo(void);

		DbUserInfo(const DbUserInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		DbUserInfo(DbUserInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */

		/* pnFirstName, pnMiddleName, pnLastName, pnLastNameAtBirth */
		PersonName personName(void) const;
		void setPersonName(const PersonName &pn);
		/* adCity, adStreet, adNumberInStreet, adNumberInMunicipality, adZipCode, adState */
		Address address(void) const;
		void setAddress(const Address &a);
		/* biDate */
		QDate biDate(void) const;
		void setBiDate(const QDate &d);
		/* userID */
		QString userID(void) const;
		void setUserId(const QString &uid);
		/* userType */
		enum Type::UserType userType(void) const;
		void setUserType(enum Type::UserType ut);
		/* userPrivils */
		Type::Privileges userPrivils(void) const;
		void setUserPrivils(Type::Privileges p);
		/* ic */
		QString ic(void) const;
		void setIc(const QString &ic);
		/* firmName */
		QString firmName(void) const;
		void setFirmName(const QString &fn);
		/* caStreet */
		QString caStreet(void) const;
		void setCaStreet(const QString &cs);
		/* caCity */
		QString caCity(void) const;
		void setCaCity(const QString &cc);
		/* caZipCode */
		QString caZipCode(void) const;
		void setCaZipCode(const QString &cz);
		/* caState */
		QString caState(void) const;
		void setCaState(const QString &cs);
		/* AIFOTicket -- Optional, tDbUsersArray (dbTypes.xsd). */
		QString aifoTicket(void) const;
		void setAifoTicket(const QString &at);

		DbUserInfo &operator=(const DbUserInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		DbUserInfo &operator=(DbUserInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		void *m_dataPtr;
	};

}
