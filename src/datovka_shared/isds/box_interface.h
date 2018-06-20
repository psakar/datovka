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
#include <QDateTime>
#include <QList>
#include <QPair>
#include <QScopedPointer>
#include <QString>

#include "src/datovka_shared/isds/types.h"

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

	private:
		QScopedPointer<PersonNamePrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(PersonName &first, PersonName &second) Q_DECL_NOTHROW;

	class DbOwnerInfoPrivate;
	/*!
	 * @brief Exists as type tDbOwnerInfo (dbTypes.xsd).
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6.1)
	 */
	class DbOwnerInfo {
		Q_DECLARE_PRIVATE(DbOwnerInfo)

	public:
		DbOwnerInfo(void);
		DbOwnerInfo(const DbOwnerInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		DbOwnerInfo(DbOwnerInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~DbOwnerInfo(void);

		DbOwnerInfo &operator=(const DbOwnerInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		DbOwnerInfo &operator=(DbOwnerInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const DbOwnerInfo &other) const;
		bool operator!=(const DbOwnerInfo &other) const;

		friend void swap(DbOwnerInfo &first, DbOwnerInfo &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* dbID */
		const QString &dbID(void) const;
		void setDbID(const QString &bi);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbID(QString &&bi);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbType */
		enum Type::DbType dbType(void) const;
		void setDbType(enum Type::DbType bt);
		/* ic */
		const QString &ic(void) const;
		void setIc(const QString &ic);
#ifdef Q_COMPILER_RVALUE_REFS
		void setIc(QString &&ic);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* pnFirstName, pnMiddleName, pnLastName, pnLastNameAtBirth */
		const PersonName &personName(void) const;
		void setPersonName(const PersonName &pn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setPersonName(PersonName &&pn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* firmName */
		const QString &firmName(void) const;
		void setFirmName(const QString &fn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFirmName(QString &&fn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* biDate, biCity, biCounty, biState */
		const BirthInfo &birthInfo(void) const;
		void setBirthInfo(const BirthInfo &bi);
#ifdef Q_COMPILER_RVALUE_REFS
		void setBirthInfo(BirthInfo &&bi);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adCity, adStreet, adNumberInStreet, adNumberInMunicipality, adZipCode, adState */
		const Address &address(void) const;
		void setAddress(const Address &a);
#ifdef Q_COMPILER_RVALUE_REFS
		void setAddress(Address &&a);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* Nationality */
		const QString &nationality(void) const;
		void setNationality(const QString &n);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNationality(QString &&n);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* email */
		const QString &email(void) const;
		void setEmail(const QString &e);
#ifdef Q_COMPILER_RVALUE_REFS
		void setEmail(QString &&e);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* telNumber */
		const QString &telNumber(void) const;
		void setTelNumber(const QString &tn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setTelNumber(QString &&tn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* identifier */
		const QString &identifier(void) const;
		void setIdentifier(const QString &i);
#ifdef Q_COMPILER_RVALUE_REFS
		void setIdentifier(QString &&i);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* registryCode */
		const QString &registryCode(void) const;
		void setRegistryCode(const QString &rc);
#ifdef Q_COMPILER_RVALUE_REFS
		void setRegistryCode(QString &&rc);
#endif /* Q_COMPILER_RVALUE_REFS */
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
		QScopedPointer<DbOwnerInfoPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(DbOwnerInfo &first, DbOwnerInfo &second) Q_DECL_NOTHROW;

	class DbUserInfoPrivate;
	/*!
	 * @brief Exists as type tDbUserInfo, tDbUserInfoExt (dbTypes.xsd).
	 *
	 * pril_3/WS_ISDS_Sprava_datovych_schranek.pdf (section 1.6.2)
	 */
	class DbUserInfo {
		Q_DECLARE_PRIVATE(DbUserInfo)

	public:
		DbUserInfo(void);
		DbUserInfo(const DbUserInfo &other);
#ifdef Q_COMPILER_RVALUE_REFS
		DbUserInfo(DbUserInfo &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~DbUserInfo(void);

		DbUserInfo &operator=(const DbUserInfo &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		DbUserInfo &operator=(DbUserInfo &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const DbUserInfo &other) const;
		bool operator!=(const DbUserInfo &other) const;

		friend void swap(DbUserInfo &first, DbUserInfo &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* pnFirstName, pnMiddleName, pnLastName, pnLastNameAtBirth */
		const PersonName &personName(void) const;
		void setPersonName(const PersonName &pn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setPersonName(PersonName &&pn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* adCity, adStreet, adNumberInStreet, adNumberInMunicipality, adZipCode, adState */
		const Address &address(void) const;
		void setAddress(const Address &a);
#ifdef Q_COMPILER_RVALUE_REFS
		void setAddress(Address &&a);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* biDate */
		const QDate &biDate(void) const;
		void setBiDate(const QDate &bd);
#ifdef Q_COMPILER_RVALUE_REFS
		void setBiDate(QDate &&bd);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* userID */
		const QString &userID(void) const;
		void setUserId(const QString &uid);
#ifdef Q_COMPILER_RVALUE_REFS
		void setUserId(QString &&uid);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* userType */
		enum Type::UserType userType(void) const;
		void setUserType(enum Type::UserType ut);
		/* userPrivils */
		Type::Privileges userPrivils(void) const;
		void setUserPrivils(Type::Privileges p);
		/* ic */
		const QString &ic(void) const;
		void setIc(const QString &ic);
#ifdef Q_COMPILER_RVALUE_REFS
		void setIc(QString &&ic);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* firmName */
		const QString &firmName(void) const;
		void setFirmName(const QString &fn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFirmName(QString &&fn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* caStreet */
		const QString &caStreet(void) const;
		void setCaStreet(const QString &cs);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCaStreet(QString &&cs);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* caCity */
		const QString &caCity(void) const;
		void setCaCity(const QString &cc);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCaCity(QString &&cc);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* caZipCode */
		const QString &caZipCode(void) const;
		void setCaZipCode(const QString &cz);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCaZipCode(QString &&cz);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* caState */
		const QString &caState(void) const;
		void setCaState(const QString &cs);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCaState(QString &&cs);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<DbUserInfoPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(DbUserInfo &first, DbUserInfo &second) Q_DECL_NOTHROW;

	class FulltextResultPrivate;
	/*!
	 * @brief Full-text data-box search result.
	 *
	 * pril_2/WS_ISDS_Vyhledavani_datovych_schranek.pdf (section 2.2)
	 */
	class FulltextResult {
		Q_DECLARE_PRIVATE(FulltextResult)

	public:
		FulltextResult(void);
		FulltextResult(const FulltextResult &other);
#ifdef Q_COMPILER_RVALUE_REFS
		FulltextResult(FulltextResult &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~FulltextResult(void);

		FulltextResult &operator=(const FulltextResult &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		FulltextResult &operator=(FulltextResult &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const FulltextResult &other) const;
		bool operator!=(const FulltextResult &other) const;

		friend void swap(FulltextResult &first, FulltextResult &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/*
		 * For convenience purposes. Message identifier consists only
		 * of digits, but documentation explicitly states that it is
		 * a max. 20 chars old string.
		 *
		 * Returns -1 if conversion to number fails.
		 */
		qint64 dbId(void) const;
		void setDbId(qint64 id);

		/* dbID */
		const QString &dbID(void) const;
		void setDbID(const QString &id);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbID(QString &&id);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbType */
		enum Type::DbType dbType(void) const;
		void setDbType(enum Type::DbType bt);
		/* dbName */
		const QString &dbName(void) const;
		void setDbName(const QString &n);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbName(QString &&n);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbAddress */
		const QString &dbAddress(void) const;
		void setDbAddress(const QString &a);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbAddress(QString &&a);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbBiDate */
		const QDate &dbBiDate(void) const;
		void setDbBiDate(const QDate &bd);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbBiDate(QDate &&bd);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbICO */
		const QString &ic(void) const;
		void setIc(const QString &ic);
#ifdef Q_COMPILER_RVALUE_REFS
		void setIc(QString &&ic);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbEffectiveOVM */
		bool dbEffectiveOVM(void) const;
		void setDbEffectiveOVM(bool eo);
		/* dbSendOptions -- Not provided; instead use methods below. */
		bool active(void) const;
		void setActive(bool a);
		bool publicSending(void) const;
		void setPublicSending(bool ps);
		bool commercialSending(void) const;
		void setCommercialSending(bool cs);

		/*
		 * Indexes of start/stop pairs of highlighted name text which
		 * match the sought element.
		 */
		const QList< QPair<int, int> > &nameMatches(void) const;
		void setNameMatches(const QList< QPair<int, int> > &nm);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNameMatches(QList< QPair<int, int> > &&nm);
#endif /* Q_COMPILER_RVALUE_REFS */
		const QList< QPair<int, int> > &addressMatches(void) const;
		void setAddressMatches(const QList< QPair<int, int> > &am);
#ifdef Q_COMPILER_RVALUE_REFS
		void setAddressMatches(QList< QPair<int, int> > &&am);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<FulltextResultPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(FulltextResult &first, FulltextResult &second) Q_DECL_NOTHROW;

	class CreditEventChargedPrivate;
	class CreditEventCharged {
		Q_DECLARE_PRIVATE(CreditEventCharged)

	public:
		CreditEventCharged(void);
		CreditEventCharged(const CreditEventCharged &other);
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventCharged(CreditEventCharged &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~CreditEventCharged(void);

		CreditEventCharged &operator=(const CreditEventCharged &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventCharged &operator=(CreditEventCharged &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const CreditEventCharged &other) const;
		bool operator!=(const CreditEventCharged &other) const;

		friend void swap(CreditEventCharged &first, CreditEventCharged &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* transaction */
		const QString &transactID(void) const;
		void setTransactID(const QString &t);
#ifdef Q_COMPILER_RVALUE_REFS
		void setTransactID(QString &&t);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<CreditEventChargedPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(CreditEventCharged &first, CreditEventCharged &second) Q_DECL_NOTHROW;

	class CreditEventDischarged : public CreditEventCharged {
#if defined (Q_OS_OSX)
	/*
	 * XCode prior to version 9 has problems with inheriting constructors
	 * which should work in C++11.
	 *
	 * Use
	 * # clang/gcc -dM -E - < /dev/null
	 * to identify compiler.
	 */
#  if defined (__clang_major__) && (__clang_major__ < 9)
#    warning "Using user-provided constructors."
	public:
		CreditEventDischarged(void) : CreditEventCharged() { }
#  endif /* __clang_major__ */
#endif /* Q_OS_OSX */
	};

	class CreditEventMsgSentPrivate;
	class CreditEventMsgSent {
		Q_DECLARE_PRIVATE(CreditEventMsgSent)

	public:
		CreditEventMsgSent(void);
		CreditEventMsgSent(const CreditEventMsgSent &other);
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventMsgSent(CreditEventMsgSent &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~CreditEventMsgSent(void);

		CreditEventMsgSent &operator=(const CreditEventMsgSent &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventMsgSent &operator=(CreditEventMsgSent &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const CreditEventMsgSent &other) const;
		bool operator!=(const CreditEventMsgSent &other) const;

		friend void swap(CreditEventMsgSent &first, CreditEventMsgSent &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/*
		 * For convenience purposes. Message identifier consists only
		 * of digits, but documentation explicitly states that it is
		 * a max. 20 chars old string.
		 *
		 * Returns -1 if conversion to number fails.
		 */
		qint64 dmId(void) const;
		void setDmId(qint64 id);

		/* recipient box identifier */
		const QString &dbIDRecipient(void) const;
		void setDbIDRecipient(const QString &id);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbIDRecipient(QString &&id);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* message identifier */
		const QString &dmID(void) const;
		void setDmID(const QString &id);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmID(QString &&id);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<CreditEventMsgSentPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(CreditEventMsgSent &first, CreditEventMsgSent &second) Q_DECL_NOTHROW;

	class CreditEventStorageSetPrivate;
	class CreditEventStorageSet {
		Q_DECLARE_PRIVATE(CreditEventStorageSet)

	public:
		CreditEventStorageSet(void);
		CreditEventStorageSet(const CreditEventStorageSet &other);
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventStorageSet(CreditEventStorageSet &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~CreditEventStorageSet(void);

		CreditEventStorageSet &operator=(const CreditEventStorageSet &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEventStorageSet &operator=(CreditEventStorageSet &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const CreditEventStorageSet &other) const;
		bool operator!=(const CreditEventStorageSet &other) const;

		friend void swap(CreditEventStorageSet &first, CreditEventStorageSet &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* new storage capacity in number of messages */
		qint64 newCapacity(void) const;
		void setNewCapacity(qint64 nc);
		/* new capacity from */
		const QDate &newFrom(void) const;
		void setNewFrom(const QDate &nf);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNewFrom(QDate &&nf);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* new capacity to */
		const QDate &newTo(void) const;
		void setNewTo(const QDate &nt);
#ifdef Q_COMPILER_RVALUE_REFS
		void setNewTo(QDate &&nt);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* optional; old capacity in number of messages */
		qint64 oldCapacity(void) const;
		void setOldCapacity(qint64 oc);
		/* optional; old capacity from */
		const QDate &oldFrom(void) const;
		void setOldFrom(const QDate &of);
#ifdef Q_COMPILER_RVALUE_REFS
		void setOldFrom(QDate &&of);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* optional; old capacity to */
		const QDate &oldTo(void) const;
		void setOldTo(const QDate &ot);
#ifdef Q_COMPILER_RVALUE_REFS
		void setOldTo(QDate &&ot);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* optional; name of user who initiated this change */
		const QString &initiator(void) const;
		void setInitiator(const QString &i);
#ifdef Q_COMPILER_RVALUE_REFS
		void setInitiator(QString &&i);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<CreditEventStorageSetPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(CreditEventStorageSet &first, CreditEventStorageSet &second) Q_DECL_NOTHROW;

	class CreditEventPrivate;
	/*!
	 * @brief Based on tDBCreditInfoOutput (dbTypes.xsd)
	 *
	 * pril_2/WS_ISDS_Vyhledavani_datovych_schranek.pdf (section 2.8)
	 */
	class CreditEvent {
		Q_DECLARE_PRIVATE(CreditEvent)

	public:
		CreditEvent(void);
		CreditEvent(const CreditEvent &other);
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEvent(CreditEvent &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~CreditEvent(void);

		CreditEvent &operator=(const CreditEvent &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		CreditEvent &operator=(CreditEvent &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const CreditEvent &other) const;
		bool operator!=(const CreditEvent &other) const;

		friend void swap(CreditEvent &first, CreditEvent &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* ciEventTime */
		const QDateTime &time(void) const;
		void setTime(const QDateTime &t);
#ifdef Q_COMPILER_RVALUE_REFS
		void setTime(QDateTime &&t);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* ciCreditChange */
		qint64 creditChange(void) const;
		void setCreditChange(qint64 cc);
		/* ciCreditAfter */
		qint64 creditAfter(void) const;
		void setCreditAfter(qint64 ca);
		/* ciEventType */
		enum Type::CreditEventType type(void) const;
		/*
		 * Type is set automatically with assigned data.
		 * Actual record data must be accessed after checking the event
		 * type.
		 */
		const CreditEventCharged &charged(void) const;
		void setCharged(const CreditEventCharged &cec);
#ifdef Q_COMPILER_RVALUE_REFS
		void setCharged(CreditEventCharged &&cec);
#endif /* Q_COMPILER_RVALUE_REFS */
		const CreditEventDischarged &discharged(void) const;
		void setDischarged(const CreditEventDischarged &ced);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDischarged(CreditEventDischarged &&ced);
#endif /* Q_COMPILER_RVALUE_REFS */
		const CreditEventMsgSent &msgSent(void) const;
		void setMsgSent(const CreditEventMsgSent &cems);
#ifdef Q_COMPILER_RVALUE_REFS
		void setMsgSent(CreditEventMsgSent &&cems);
#endif /* Q_COMPILER_RVALUE_REFS */
		const CreditEventStorageSet &storageSet(void) const;
		void setStorageSet(const CreditEventStorageSet &cess);
#ifdef Q_COMPILER_RVALUE_REFS
		void setStorageSet(CreditEventStorageSet &&cess);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* Expired has no actual additional data. */
		void setExpired(void);

	private:
		QScopedPointer<CreditEventPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(CreditEvent &first, CreditEvent &second) Q_DECL_NOTHROW;
}
