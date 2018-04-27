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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <cstdlib>
#include <cstring>
#include <isds.h>
#include <utility> /* std::move */

#include "src/isds/box_interface.h"
#include "src/isds/internal_conversion.h"

/* Null objects - for convenience. */
static const QDate nullDate;
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
 * @note Returns if private hash could not be allocated.
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
 * @note Returns if private hash could not be allocated.
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
 * @note Returns if private hash could not be allocated.
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

Isds::DbOwnerInfo::DbOwnerInfo(void)
    : m_dataPtr(NULL)
{
}

Isds::DbOwnerInfo::~DbOwnerInfo(void)
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return;
	}
	isds_DbOwnerInfo_free(&boi);
}

Isds::DbOwnerInfo::DbOwnerInfo(const DbOwnerInfo &other)
    : m_dataPtr(NULL)
{
	if (other.m_dataPtr != NULL) {
		m_dataPtr = isds_DbOwnerInfo_duplicate(
		    (const struct isds_DbOwnerInfo *)other.m_dataPtr);
		if (Q_UNLIKELY(m_dataPtr = NULL)) {
			Q_ASSERT(0);
		}
	}
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbOwnerInfo::DbOwnerInfo(DbOwnerInfo &&other) Q_DECL_NOEXCEPT
    : m_dataPtr(std::move(other.m_dataPtr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

QString Isds::DbOwnerInfo::dbID(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->dbID);
}

/*!
 * @brief Allocates libisds DbOwnerInfo structure.
 *
 * @param[in,out] dataPtr Pointer to pointer holding the structure.
 */
static
void intAllocMissingDbOwnerInfo(void **dataPtr)
{
	if (Q_UNLIKELY(dataPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*dataPtr != NULL) {
		/* Already allocated. */
		return;
	}
	struct isds_DbOwnerInfo *boi =
	    (struct isds_DbOwnerInfo *)std::malloc(sizeof(*boi));
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}
	std::memset(boi, 0, sizeof(*boi));
	*dataPtr = boi;
}

void Isds::DbOwnerInfo::setDbID(const QString &bi)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->dbID, bi);
}

/*!
 * @brief Converts data box types.
 */
static
enum Isds::Type::DbType libisdsDbType2DbType(const isds_DbType *bt)
{
	if (bt == NULL) {
		return Isds::Type::BT_NULL;
	}

	switch (*bt) {
	case DBTYPE_SYSTEM: return Isds::Type::BT_SYSTEM; break;
	case DBTYPE_OVM: return Isds::Type::BT_OVM; break;
	case DBTYPE_OVM_NOTAR: return Isds::Type::BT_OVM_NOTAR; break;
	case DBTYPE_OVM_EXEKUT: return Isds::Type::BT_OVM_EXEKUT; break;
	case DBTYPE_OVM_REQ: return Isds::Type::BT_OVM_REQ; break;
	case DBTYPE_OVM_FO: return Isds::Type::BT_OVM_FO; break;
	case DBTYPE_OVM_PFO: return Isds::Type::BT_OVM_PFO; break;
	case DBTYPE_OVM_PO: return Isds::Type::BT_OVM_PO; break;
	case DBTYPE_PO: return Isds::Type::BT_PO; break;
	case DBTYPE_PO_ZAK: return Isds::Type::BT_PO_ZAK; break;
	case DBTYPE_PO_REQ: return Isds::Type::BT_PO_REQ; break;
	case DBTYPE_PFO: return Isds::Type::BT_PFO; break;
	case DBTYPE_PFO_ADVOK: return Isds::Type::BT_PFO_ADVOK; break;
	case DBTYPE_PFO_DANPOR: return Isds::Type::BT_PFO_DANPOR; break;
	case DBTYPE_PFO_INSSPR: return Isds::Type::BT_PFO_INSSPR; break;
	case DBTYPE_PFO_AUDITOR: return Isds::Type::BT_PFO_AUDITOR; break;
	case DBTYPE_FO: return Isds::Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		return Isds::Type::BT_SYSTEM; /* FIXME */
		break;
	}
}

/*!
 * @brief Converts data box types.
 */
static
void dbType2libisdsDbType(isds_DbType **btPtr, enum Isds::Type::DbType bt)
{
	if (Q_UNLIKELY(btPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*btPtr == NULL) {
		*btPtr = (isds_DbType *)std::malloc(sizeof(**btPtr));
		if (Q_UNLIKELY(*btPtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	switch (bt) {
	/* case Isds::Type::BT_NULL: Same as default. */
	case Isds::Type::BT_SYSTEM: **btPtr = DBTYPE_SYSTEM; break;
	case Isds::Type::BT_OVM: **btPtr = DBTYPE_OVM; break;
	case Isds::Type::BT_OVM_NOTAR: **btPtr = DBTYPE_OVM_NOTAR; break;
	case Isds::Type::BT_OVM_EXEKUT: **btPtr = DBTYPE_OVM_EXEKUT; break;
	case Isds::Type::BT_OVM_REQ: **btPtr = DBTYPE_OVM_REQ; break;
	case Isds::Type::BT_OVM_FO: **btPtr = DBTYPE_OVM_FO; break;
	case Isds::Type::BT_OVM_PFO: **btPtr = DBTYPE_OVM_PFO; break;
	case Isds::Type::BT_OVM_PO: **btPtr = DBTYPE_OVM_PO; break;
	case Isds::Type::BT_PO: **btPtr = DBTYPE_PO; break;
	case Isds::Type::BT_PO_ZAK: **btPtr = DBTYPE_PO_ZAK; break;
	case Isds::Type::BT_PO_REQ: **btPtr = DBTYPE_PO_REQ; break;
	case Isds::Type::BT_PFO: **btPtr = DBTYPE_PFO; break;
	case Isds::Type::BT_PFO_ADVOK: **btPtr = DBTYPE_PFO_ADVOK; break;
	case Isds::Type::BT_PFO_DANPOR: **btPtr = DBTYPE_PFO_DANPOR; break;
	case Isds::Type::BT_PFO_INSSPR: **btPtr = DBTYPE_PFO_INSSPR; break;
	case Isds::Type::BT_PFO_AUDITOR: **btPtr = DBTYPE_PFO_AUDITOR; break;
	case Isds::Type::BT_FO: **btPtr = DBTYPE_FO; break;
	default:
		Q_ASSERT(0);
		std::free(*btPtr); *btPtr = NULL;
		break;
	}
}

enum Isds::Type::DbType Isds::DbOwnerInfo::dbType(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return Type::BT_NULL;
	}

	return libisdsDbType2DbType(boi->dbType);
}

void Isds::DbOwnerInfo::setDbType(enum Type::DbType bt)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	dbType2libisdsDbType(&boi->dbType, bt);
}

QString Isds::DbOwnerInfo::ic(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->ic);
}

void Isds::DbOwnerInfo::setIc(const QString &ic)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->ic, ic);
}

Isds::PersonName Isds::DbOwnerInfo::personName(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY((boi == NULL) || (boi->personName == NULL))) {
		return PersonName();
	}

	PersonName personName;
	setPersonNameContent(personName, boi->personName);
	return personName;
}

void Isds::DbOwnerInfo::setPersonName(const PersonName &pn)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (boi->personName == NULL) {
		boi->personName =
		    (struct isds_PersonName *)std::malloc(sizeof(*boi->personName));
		if (Q_UNLIKELY(boi->personName == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(boi->personName, 0, sizeof(*boi->personName));
	}

	setLibisdsPersonNameContent(boi->personName, pn);
}

QString Isds::DbOwnerInfo::firmName(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->firmName);
}

void Isds::DbOwnerInfo::setFirmName(const QString &fn)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->firmName, fn);
}

Isds::BirthInfo Isds::DbOwnerInfo::birthInfo(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY((boi == NULL) || (boi->birthInfo == NULL))) {
		return BirthInfo();
	}

	BirthInfo birthInfo;
	birthInfo.setDate(dateFromStructTM(boi->birthInfo->biDate));
	birthInfo.setCity(fromCStr(boi->birthInfo->biCity));
	birthInfo.setCounty(fromCStr(boi->birthInfo->biCounty));
	birthInfo.setState(fromCStr(boi->birthInfo->biState));
	return birthInfo;
}

void Isds::DbOwnerInfo::setBirthInfo(const BirthInfo &bi)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (boi->birthInfo == NULL) {
		boi->birthInfo =
		    (struct isds_BirthInfo *)std::malloc(sizeof(*boi->birthInfo));
		if (Q_UNLIKELY(boi->birthInfo == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(boi->birthInfo, 0, sizeof(*boi->birthInfo));
	}

	toCDateCopy(&boi->birthInfo->biDate, bi.date());
	toCStrCopy(&boi->birthInfo->biCity, bi.city());
	toCStrCopy(&boi->birthInfo->biCounty, bi.county());
	toCStrCopy(&boi->birthInfo->biState, bi.state());
}

Isds::Address Isds::DbOwnerInfo::address(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY((boi == NULL) || (boi->address == NULL))) {
		return Address();
	}

	Address address;
	setAddressContent(address, boi->address);
	return address;
}

void Isds::DbOwnerInfo::setAddress(const Address &a)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (boi->address == NULL) {
		boi->address =
		    (struct isds_Address *)std::malloc(sizeof(*boi->address));
		if (Q_UNLIKELY(boi->address == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(boi->address, 0, sizeof(*boi->address));
	}

	setLibisdsAddressContent(boi->address, a);
}

QString Isds::DbOwnerInfo::nationality(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->nationality);
}

void Isds::DbOwnerInfo::setNationality(const QString &n)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->nationality, n);
}

QString Isds::DbOwnerInfo::email(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->email);
}

void Isds::DbOwnerInfo::setEmail(const QString &e)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->email, e);
}

QString Isds::DbOwnerInfo::telNumber(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->telNumber);
}

void Isds::DbOwnerInfo::setTelNumber(const QString &tn)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->telNumber, tn);
}

QString Isds::DbOwnerInfo::identifier(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->identifier);
}

void Isds::DbOwnerInfo::setIdentifier(const QString &i) {
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->identifier, i);
}

QString Isds::DbOwnerInfo::registryCode(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return QString();
	}

	return fromCStr(boi->registryCode);
}

void Isds::DbOwnerInfo::setRegistryCode(const QString &rc)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&boi->registryCode, rc);
}

enum Isds::Type::DbState Isds::DbOwnerInfo::dbState(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY((boi == NULL) || (boi->dbState == NULL))) {
		return Type::BS_ERROR;
	}

	switch (*boi->dbState) {
	case 1: return Type::BS_ACCESSIBLE; break;
	case 2: return Type::BS_TEMP_INACCESSIBLE; break;
	case 3: return Type::BS_NOT_YET_ACCESSIBLE; break;
	case 4: return Type::BS_PERM_INACCESSIBLE; break;
	case 5: return Type::BS_REMOVED; break;
	case 6: return Type::BS_TEMP_UNACCESSIBLE_LAW; break;
	default:
		Q_ASSERT(0);
		return Type::BS_ERROR;
		break;
	}
}

void Isds::DbOwnerInfo::setDbState(enum Type::DbState bs)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (bs == Type::BS_ERROR) {
		if (boi->dbState != NULL) {
			std::free(boi->dbState); boi->dbState = NULL;
		}
	} else {
		toLongInt(&boi->dbState, bs);
	}
}

enum Isds::Type::NilBool Isds::DbOwnerInfo::dbEffectiveOVM(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(boi->dbEffectiveOVM);
}

void Isds::DbOwnerInfo::setDbEffectiveOVM(enum Type::NilBool eo)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&boi->dbEffectiveOVM, eo);
}

enum Isds::Type::NilBool Isds::DbOwnerInfo::dbOpenAddressing(void) const
{
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(boi->dbOpenAddressing);
}

void Isds::DbOwnerInfo::setDbOpenAddressing(enum Type::NilBool oa)
{
	intAllocMissingDbOwnerInfo(&m_dataPtr);
	struct isds_DbOwnerInfo *boi = (struct isds_DbOwnerInfo *)m_dataPtr;
	if (Q_UNLIKELY(boi == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&boi->dbOpenAddressing, oa);
}

Isds::DbOwnerInfo &Isds::DbOwnerInfo::operator=(const DbOwnerInfo &other) Q_DECL_NOTHROW
{
	if (m_dataPtr != NULL) {
		isds_DbOwnerInfo_free((struct isds_DbOwnerInfo **)&m_dataPtr);
	}

	if (other.m_dataPtr != NULL) {
		m_dataPtr = isds_DbOwnerInfo_duplicate(
		    (const struct isds_DbOwnerInfo *)other.m_dataPtr);
		if (Q_UNLIKELY(m_dataPtr = NULL)) {
			Q_ASSERT(0);
		}
	}
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbOwnerInfo &Isds::DbOwnerInfo::operator=(DbOwnerInfo &&other) Q_DECL_NOTHROW
{
	std::swap(m_dataPtr, other.m_dataPtr);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::DbUserInfo::DbUserInfo(void)
    : m_dataPtr(NULL)
{
}

Isds::DbUserInfo::~DbUserInfo(void)
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return;
	}
	isds_DbUserInfo_free(&bui);
}

Isds::DbUserInfo::DbUserInfo(const DbUserInfo &other)
    : m_dataPtr(NULL)
{
	if (other.m_dataPtr != NULL) {
		m_dataPtr = isds_DbUserInfo_duplicate(
		    (const struct isds_DbUserInfo *)other.m_dataPtr);
		if (Q_UNLIKELY(m_dataPtr = NULL)) {
			Q_ASSERT(0);
		}
	}
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbUserInfo::DbUserInfo(DbUserInfo &&other) Q_DECL_NOEXCEPT
    : m_dataPtr(std::move(other.m_dataPtr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName Isds::DbUserInfo::personName(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY((bui == NULL) || (bui->personName == NULL))) {
		return PersonName();
	}

	PersonName personName;
	setPersonNameContent(personName, bui->personName);
	return personName;
}

/*!
 * @brief Allocates libisds DbUserInfo structure.
 *
 * @param[in,out] dataPtr Pointer to pointer holding the structure.
 */
static
void intAllocMissingDbUserInfo(void **dataPtr)
{
	if (Q_UNLIKELY(dataPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*dataPtr != NULL) {
		/* Already allocated. */
		return;
	}
	struct isds_DbUserInfo *bui =
	    (struct isds_DbUserInfo *)std::malloc(sizeof(*bui));
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}
	std::memset(bui, 0, sizeof(*bui));
	*dataPtr = bui;
}

void Isds::DbUserInfo::setPersonName(const PersonName &pn)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (bui->personName == NULL) {
		bui->personName =
		    (struct isds_PersonName *)std::malloc(sizeof(*bui->personName));
		if (Q_UNLIKELY(bui->personName == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(bui->personName, 0, sizeof(*bui->personName));
	}

	setLibisdsPersonNameContent(bui->personName, pn);
}

Isds::Address Isds::DbUserInfo::address(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY((bui == NULL) || (bui->address == NULL))) {
		return Address();
	}

	Address address;
	setAddressContent(address, bui->address);
	return address;
}

void Isds::DbUserInfo::setAddress(const Address &a)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (bui->address == NULL) {
		bui->address =
		    (struct isds_Address *)std::malloc(sizeof(*bui->address));
		if (Q_UNLIKELY(bui->address == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(bui->address, 0, sizeof(*bui->address));
	}

	setLibisdsAddressContent(bui->address, a);
}

QDate Isds::DbUserInfo::biDate(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QDate();
	}

	return dateFromStructTM(bui->biDate);
}

void Isds::DbUserInfo::setBiDate(const QDate &d)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCDateCopy(&bui->biDate, d);
}

QString Isds::DbUserInfo::userID(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->userID);
}

void Isds::DbUserInfo::setUserId(const QString &uid)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->userID, uid);
}

/*!
 * @brief Converts user types.
 */
static
enum Isds::Type::UserType libisdsUserType2UserType(const isds_UserType *ut)
{
	if (ut == NULL) {
		return Isds::Type::UT_NULL;
	}

	switch (*ut) {
	case USERTYPE_PRIMARY: return Isds::Type::UT_PRIMARY; break;
	case USERTYPE_ENTRUSTED: return Isds::Type::UT_ENTRUSTED; break;
	case USERTYPE_ADMINISTRATOR: return Isds::Type::UT_ADMINISTRATOR; break;
	case USERTYPE_OFFICIAL: return Isds::Type::UT_OFFICIAL; break;
	case USERTYPE_OFFICIAL_CERT: return Isds::Type::UT_OFFICIAL_CERT; break;
	case USERTYPE_LIQUIDATOR: return Isds::Type::UT_LIQUIDATOR; break;
	case USERTYPE_RECEIVER: return Isds::Type::UT_RECEIVER; break;
	case USERTYPE_GUARDIAN: return Isds::Type::UT_GUARDIAN; break;
	default:
		Q_ASSERT(0);
		return Isds::Type::UT_PRIMARY; /* FIXME */
		break;
	}
}

/*!
 * @brief Converts user types.
 */
static
void userType2libisdsUserType(isds_UserType **utPtr,
    enum Isds::Type::UserType ut)
{
	if (Q_UNLIKELY(utPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*utPtr == NULL) {
		*utPtr = (isds_UserType *)std::malloc(sizeof(**utPtr));
		if (Q_UNLIKELY(*utPtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	switch (ut) {
	/* case Isds::Type::UT_NULL: Same as default. */
	case Isds::Type::UT_PRIMARY: **utPtr = USERTYPE_PRIMARY; break;
	case Isds::Type::UT_ENTRUSTED: **utPtr = USERTYPE_ENTRUSTED; break;
	case Isds::Type::UT_ADMINISTRATOR: **utPtr = USERTYPE_ADMINISTRATOR; break;
	case Isds::Type::UT_OFFICIAL: **utPtr = USERTYPE_OFFICIAL; break;
	case Isds::Type::UT_OFFICIAL_CERT: **utPtr = USERTYPE_OFFICIAL_CERT; break;
	case Isds::Type::UT_LIQUIDATOR: **utPtr = USERTYPE_LIQUIDATOR; break;
	case Isds::Type::UT_RECEIVER: **utPtr = USERTYPE_RECEIVER; break;
	case Isds::Type::UT_GUARDIAN: **utPtr = USERTYPE_GUARDIAN; break;
	default:
		Q_ASSERT(0);
		std::free(*utPtr); *utPtr = NULL;
		break;
	}
}

enum Isds::Type::UserType Isds::DbUserInfo::userType(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return Type::UT_NULL;
	}

	return libisdsUserType2UserType(bui->userType);
}

void Isds::DbUserInfo::setUserType(enum Type::UserType ut)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	userType2libisdsUserType(&bui->userType, ut);
}

Isds::Type::Privileges Isds::DbUserInfo::userPrivils(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY((bui == NULL) || (bui->userPrivils == NULL))) {
		return Type::PRIVIL_NONE;
	}

	const qint64 privNum = *bui->userPrivils;
	Type::Privileges privileges = Type::PRIVIL_NONE;
	if (privNum & Type::PRIVIL_READ_NON_PERSONAL) {
		privileges |= Type::PRIVIL_READ_NON_PERSONAL;
	}
	if (privNum & Type::PRIVIL_READ_ALL) {
		privileges |= Type::PRIVIL_READ_ALL;
	}
	if (privNum & Type::PRIVIL_CREATE_DM) {
		privileges |= Type::PRIVIL_CREATE_DM;
	}
	if (privNum & Type::PRIVIL_VIEW_INFO) {
		privileges |= Type::PRIVIL_VIEW_INFO;
	}
	if (privNum & Type::PRIVIL_SEARCH_DB) {
		privileges |= Type::PRIVIL_SEARCH_DB;
	}
	if (privNum & Type::PRIVIL_OWNER_ADM) {
		privileges |= Type::PRIVIL_OWNER_ADM;
	}
	if (privNum & Type::PRIVIL_READ_VAULT) {
		privileges |= Type::PRIVIL_READ_VAULT;
	}
	if (privNum & Type::PRIVIL_ERASE_VAULT) {
		privileges |= Type::PRIVIL_ERASE_VAULT;
	}
	return privileges;
}

void Isds::DbUserInfo::setUserPrivils(Type::Privileges p)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (bui->userPrivils == NULL) {
		bui->userPrivils =
		    (long int *)std::malloc(sizeof(*bui->userPrivils));
		if (Q_UNLIKELY(bui->userPrivils == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}

	*bui->userPrivils = p;
}

QString Isds::DbUserInfo::ic(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->ic);
}

void Isds::DbUserInfo::setIc(const QString &ic)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->ic, ic);
}

QString Isds::DbUserInfo::firmName(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->firmName);
}

void Isds::DbUserInfo::setFirmName(const QString &fn)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->firmName, fn);
}

QString Isds::DbUserInfo::caStreet(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->caStreet);
}

void Isds::DbUserInfo::setCaStreet(const QString &cs)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->caStreet, cs);
}

QString Isds::DbUserInfo::caCity(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->caCity);
}

void Isds::DbUserInfo::setCaCity(const QString &cc)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->caCity, cc);
}

QString Isds::DbUserInfo::caZipCode(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->caZipCode);
}

void Isds::DbUserInfo::setCaZipCode(const QString &cz)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->caZipCode, cz);
}

QString Isds::DbUserInfo::caState(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->caState);
}

void Isds::DbUserInfo::setCaState(const QString &cs)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->caState, cs);
}

QString Isds::DbUserInfo::aifoTicket(void) const
{
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		return QString();
	}

	return fromCStr(bui->aifo_ticket);
}

void Isds::DbUserInfo::setAifoTicket(const QString &at)
{
	intAllocMissingDbUserInfo(&m_dataPtr);
	struct isds_DbUserInfo *bui = (struct isds_DbUserInfo *)m_dataPtr;
	if (Q_UNLIKELY(bui == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&bui->aifo_ticket, at);
}

Isds::DbUserInfo &Isds::DbUserInfo::operator=(const DbUserInfo &other) Q_DECL_NOTHROW
{
	if (m_dataPtr != NULL) {
		isds_DbUserInfo_free((struct isds_DbUserInfo **)&m_dataPtr);
	}

	if (other.m_dataPtr != NULL) {
		m_dataPtr = isds_DbUserInfo_duplicate(
		    (const struct isds_DbUserInfo *)other.m_dataPtr);
		if (Q_UNLIKELY(m_dataPtr = NULL)) {
			Q_ASSERT(0);
		}
	}
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::DbUserInfo &Isds::DbUserInfo::operator=(DbUserInfo &&other) Q_DECL_NOTHROW
{
	std::swap(m_dataPtr, other.m_dataPtr);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */
