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

#include <utility> /* std::move */

#include "src/datovka_shared/isds/box_interface.h"
#include "src/datovka_shared/isds/internal_conversion.h"

/* Null objects - for convenience. */
static const Isds::Address nullAddress;
static const Isds::BirthInfo nullBirthInfo;
static const Isds::CreditEventCharged nullCreditEventCharged;
static const Isds::CreditEventDischarged nullCreditEventDischarged;
static const Isds::CreditEventMsgSent nullCreditEventMsgSent;
static const Isds::CreditEventStorageSet nullCreditEventStorageSet;
static const Isds::PersonName nullPersonName;
static const QDate nullDate;
static const QDateTime nullDateTime;
static const QList< QPair<int, int> > nullMatchList;
static const QString nullString;

/*!
 * @brief PIMPL Address class.
 */
class Isds::AddressPrivate {
	//Q_DISABLE_COPY(AddressPrivate)
public:
	AddressPrivate(void)
	    : m_adCity()/*, m_adDistrict() */, m_adStreet(),
	    m_adNumberInStreet(), m_adNumberInMunicipality(),
	    m_adZipCode(), m_adState()/*, m_adAMCode(-1) */
	{ }

	AddressPrivate &operator=(const AddressPrivate &other) Q_DECL_NOTHROW
	{
		m_adCity = other.m_adCity;
		//m_adDistrict = other.m_adDistrict;
		m_adStreet = other.m_adStreet;
		m_adNumberInStreet = other.m_adNumberInStreet;
		m_adNumberInMunicipality = other.m_adNumberInMunicipality;
		m_adZipCode = other.m_adZipCode;
		m_adState = other.m_adState;
		//m_adAMCode = other.m_adAMCode;

		return *this;
	}

	bool operator==(const AddressPrivate &other) const
	{
		return (m_adCity == other.m_adCity) &&
		//    (m_adDistrict == other.m_adDistrict) &&
		    (m_adStreet == other.m_adStreet) &&
		    (m_adNumberInStreet == other.m_adNumberInStreet) &&
		    (m_adNumberInMunicipality == other.m_adNumberInMunicipality) &&
		    (m_adZipCode == other.m_adZipCode) &&
		    (m_adState == other.m_adState);
		//    (m_adAMCode == other.m_adAMCode);
	}

	QString m_adCity;
	//QString m_adDistrict; /* Not present in libisds-0.10.7. Optional, used in extended structure versions. */
	QString m_adStreet;
	QString m_adNumberInStreet;
	QString m_adNumberInMunicipality;
	QString m_adZipCode;
	QString m_adState;
	//qint64 m_adAMCode; /*
	//                    * Not present in libisds-0.10.7.
	//                    * AM (adresni misto) code according to RUIAN (Registr uzemni identifikace, adres a nemovitosti)
	//                    * It's a number code.
	//                    * Optional, used in extended structure versions.
	//                    */
};

Isds::Address::Address(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Address::Address(const Address &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) AddressPrivate) : Q_NULLPTR)
{
	Q_D(Address);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address::Address(Address &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Address::~Address(void)
{
}

/*!
 * @brief Ensures private address presence.
 *
 * @note Returns if private address could not be allocated.
 */
#define ensureAddressPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			AddressPrivate *p = new (std::nothrow) AddressPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Address &Isds::Address::operator=(const Address &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureAddressPrivate(*this);
	Q_D(Address);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address &Isds::Address::operator=(Address &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Address::operator==(const Address &other) const
{
	Q_D(const Address);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::Address::operator!=(const Address &other) const
{
	return !operator==(other);
}

bool Isds::Address::isNull(void) const
{
	Q_D(const Address);
	return d == Q_NULLPTR;
}

const QString &Isds::Address::city(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adCity;
}

void Isds::Address::setCity(const QString &c)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adCity = c;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setCity(QString &&c)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adCity = c;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Address::street(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adStreet;
}

void Isds::Address::setStreet(const QString &s)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adStreet = s;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setStreet(QString &&s)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adStreet = s;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Address::numberInStreet(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adNumberInStreet;
}

void Isds::Address::setNumberInStreet(const QString &nis)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adNumberInStreet = nis;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setNumberInStreet(QString &&nis)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adNumberInStreet = nis;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Address::numberInMunicipality(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adNumberInMunicipality;
}

void Isds::Address::setNumberInMunicipality(const QString &nim)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adNumberInMunicipality = nim;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setNumberInMunicipality(QString &&nim)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adNumberInMunicipality = nim;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Address::zipCode(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adZipCode;
}

void Isds::Address::setZipCode(const QString &zc)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adZipCode = zc;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setZipCode(QString &&zc)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adZipCode = zc;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Address::state(void) const
{
	Q_D(const Address);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_adState;
}

void Isds::Address::setState(const QString &s)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adState = s;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Address::setState(QString &&s)
{
	ensureAddressPrivate();
	Q_D(Address);
	d->m_adState = s;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(Address &first, Address &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL BirthInfo class.
 */
class Isds::BirthInfoPrivate {
	//Q_DISABLE_COPY(BirthInfoPrivate)
public:
	BirthInfoPrivate(void)
	    : m_biDate(), m_biCity(), m_biCounty(), m_biState()
	{ }

	BirthInfoPrivate &operator=(const BirthInfoPrivate &other) Q_DECL_NOTHROW
	{
		m_biDate = other.m_biDate;
		m_biCity = other.m_biCity;
		m_biCounty = other.m_biCounty;
		m_biState = other.m_biState;

		return *this;
	}

	bool operator==(const BirthInfoPrivate &other) const
	{
		return (m_biDate == other.m_biDate) &&
		    (m_biCity == other.m_biCity) &&
		    (m_biCounty == other.m_biCounty) &&
		    (m_biState == other.m_biState);
	}

	QDate m_biDate;
	QString m_biCity;
	QString m_biCounty;
	QString m_biState;
};

Isds::BirthInfo::BirthInfo(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::BirthInfo::BirthInfo(const BirthInfo &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) BirthInfoPrivate) : Q_NULLPTR)
{
	Q_D(BirthInfo);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo::BirthInfo(BirthInfo &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::BirthInfo::~BirthInfo(void)
{
}

/*!
 * @brief Ensures private birth info presence.
 *
 * @note Returns if private birth info could not be allocated.
 */
#define ensureBirthInfoPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			BirthInfoPrivate *p = new (std::nothrow) BirthInfoPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::BirthInfo &Isds::BirthInfo::operator=(const BirthInfo &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureBirthInfoPrivate(*this);
	Q_D(BirthInfo);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo &Isds::BirthInfo::operator=(BirthInfo &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::BirthInfo::operator==(const BirthInfo &other) const
{
	Q_D(const BirthInfo);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::BirthInfo::operator!=(const BirthInfo &other) const
{
	return !operator==(other);
}

bool Isds::BirthInfo::isNull(void) const
{
	Q_D(const BirthInfo);
	return d == Q_NULLPTR;
}

const QDate &Isds::BirthInfo::date(void) const
{
	Q_D(const BirthInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}

	return d->m_biDate;
}

void Isds::BirthInfo::setDate(const QDate &bd)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biDate = bd;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::BirthInfo::setDate(QDate &&bd)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biDate = bd;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::BirthInfo::city(void) const
{
	Q_D(const BirthInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_biCity;
}

void Isds::BirthInfo::setCity(const QString &c)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biCity = c;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::BirthInfo::setCity(QString &&c)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biCity = c;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::BirthInfo::county(void) const
{
	Q_D(const BirthInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_biCounty;
}

void Isds::BirthInfo::setCounty(const QString &c)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biCounty = c;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::BirthInfo::setCounty(QString &&c)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biCounty = c;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::BirthInfo::state(void) const
{
	Q_D(const BirthInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_biState;
}

void Isds::BirthInfo::setState(const QString &s)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biState = s;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::BirthInfo::setState(QString &&s)
{
	ensureBirthInfoPrivate();
	Q_D(BirthInfo);
	d->m_biState = s;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(BirthInfo &first, BirthInfo &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL PersonName class.
 */
class Isds::PersonNamePrivate {
	//Q_DISABLE_COPY(PersonNamePrivate)
public:
	PersonNamePrivate(void)
	    : m_pnFirstName(), m_pnMiddleName(), m_pnLastName(),
	    m_pnLastNameAtBirth()
	{ }

	PersonNamePrivate &operator=(const PersonNamePrivate &other) Q_DECL_NOTHROW
	{
		m_pnFirstName = other.m_pnFirstName;
		m_pnMiddleName = other.m_pnMiddleName;
		m_pnLastName = other.m_pnLastName;
		m_pnLastNameAtBirth = other.m_pnLastNameAtBirth;

		return *this;
	}

	bool operator==(const PersonNamePrivate &other) const
	{
		return (m_pnFirstName == other.m_pnFirstName) &&
		    (m_pnMiddleName == other.m_pnMiddleName) &&
		    (m_pnLastName == other.m_pnLastName) &&
		    (m_pnLastNameAtBirth == other.m_pnLastNameAtBirth);
	}

	QString m_pnFirstName;
	QString m_pnMiddleName;
	QString m_pnLastName;
	QString m_pnLastNameAtBirth;
};

Isds::PersonName::PersonName(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::PersonName::PersonName(const PersonName &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) PersonNamePrivate) : Q_NULLPTR)
{
	Q_D(PersonName);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName::PersonName(PersonName &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName::~PersonName(void)
{
}

/*!
 * @brief Ensures private person name presence.
 *
 * @note Returns if private person name could not be allocated.
 */
#define ensurePersonNamePrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			PersonNamePrivate *p = new (std::nothrow) PersonNamePrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::PersonName &Isds::PersonName::operator=(const PersonName &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensurePersonNamePrivate(*this);
	Q_D(PersonName);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName &Isds::PersonName::operator=(PersonName &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::PersonName::operator==(const PersonName &other) const
{
	Q_D(const PersonName);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::PersonName::operator!=(const PersonName &other) const
{
	return !operator==(other);
}

bool Isds::PersonName::isNull(void) const
{
	Q_D(const PersonName);
	return d == Q_NULLPTR;
}

const QString &Isds::PersonName::firstName(void) const
{
	Q_D(const PersonName);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_pnFirstName;
}

void Isds::PersonName::setFirstName(const QString &fn)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnFirstName = fn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::PersonName::setFirstName(QString &&fn)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnFirstName = fn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::PersonName::middleName(void) const
{
	Q_D(const PersonName);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_pnMiddleName;
}

void Isds::PersonName::setMiddleName(const QString &mn)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnMiddleName = mn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::PersonName::setMiddleName(QString &&mn)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnMiddleName = mn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::PersonName::lastName(void) const
{
	Q_D(const PersonName);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_pnLastName;
}

void Isds::PersonName::setLastName(const QString &ln)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnLastName = ln;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::PersonName::setLastName(QString &&ln)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnLastName = ln;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::PersonName::lastNameAtBirth(void) const
{
	Q_D(const PersonName);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_pnLastNameAtBirth;
}

void Isds::PersonName::setLastNameAtBirth(const QString &lnab)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnLastNameAtBirth = lnab;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::PersonName::setLastNameAtBirth(QString &&lnab)
{
	ensurePersonNamePrivate();
	Q_D(PersonName);
	d->m_pnLastNameAtBirth = lnab;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(PersonName &first, PersonName &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL DbOwnerInfo class.
 */
class Isds::DbOwnerInfoPrivate {
	//Q_DISABLE_COPY(DbOwnerInfoPrivate)
public:
	DbOwnerInfoPrivate(void)
	    : m_dbID(), m_dbType(Type::BT_NULL), m_ic(), m_personName(),
	    m_firmName(), m_birthInfo(), m_address(), m_nationality(),
	    m_email(), m_telNumber(), m_identifier(), m_registryCode(),
	    m_dbState(Type::BS_ERROR),
	    m_dbEffectiveOVM(Type::BOOL_NULL),
	    m_dbOpenAddressing(Type::BOOL_NULL)
	{ }

	DbOwnerInfoPrivate &operator=(const DbOwnerInfoPrivate &other) Q_DECL_NOTHROW
	{
		m_dbID = other.m_dbID;
		m_dbType = other.m_dbType;
		m_ic = other.m_ic;
		m_personName = other.m_personName;
		m_firmName = other.m_firmName;
		m_birthInfo = other.m_birthInfo;
		m_address = other.m_address;
		m_nationality = other.m_nationality;
		m_email = other.m_email;
		m_telNumber = other.m_telNumber;
		m_identifier = other.m_identifier;
		m_registryCode = other.m_registryCode;
		m_dbState = other.m_dbState;
		m_dbEffectiveOVM = other.m_dbEffectiveOVM;
		m_dbOpenAddressing = other.m_dbOpenAddressing;

		return *this;
	}

	bool operator==(const DbOwnerInfoPrivate &other) const
	{
		return (m_dbID == other.m_dbID) &&
		    (m_dbType == other.m_dbType) &&
		    (m_ic == other.m_ic) &&
		    (m_personName == other.m_personName) &&
		    (m_firmName == other.m_firmName) &&
		    (m_birthInfo == other.m_birthInfo) &&
		    (m_address == other.m_address) &&
		    (m_nationality == other.m_nationality) &&
		    (m_email == other.m_email) &&
		    (m_telNumber == other.m_telNumber) &&
		    (m_identifier == other.m_identifier) &&
		    (m_registryCode == other.m_registryCode) &&
		    (m_dbState == other.m_dbState) &&
		    (m_dbEffectiveOVM == other.m_dbEffectiveOVM) &&
		    (m_dbOpenAddressing == other.m_dbOpenAddressing);
	}

	QString m_dbID;
	enum Type::DbType m_dbType;
	QString m_ic;
	PersonName m_personName;
	QString m_firmName;
	BirthInfo m_birthInfo;
	Address m_address;
	QString m_nationality;
	QString m_email;
	QString m_telNumber;
	QString m_identifier;
	QString m_registryCode;
	enum Type::DbState m_dbState;
	enum Type::NilBool m_dbEffectiveOVM;
	enum Type::NilBool m_dbOpenAddressing;
};

Isds::DbOwnerInfo::DbOwnerInfo(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::DbOwnerInfo::DbOwnerInfo(const DbOwnerInfo &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) DbOwnerInfoPrivate) : Q_NULLPTR)
{
	Q_D(DbOwnerInfo);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbOwnerInfo::DbOwnerInfo(DbOwnerInfo &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::DbOwnerInfo::~DbOwnerInfo(void)
{
}

/*!
 * @brief Ensures private box owner info info presence.
 *
 * @note Returns if private box owner info could not be allocated.
 */
#define ensureDbOwnerInfoPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			DbOwnerInfoPrivate *p = new (std::nothrow) DbOwnerInfoPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::DbOwnerInfo &Isds::DbOwnerInfo::operator=(const DbOwnerInfo &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureDbOwnerInfoPrivate(*this);
	Q_D(DbOwnerInfo);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbOwnerInfo &Isds::DbOwnerInfo::operator=(DbOwnerInfo &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::DbOwnerInfo::operator==(const DbOwnerInfo &other) const
{
	Q_D(const DbOwnerInfo);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::DbOwnerInfo::operator!=(const DbOwnerInfo &other) const
{
	return !operator==(other);
}

bool Isds::DbOwnerInfo::isNull(void) const
{
	Q_D(const DbOwnerInfo);
	return d == Q_NULLPTR;
}

const QString &Isds::DbOwnerInfo::dbID(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_dbID;
}

void Isds::DbOwnerInfo::setDbID(const QString &bi)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbID = bi;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setDbID(QString &&bi)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbID = bi;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::DbType Isds::DbOwnerInfo::dbType(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BT_NULL;
	}

	return d->m_dbType;
}

void Isds::DbOwnerInfo::setDbType(enum Type::DbType bt)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbType = bt;
}

const QString &Isds::DbOwnerInfo::ic(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_ic;
}

void Isds::DbOwnerInfo::setIc(const QString &ic)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_ic = ic;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setIc(QString &&ic)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_ic = ic;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::PersonName &Isds::DbOwnerInfo::personName(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullPersonName;
	}

	return d->m_personName;
}

void Isds::DbOwnerInfo::setPersonName(const PersonName &pn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_personName = pn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setPersonName(PersonName &&pn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_personName = pn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::firmName(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_firmName;
}

void Isds::DbOwnerInfo::setFirmName(const QString &fn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_firmName = fn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setFirmName(QString &&fn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_firmName = fn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::BirthInfo &Isds::DbOwnerInfo::birthInfo(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullBirthInfo;
	}

	return d->m_birthInfo;
}

void Isds::DbOwnerInfo::setBirthInfo(const BirthInfo &bi)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_birthInfo = bi;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setBirthInfo(BirthInfo &&bi)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_birthInfo = bi;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::Address &Isds::DbOwnerInfo::address(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullAddress;
	}

	return d->m_address;
}

void Isds::DbOwnerInfo::setAddress(const Address &a)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_address = a;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setAddress(Address &&a)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_address = a;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::nationality(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_nationality;
}

void Isds::DbOwnerInfo::setNationality(const QString &n)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_nationality = n;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setNationality(QString &&n)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_nationality = n;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::email(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_email;
}

void Isds::DbOwnerInfo::setEmail(const QString &e)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_email = e;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setEmail(QString &&e)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_email = e;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::telNumber(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_telNumber;
}

void Isds::DbOwnerInfo::setTelNumber(const QString &tn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_telNumber = tn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setTelNumber(QString &&tn)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_telNumber = tn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::identifier(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_identifier;
}

void Isds::DbOwnerInfo::setIdentifier(const QString &i)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_identifier = i;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setIdentifier(QString &&i)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_identifier = i;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbOwnerInfo::registryCode(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_registryCode;
}

void Isds::DbOwnerInfo::setRegistryCode(const QString &rc)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_registryCode = rc;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbOwnerInfo::setRegistryCode(QString &&rc)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_registryCode = rc;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::DbState Isds::DbOwnerInfo::dbState(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BS_ERROR;
	}

	return d->m_dbState;
}

void Isds::DbOwnerInfo::setDbState(enum Type::DbState bs)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbState = bs;
}

enum Isds::Type::NilBool Isds::DbOwnerInfo::dbEffectiveOVM(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}

	return d->m_dbEffectiveOVM;
}

void Isds::DbOwnerInfo::setDbEffectiveOVM(enum Type::NilBool eo)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbEffectiveOVM = eo;
}

enum Isds::Type::NilBool Isds::DbOwnerInfo::dbOpenAddressing(void) const
{
	Q_D(const DbOwnerInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}

	return d->m_dbOpenAddressing;
}

void Isds::DbOwnerInfo::setDbOpenAddressing(enum Type::NilBool oa)
{
	ensureDbOwnerInfoPrivate();
	Q_D(DbOwnerInfo);
	d->m_dbOpenAddressing = oa;
}

void Isds::swap(DbOwnerInfo &first, DbOwnerInfo &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL DbUserInfo class.
 */
class Isds::DbUserInfoPrivate {
	//Q_DISABLE_COPY(DbUserInfoPrivate)
public:
	DbUserInfoPrivate(void)
	    : m_personName(), m_address(), m_biDate(), m_userID(),
	    m_userType(Type::UT_NULL), m_userPrivils(Type::PRIVIL_NONE),
	    m_ic(), m_firmName(), m_caStreet(), m_caCity(), m_caZipCode(),
	    m_caState()
	{ }

	DbUserInfoPrivate &operator=(const DbUserInfoPrivate &other) Q_DECL_NOTHROW
	{
		m_personName = other.m_personName;
		m_address = other.m_address;
		m_biDate = other.m_biDate;
		m_userID = other.m_userID;
		m_userType = other.m_userType;
		m_userPrivils = other.m_userPrivils;
		m_ic = other.m_ic;
		m_firmName = other.m_firmName;
		m_caStreet = other.m_caStreet;
		m_caCity = other.m_caCity;
		m_caZipCode = other.m_caZipCode;
		m_caState = other.m_caState;

		return *this;
	}

	bool operator==(const DbUserInfoPrivate &other) const
	{
		return (m_personName == other.m_personName) &&
		    (m_address == other.m_address) &&
		    (m_biDate == other.m_biDate) &&
		    (m_userID == other.m_userID) &&
		    (m_userType == other.m_userType) &&
		    (m_userPrivils == other.m_userPrivils) &&
		    (m_ic == other.m_ic) &&
		    (m_firmName == other.m_firmName) &&
		    (m_caStreet == other.m_caStreet) &&
		    (m_caCity == other.m_caCity) &&
		    (m_caZipCode == other.m_caZipCode) &&
		    (m_caState == other.m_caState);
	}

	PersonName m_personName;
	Address m_address;
	QDate m_biDate;
	QString m_userID;
	enum Type::UserType m_userType;
	Type::Privileges m_userPrivils;
	QString m_ic;
	QString m_firmName;
	QString m_caStreet;
	QString m_caCity;
	QString m_caZipCode;
	QString m_caState;
	//QString m_aifoTicket; /* Not present in libisds-0.10.7. */
};

Isds::DbUserInfo::DbUserInfo(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::DbUserInfo::DbUserInfo(const DbUserInfo &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) DbUserInfoPrivate) : Q_NULLPTR)
{
	Q_D(DbUserInfo);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbUserInfo::DbUserInfo(DbUserInfo &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::DbUserInfo::~DbUserInfo(void)
{
}

/*!
 * @brief Ensures private box user info info presence.
 *
 * @note Returns if private box user info could not be allocated.
 */
#define ensureDbUserInfoPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			DbUserInfoPrivate *p = new (std::nothrow) DbUserInfoPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::DbUserInfo &Isds::DbUserInfo::operator=(const DbUserInfo &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureDbUserInfoPrivate(*this);
	Q_D(DbUserInfo);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbUserInfo &Isds::DbUserInfo::operator=(DbUserInfo &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::DbUserInfo::operator==(const DbUserInfo &other) const
{
	Q_D(const DbUserInfo);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::DbUserInfo::operator!=(const DbUserInfo &other) const
{
	return !operator==(other);
}

bool Isds::DbUserInfo::isNull(void) const
{
	Q_D(const DbUserInfo);
	return d == Q_NULLPTR;
}

const Isds::PersonName &Isds::DbUserInfo::personName(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullPersonName;
	}

	return d->m_personName;
}

void Isds::DbUserInfo::setPersonName(const PersonName &pn)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_personName = pn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setPersonName(PersonName &&pn)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_personName = pn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::Address &Isds::DbUserInfo::address(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullAddress;
	}

	return d->m_address;
}

void Isds::DbUserInfo::setAddress(const Address &a)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_address = a;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setAddress(Address &&a)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_address = a;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QDate &Isds::DbUserInfo::biDate(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}

	return d->m_biDate;
}

void Isds::DbUserInfo::setBiDate(const QDate &bd)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_biDate = bd;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setBiDate(QDate &&bd)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_biDate = bd;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::userID(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_userID;
}

void Isds::DbUserInfo::setUserId(const QString &uid)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_userID = uid;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setUserId(QString &&uid)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_userID = uid;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::UserType Isds::DbUserInfo::userType(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::UT_NULL;
	}

	return d->m_userType;
}

void Isds::DbUserInfo::setUserType(enum Type::UserType ut)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_userType = ut;
}

Isds::Type::Privileges Isds::DbUserInfo::userPrivils(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::PRIVIL_NONE;
	}

	return d->m_userPrivils;
}

void Isds::DbUserInfo::setUserPrivils(Type::Privileges p)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_userPrivils = p;
}

const QString &Isds::DbUserInfo::ic(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_ic;
}

void Isds::DbUserInfo::setIc(const QString &ic)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_ic = ic;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setIc(QString &&ic)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_ic = ic;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::firmName(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_firmName;
}

void Isds::DbUserInfo::setFirmName(const QString &fn)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_firmName = fn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setFirmName(QString &&fn)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_firmName = fn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::caStreet(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_caStreet;
}

void Isds::DbUserInfo::setCaStreet(const QString &cs)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caStreet = cs;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setCaStreet(QString &&cs)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caStreet = cs;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::caCity(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_caCity;
}

void Isds::DbUserInfo::setCaCity(const QString &cc)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caCity = cc;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setCaCity(QString &&cc)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caCity = cc;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::caZipCode(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_caZipCode;
}

void Isds::DbUserInfo::setCaZipCode(const QString &cz)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caZipCode = cz;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setCaZipCode(QString &&cz)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caZipCode = cz;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::DbUserInfo::caState(void) const
{
	Q_D(const DbUserInfo);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_caState;
}

void Isds::DbUserInfo::setCaState(const QString &cs)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caState = cs;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::DbUserInfo::setCaState(QString &&cs)
{
	ensureDbUserInfoPrivate();
	Q_D(DbUserInfo);
	d->m_caState = cs;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(DbUserInfo &first, DbUserInfo &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL full-text data-box search result class.
 */
class Isds::FulltextResultPrivate {
	//Q_DISABLE_COPY(FulltextResultPrivate)
public:
	FulltextResultPrivate(void)
	    : m_dbID(), m_dbType(Isds::Type::BT_NULL), m_dbName(), m_dbAddress(),
	    m_dbBiDate(), m_dbICO(), m_dbEffectiveOVM(false),
	    active(false), publicSending(false), commercialSending(false),
	    nameMatches(), addressMatches()
	{ }

	FulltextResultPrivate &operator=(const FulltextResultPrivate &other) Q_DECL_NOTHROW
	{
		m_dbID = other.m_dbID;
		m_dbType = other.m_dbType;
		m_dbName = other.m_dbName;
		m_dbAddress = other.m_dbAddress;
		m_dbBiDate = other.m_dbBiDate;
		m_dbICO = other.m_dbICO;
		m_dbEffectiveOVM = other.m_dbEffectiveOVM;
		active = other.active;
		publicSending = other.publicSending;
		commercialSending = other.commercialSending;
		nameMatches = other.nameMatches;
		addressMatches = other.addressMatches;

		return *this;
	}

	bool operator==(const FulltextResultPrivate &other) const
	{
		return (m_dbID == other.m_dbID) &&
		    (m_dbType == other.m_dbType) &&
		    (m_dbName == other.m_dbName) &&
		    (m_dbAddress == other.m_dbAddress) &&
		    (m_dbBiDate == other.m_dbBiDate) &&
		    (m_dbICO == other.m_dbICO) &&
		    (m_dbEffectiveOVM == other.m_dbEffectiveOVM) &&
		    (active == other.active) &&
		    (publicSending == other.publicSending) &&
		    (commercialSending == other.commercialSending) &&
		    (nameMatches == other.nameMatches) &&
		    (addressMatches == other.addressMatches);
	}

	QString m_dbID;
	enum Isds::Type::DbType m_dbType;
	QString m_dbName;
	QString m_dbAddress;
	QDate m_dbBiDate;
	QString m_dbICO;
	bool m_dbEffectiveOVM;
	/*
	 * Following entries are derived from libisds at it does not provide
	 * interface for dbSendOptions.
	 */
	bool active; /* Box can receive messages. */
	bool publicSending; /* Seeker box can send ordinary messages into this box. */
	bool commercialSending; /* Seeker box can send commercial messages into this box. */

	QList< QPair<int, int> > nameMatches;
	QList< QPair<int, int> > addressMatches;
};

Isds::FulltextResult::FulltextResult(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::FulltextResult::FulltextResult(const FulltextResult &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) FulltextResultPrivate) : Q_NULLPTR)
{
	Q_D(FulltextResult);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::FulltextResult::FulltextResult(FulltextResult &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::FulltextResult::~FulltextResult(void)
{
}

/*!
 * @brief Ensures private full-text search result presence.
 *
 * @note Returns if private full-text search result could not be allocated.
 */
#define ensureFulltextResultPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			FulltextResultPrivate *p = new (std::nothrow) FulltextResultPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::FulltextResult &Isds::FulltextResult::operator=(
    const FulltextResult &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureFulltextResultPrivate(*this);
	Q_D(FulltextResult);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::FulltextResult &Isds::FulltextResult::operator=(
    FulltextResult &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::FulltextResult::operator==(const FulltextResult &other) const
{
	Q_D(const FulltextResult);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::FulltextResult::operator!=(const FulltextResult &other) const
{
	return !operator==(other);
}

bool Isds::FulltextResult::isNull(void) const
{
	Q_D(const FulltextResult);
	return d == Q_NULLPTR;
}

qint64 Isds::FulltextResult::dbId(void) const
{
	bool ok = false;
	qint64 id = string2NonNegativeLong(dbID(), &ok);
	return ok ? id : -1;
}

void Isds::FulltextResult::setDbId(qint64 id)
{
	setDbID(nonNegativeLong2String(id));
}

const QString &Isds::FulltextResult::dbID(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbID;
}

void Isds::FulltextResult::setDbID(const QString &id)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbID = id;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setDbID(QString &&id)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbID = id;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::DbType Isds::FulltextResult::dbType(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BT_NULL;
	}
	return d->m_dbType;
}

void Isds::FulltextResult::setDbType(enum Type::DbType bt)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbType = bt;
}

const QString &Isds::FulltextResult::dbName(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbName;
}

void Isds::FulltextResult::setDbName(const QString &n)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbName = n;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setDbName(QString &&n)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbName = n;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::FulltextResult::dbAddress(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbAddress;
}

void Isds::FulltextResult::setDbAddress(const QString &a)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbAddress = a;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setDbAddress(QString &&a)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbAddress = a;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QDate &Isds::FulltextResult::dbBiDate(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}
	return d->m_dbBiDate;
}

void Isds::FulltextResult::setDbBiDate(const QDate &bd)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbBiDate = bd;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setDbBiDate(QDate &&bd)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbBiDate = bd;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::FulltextResult::ic(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbICO;
}

void Isds::FulltextResult::setIc(const QString &ic)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbICO = ic;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setIc(QString &&ic)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbICO = ic;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::FulltextResult::dbEffectiveOVM(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}
	return d->m_dbEffectiveOVM;
}

void Isds::FulltextResult::setDbEffectiveOVM(bool eo)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->m_dbEffectiveOVM = eo;
}

bool Isds::FulltextResult::active(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}
	return d->active;
}

void Isds::FulltextResult::setActive(bool a)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->active = a;
}

bool Isds::FulltextResult::publicSending(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}
	return d->publicSending;
}

void Isds::FulltextResult::setPublicSending(bool ps)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->publicSending = ps;
}

bool Isds::FulltextResult::commercialSending(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}
	return d->commercialSending;
}

void Isds::FulltextResult::setCommercialSending(bool cs)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->commercialSending = cs;
}

const QList< QPair<int, int> > &Isds::FulltextResult::nameMatches(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullMatchList;
	}
	return d->nameMatches;
}

void Isds::FulltextResult::setNameMatches(const QList< QPair<int, int> > &nm)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->nameMatches = nm;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setNameMatches(QList< QPair<int, int> > &&nm)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->nameMatches = nm;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QList< QPair<int, int> > &Isds::FulltextResult::addressMatches(void) const
{
	Q_D(const FulltextResult);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullMatchList;
	}
	return d->addressMatches;
}

void Isds::FulltextResult::setAddressMatches(const QList< QPair<int, int> > &am)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->addressMatches = am;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::FulltextResult::setAddressMatches(QList< QPair<int, int> > &&am)
{
	ensureFulltextResultPrivate();
	Q_D(FulltextResult);
	d->addressMatches = am;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(FulltextResult &first, FulltextResult &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL CreditEventCharged class.
 */
class Isds::CreditEventChargedPrivate {
	//Q_DISABLE_COPY(CreditEventChargedPrivate)

public:
	CreditEventChargedPrivate(void)
	    : m_transaction()
	{ }

	CreditEventChargedPrivate &operator=(const CreditEventChargedPrivate &other) Q_DECL_NOTHROW
	{
		m_transaction = other.m_transaction;

		return *this;
	}

	bool operator==(const CreditEventChargedPrivate &other) const
	{
		return (m_transaction == other.m_transaction);
	}

	QString m_transaction; /*!< Transaction identifier. */
};

Isds::CreditEventCharged::CreditEventCharged(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::CreditEventCharged::CreditEventCharged(const CreditEventCharged &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) CreditEventChargedPrivate) : Q_NULLPTR)
{
	Q_D(CreditEventCharged);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventCharged::CreditEventCharged(CreditEventCharged &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::CreditEventCharged::~CreditEventCharged(void)
{
}

/*!
 * @brief Ensures private charge credit event presence.
 *
 * @note Returns if private charge credit event could not be allocated.
 */
#define ensureCreditEventChargedPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			CreditEventChargedPrivate *p = new (std::nothrow) CreditEventChargedPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::CreditEventCharged &Isds::CreditEventCharged::operator=(const CreditEventCharged &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureCreditEventChargedPrivate(*this);
	Q_D(CreditEventCharged);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventCharged &Isds::CreditEventCharged::operator=(CreditEventCharged &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::CreditEventCharged::operator==(const CreditEventCharged &other) const
{
	Q_D(const CreditEventCharged);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::CreditEventCharged::operator!=(const CreditEventCharged &other) const
{
	return !operator==(other);
}

bool Isds::CreditEventCharged::isNull(void) const
{
	Q_D(const CreditEventCharged);
	return d == Q_NULLPTR;
}

const QString &Isds::CreditEventCharged::transactID(void) const
{
	Q_D(const CreditEventCharged);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_transaction;
}

void Isds::CreditEventCharged::setTransactID(const QString &t)
{
	ensureCreditEventChargedPrivate();
	Q_D(CreditEventCharged);
	d->m_transaction = t;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventCharged::setTransactID(QString &&t)
{
	ensureCreditEventChargedPrivate();
	Q_D(CreditEventCharged);
	d->m_transaction = t;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(CreditEventCharged &first, CreditEventCharged &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL CreditEventMsgSent class.
 */
class Isds::CreditEventMsgSentPrivate {
	//Q_DISABLE_COPY(CreditEventMsgSentPrivate)

public:
	CreditEventMsgSentPrivate(void)
	    : m_dbIDRecipient(), m_dmID()
	{ }

	CreditEventMsgSentPrivate &operator=(const CreditEventMsgSentPrivate &other) Q_DECL_NOTHROW
	{
		m_dbIDRecipient = other.m_dbIDRecipient;
		m_dmID = other.m_dmID;

		return *this;
	}

	bool operator==(const CreditEventMsgSentPrivate &other) const
	{
		return (m_dbIDRecipient == other.m_dbIDRecipient) &&
		    (m_dmID == other.m_dmID);
	}

	QString m_dbIDRecipient; /*!< Recipient box identifier. */
	QString m_dmID; /*!< Sent message identifier. */
};

Isds::CreditEventMsgSent::CreditEventMsgSent(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::CreditEventMsgSent::CreditEventMsgSent(const CreditEventMsgSent &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) CreditEventMsgSentPrivate) : Q_NULLPTR)
{
	Q_D(CreditEventMsgSent);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventMsgSent::CreditEventMsgSent(CreditEventMsgSent &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::CreditEventMsgSent::~CreditEventMsgSent(void)
{
}

/*!
 * @brief Ensures private message sent event presence.
 *
 * @note Returns if private message sent event could not be allocated.
 */
#define ensureCreditEventMsgSentPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			CreditEventMsgSentPrivate *p = new (std::nothrow) CreditEventMsgSentPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::CreditEventMsgSent &Isds::CreditEventMsgSent::operator=(const CreditEventMsgSent &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureCreditEventMsgSentPrivate(*this);
	Q_D(CreditEventMsgSent);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventMsgSent &Isds::CreditEventMsgSent::operator=(CreditEventMsgSent &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::CreditEventMsgSent::operator==(const CreditEventMsgSent &other) const
{
	Q_D(const CreditEventMsgSent);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::CreditEventMsgSent::operator!=(const CreditEventMsgSent &other) const
{
	return !operator==(other);
}

bool Isds::CreditEventMsgSent::isNull(void) const
{
	Q_D(const CreditEventMsgSent);
	return d == Q_NULLPTR;
}

qint64 Isds::CreditEventMsgSent::dmId(void) const
{
	bool ok = false;
	qint64 id = string2NonNegativeLong(dmID(), &ok);
	return ok ? id : -1;
}

void Isds::CreditEventMsgSent::setDmId(qint64 id)
{
	setDmID(nonNegativeLong2String(id));
}

const QString &Isds::CreditEventMsgSent::dbIDRecipient(void) const
{
	Q_D(const CreditEventMsgSent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_dbIDRecipient;
}

void Isds::CreditEventMsgSent::setDbIDRecipient(const QString &id)
{
	ensureCreditEventMsgSentPrivate();
	Q_D(CreditEventMsgSent);
	d->m_dbIDRecipient = id;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventMsgSent::setDbIDRecipient(QString &&id)
{
	ensureCreditEventMsgSentPrivate();
	Q_D(CreditEventMsgSent);
	d->m_dbIDRecipient = id;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::CreditEventMsgSent::dmID(void) const
{
	Q_D(const CreditEventMsgSent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_dmID;
}

void Isds::CreditEventMsgSent::setDmID(const QString &id)
{
	ensureCreditEventMsgSentPrivate();
	Q_D(CreditEventMsgSent);
	d->m_dmID = id;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventMsgSent::setDmID(QString &&id)
{
	ensureCreditEventMsgSentPrivate();
	Q_D(CreditEventMsgSent);
	d->m_dmID = id;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(CreditEventMsgSent &first, CreditEventMsgSent &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL CreditEventStorageSet class.
 */
class Isds::CreditEventStorageSetPrivate {
	//Q_DISABLE_COPY(CreditEventStorageSetPrivate)

public:
	CreditEventStorageSetPrivate(void)
	    : m_newCapacity(-1), m_newFrom(), m_newTo(), m_oldCapacity(-1),
	    m_oldFrom(), m_oldTo(), m_initiator()
	{ }

	CreditEventStorageSetPrivate &operator=(const CreditEventStorageSetPrivate &other) Q_DECL_NOTHROW
	{
		m_newCapacity = other.m_newCapacity;
		m_newFrom = other.m_newFrom;
		m_newTo = other.m_newTo;
		m_oldCapacity = other.m_oldCapacity;
		m_oldFrom = other.m_oldFrom;
		m_oldTo = other.m_oldTo;
		m_initiator = other.m_initiator;

		return *this;
	}

	bool operator==(const CreditEventStorageSetPrivate &other) const
	{
		return (m_newCapacity == other.m_newCapacity) &&
		    (m_newFrom == other.m_newFrom) &&
		    (m_newTo == other.m_newTo) &&
		    (m_oldCapacity == other.m_oldCapacity) &&
		    (m_oldFrom == other.m_oldFrom) &&
		    (m_oldTo == other.m_oldTo) &&
		    (m_initiator == other.m_initiator);
	}

	qint64 m_newCapacity; /*!< New storage capacity in number of messages. */
	QDate m_newFrom; /*!< The new capacity is available from this date. */
	QDate m_newTo; /*!< The new capacity expires on this date. */
	qint64 m_oldCapacity; /*!< Optional value. Previous storage capacity in number of messages. */
	QDate m_oldFrom; /*!< Optional value. */
	QDate m_oldTo; /*!< Optional value. */
	QString m_initiator; /*!< Optional value. Name of a user who initiated the change. */
};

Isds::CreditEventStorageSet::CreditEventStorageSet(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::CreditEventStorageSet::CreditEventStorageSet(const CreditEventStorageSet &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) CreditEventStorageSetPrivate) : Q_NULLPTR)
{
	Q_D(CreditEventStorageSet);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventStorageSet::CreditEventStorageSet(CreditEventStorageSet &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::CreditEventStorageSet::~CreditEventStorageSet(void)
{
}

/*!
 * @brief Ensures private storage set event presence.
 *
 * @note Returns if private storage set event could not be allocated.
 */
#define ensureCreditEventStorageSetPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			CreditEventStorageSetPrivate *p = new (std::nothrow) CreditEventStorageSetPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::CreditEventStorageSet &Isds::CreditEventStorageSet::operator=(const CreditEventStorageSet &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureCreditEventStorageSetPrivate(*this);
	Q_D(CreditEventStorageSet);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEventStorageSet &Isds::CreditEventStorageSet::operator=(CreditEventStorageSet &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::CreditEventStorageSet::operator==(const CreditEventStorageSet &other) const
{
	Q_D(const CreditEventStorageSet);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::CreditEventStorageSet::operator!=(const CreditEventStorageSet &other) const
{
	return !operator==(other);
}

bool Isds::CreditEventStorageSet::isNull(void) const
{
	Q_D(const CreditEventStorageSet);
	return d == Q_NULLPTR;
}

qint64 Isds::CreditEventStorageSet::newCapacity(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_newCapacity;
}

void Isds::CreditEventStorageSet::setNewCapacity(qint64 nc)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_newCapacity = (nc >= 0) ? nc : -1;
}

const QDate &Isds::CreditEventStorageSet::newFrom(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}
	return d->m_newFrom;
}

void Isds::CreditEventStorageSet::setNewFrom(const QDate &nf)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_newFrom = nf;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventStorageSet::setNewFrom(QDate &&nf)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_newFrom = nf;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QDate &Isds::CreditEventStorageSet::newTo(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}
	return d->m_newTo;
}

void Isds::CreditEventStorageSet::setNewTo(const QDate &nt)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_newTo = nt;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventStorageSet::setNewTo(QDate &&nt)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_newTo = nt;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 Isds::CreditEventStorageSet::oldCapacity(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_oldCapacity;
}

void Isds::CreditEventStorageSet::setOldCapacity(qint64 oc)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_oldCapacity = (oc >= 0) ? oc : -1;
}

const QDate &Isds::CreditEventStorageSet::oldFrom(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}
	return d->m_oldFrom;
}

void Isds::CreditEventStorageSet::setOldFrom(const QDate &of)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_oldFrom = of;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventStorageSet::setOldFrom(QDate &&of)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_oldFrom = of;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QDate &Isds::CreditEventStorageSet::oldTo(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDate;
	}
	return d->m_oldTo;
}

void Isds::CreditEventStorageSet::setOldTo(const QDate &ot)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_oldTo = ot;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventStorageSet::setOldTo(QDate &&ot)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_oldTo = ot;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::CreditEventStorageSet::initiator(void) const
{
	Q_D(const CreditEventStorageSet);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_initiator;
}

void Isds::CreditEventStorageSet::setInitiator(const QString &i)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_initiator = i;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEventStorageSet::setInitiator(QString &&i)
{
	ensureCreditEventStorageSetPrivate();
	Q_D(CreditEventStorageSet);
	d->m_initiator = i;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(CreditEventStorageSet &first, CreditEventStorageSet &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief PIMPL CreditEvent class.
 */
class Isds::CreditEventPrivate {
	//Q_DISABLE_COPY(CreditEventPrivate)

public:
	CreditEventPrivate(void)
	    : m_time(), m_creditChange(0), m_creditAfter(0),
	    m_type(Type::CET_UNKNOWN), m_charged(), m_discharged(), m_msgSent(),
	    m_storageSet()
	{ }

	CreditEventPrivate &operator=(const CreditEventPrivate &other) Q_DECL_NOTHROW
	{
		m_time = other.m_time;
		m_creditChange = other.m_creditChange;
		m_creditAfter = other.m_creditAfter;
		m_type = other.m_type;
		m_charged = other.m_charged;
		m_discharged = other.m_discharged;
		m_msgSent = other.m_msgSent;
		m_storageSet = other.m_storageSet;

		return *this;
	}

	bool operator==(const CreditEventPrivate &other) const
	{
		return (m_time == other.m_time) &&
		    (m_creditChange == other.m_creditChange) &&
		    (m_creditAfter == other.m_creditAfter) &&
		    (m_type == other.m_type) &&
		    (m_charged == other.m_charged) &&
		    (m_discharged == other.m_discharged) &&
		    (m_msgSent == other.m_msgSent) &&
		    (m_storageSet == other.m_storageSet);
	}

	QDateTime m_time; /*!< Event time. */
	qint64 m_creditChange; /*!< Credit change (may be negative) in Heller. */
	qint64 m_creditAfter; /*!< Credit value in Heller after this event. */
	enum Type::CreditEventType m_type; /*!< Event type. */

	/* C++ does not allow classes with non-trivial constructors in union. */
	CreditEventCharged m_charged;
	CreditEventDischarged m_discharged;
	CreditEventMsgSent m_msgSent;
	CreditEventStorageSet m_storageSet;
};

Isds::CreditEvent::CreditEvent(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::CreditEvent::CreditEvent(const CreditEvent &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) CreditEventPrivate) : Q_NULLPTR)
{
	Q_D(CreditEvent);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEvent::CreditEvent(CreditEvent &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::CreditEvent::~CreditEvent(void)
{
}

/*!
 * @brief Ensures private storage set event presence.
 *
 * @note Returns if private storage set event could not be allocated.
 */
#define ensureCreditEventPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			CreditEventPrivate *p = new (std::nothrow) CreditEventPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::CreditEvent &Isds::CreditEvent::operator=(const CreditEvent &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureCreditEventPrivate(*this);
	Q_D(CreditEvent);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::CreditEvent &Isds::CreditEvent::operator=(CreditEvent &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::CreditEvent::operator==(const CreditEvent &other) const
{
	Q_D(const CreditEvent);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::CreditEvent::operator!=(const CreditEvent &other) const
{
	return !operator==(other);
}

bool Isds::CreditEvent::isNull(void) const
{
	Q_D(const CreditEvent);
	return d == Q_NULLPTR;
}

const QDateTime &Isds::CreditEvent::time(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDateTime;
	}
	return d->m_time;
}

void Isds::CreditEvent::setTime(const QDateTime &t)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	d->m_time = t;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEvent::setTime(QDateTime &&t)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	d->m_time = t;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 Isds::CreditEvent::creditChange(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return 0;
	}
	return d->m_creditChange;
}

void Isds::CreditEvent::setCreditChange(qint64 cc)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	d->m_creditChange = cc;
}

qint64 Isds::CreditEvent::creditAfter(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return 0;
	}
	return d->m_creditAfter;
}

void Isds::CreditEvent::setCreditAfter(qint64 ca)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	d->m_creditAfter = ca;
}

enum Isds::Type::CreditEventType Isds::CreditEvent::type(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::CET_UNKNOWN;
	}
	return d->m_type;
}

const Isds::CreditEventCharged &Isds::CreditEvent::charged(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullCreditEventCharged;
	}
	return d->m_charged;
}

/*!
 * @brief Erase private event content according to set type.
 *
 * @param[in] cep Private credit event.
 */
static
void eraseEventContent(Isds::CreditEventPrivate *cep)
{
	if (Q_UNLIKELY(cep == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	switch (cep->m_type) {
	case Isds::Type::CET_UNKNOWN:
		/* Nothing to do. */
		break;
	case Isds::Type::CET_CHARGED:
		cep->m_charged = nullCreditEventCharged;
		break;
	case Isds::Type::CET_DISCHARGED:
		cep->m_discharged = nullCreditEventDischarged;
		break;
	case Isds::Type::CET_MESSAGE_SENT:
		cep->m_msgSent = nullCreditEventMsgSent;
		break;
	case Isds::Type::CET_STORAGE_SET:
		cep->m_storageSet = nullCreditEventStorageSet;
		break;
	case Isds::Type::CET_EXPIRED:
		/* Nothing to do. */
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}

void Isds::CreditEvent::setCharged(const CreditEventCharged &cec)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_CHARGED) {
		eraseEventContent(d);
	}
	d->m_charged = cec;
	d->m_type = Type::CET_CHARGED;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEvent::setCharged(CreditEventCharged &&cec)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_CHARGED) {
		eraseEventContent(d);
	}
	d->m_charged = cec;
	d->m_type = Type::CET_CHARGED;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::CreditEventDischarged &Isds::CreditEvent::discharged(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullCreditEventDischarged;
	}
	return d->m_discharged;
}

void Isds::CreditEvent::setDischarged(const CreditEventDischarged &ced)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_DISCHARGED) {
		eraseEventContent(d);
	}
	d->m_discharged = ced;
	d->m_type = Type::CET_DISCHARGED;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEvent::setDischarged(CreditEventDischarged &&ced)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_DISCHARGED) {
		eraseEventContent(d);
	}
	d->m_discharged = ced;
	d->m_type = Type::CET_DISCHARGED;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::CreditEventMsgSent &Isds::CreditEvent::msgSent(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullCreditEventMsgSent;
	}
	return d->m_msgSent;
}

void Isds::CreditEvent::setMsgSent(const CreditEventMsgSent &cems)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_MESSAGE_SENT) {
		eraseEventContent(d);
	}
	d->m_msgSent = cems;
	d->m_type = Type::CET_MESSAGE_SENT;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEvent::setMsgSent(CreditEventMsgSent &&cems)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_MESSAGE_SENT) {
		eraseEventContent(d);
	}
	d->m_msgSent = cems;
	d->m_type = Type::CET_MESSAGE_SENT;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::CreditEventStorageSet &Isds::CreditEvent::storageSet(void) const
{
	Q_D(const CreditEvent);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullCreditEventStorageSet;
	}
	return d->m_storageSet;
}

void Isds::CreditEvent::setStorageSet(const CreditEventStorageSet &cess)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_STORAGE_SET) {
		eraseEventContent(d);
	}
	d->m_storageSet = cess;
	d->m_type = Type::CET_STORAGE_SET;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::CreditEvent::setStorageSet(CreditEventStorageSet &&cess)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_STORAGE_SET) {
		eraseEventContent(d);
	}
	d->m_storageSet = cess;
	d->m_type = Type::CET_STORAGE_SET;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::CreditEvent::setExpired(void)
{
	ensureCreditEventPrivate();
	Q_D(CreditEvent);
	if (d->m_type != Type::CET_EXPIRED) {
		eraseEventContent(d);
	}
	d->m_type = Type::CET_EXPIRED;
}

void Isds::swap(CreditEvent &first, CreditEvent &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
