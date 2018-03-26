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

#include "src/isds/box_management.h"
#include "src/isds/internal_conversion.h"

Isds::Address::Address(const Address &other)
    : m_adCity(other.m_adCity),
    m_adStreet(other.m_adStreet),
    m_adNumberInStreet(other.m_adNumberInStreet),
    m_adNumberInMunicipality(other.m_adNumberInMunicipality),
    m_adZipCode(other.m_adZipCode),
    m_adState(other.m_adState)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address::Address(Address &&other) Q_DECL_NOEXCEPT
    : m_adCity(std::move(other.m_adCity)),
    m_adStreet(std::move(other.m_adStreet)),
    m_adNumberInStreet(std::move(other.m_adNumberInStreet)),
    m_adNumberInMunicipality(std::move(other.m_adNumberInMunicipality)),
    m_adZipCode(std::move(other.m_adZipCode)),
    m_adState(std::move(other.m_adState))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Address &Isds::Address::operator=(const Address &other) Q_DECL_NOTHROW
{
	m_adCity = other.m_adCity;
	m_adStreet = other.m_adStreet;
	m_adNumberInStreet = other.m_adNumberInStreet;
	m_adNumberInMunicipality = other.m_adNumberInMunicipality;
	m_adZipCode = other.m_adZipCode;
	m_adState = other.m_adState;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address &Isds::Address::operator=(Address &&other) Q_DECL_NOTHROW
{
	std::swap(m_adCity, other.m_adCity);
	std::swap(m_adStreet, other.m_adStreet);
	std::swap(m_adNumberInStreet, other.m_adNumberInStreet);
	std::swap(m_adNumberInMunicipality, other.m_adNumberInMunicipality);
	std::swap(m_adZipCode, other.m_adZipCode);
	std::swap(m_adState, other.m_adState);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::BirthInfo::BirthInfo(const BirthInfo &other)
    : m_biDate(other.m_biDate),
    m_biCity(other.m_biCity),
    m_biCounty(other.m_biCounty),
    m_biState(other.m_biState)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo::BirthInfo(BirthInfo &&other) Q_DECL_NOEXCEPT
    : m_biDate(std::move(other.m_biDate)),
    m_biCity(std::move(other.m_biCity)),
    m_biCounty(std::move(other.m_biCounty)),
    m_biState(std::move(other.m_biState))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::BirthInfo &Isds::BirthInfo::operator=(const BirthInfo &other) Q_DECL_NOTHROW
{
	m_biDate = other.m_biDate;
	m_biCity = other.m_biCity;
	m_biCounty = other.m_biCounty;
	m_biState = other.m_biState;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo &Isds::BirthInfo::operator=(BirthInfo &&other) Q_DECL_NOTHROW
{
	std::swap(m_biDate, other.m_biDate);
	std::swap(m_biCity, other.m_biCity);
	std::swap(m_biCounty, other.m_biCounty);
	std::swap(m_biState, other.m_biState);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName::PersonName(const PersonName &other)
    : m_pnFirstName(other.m_pnFirstName),
    m_pnMiddleName(other.m_pnMiddleName),
    m_pnLastName(other.m_pnLastName),
    m_pnLastNameAtBirth(other.m_pnLastNameAtBirth)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName::PersonName(PersonName &&other) Q_DECL_NOEXCEPT
    : m_pnFirstName(std::move(other.m_pnFirstName)),
    m_pnMiddleName(std::move(other.m_pnMiddleName)),
    m_pnLastName(std::move(other.m_pnLastName)),
    m_pnLastNameAtBirth(std::move(other.m_pnLastNameAtBirth))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName &Isds::PersonName::operator=(const PersonName &other) Q_DECL_NOTHROW
{
	m_pnFirstName = other.m_pnFirstName;
	m_pnMiddleName = other.m_pnMiddleName;
	m_pnLastName = other.m_pnLastName;
	m_pnLastNameAtBirth = other.m_pnLastNameAtBirth;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName &Isds::PersonName::operator=(PersonName &&other) Q_DECL_NOTHROW
{
	std::swap(m_pnFirstName, other.m_pnFirstName);
	std::swap(m_pnMiddleName, other.m_pnMiddleName);
	std::swap(m_pnLastName, other.m_pnLastName);
	std::swap(m_pnLastNameAtBirth, other.m_pnLastNameAtBirth);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

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
Isds::Type::DbType libisdsDbType2DbType(isds_DbType *bt)
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
void dbType2libisdsDbType(isds_DbType **btPtr, Isds::Type::DbType bt)
{
	if (Q_UNLIKELY(btPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*btPtr == NULL) {
		*btPtr = (isds_DbType *)std::malloc(sizeof(*btPtr));
		if (Q_UNLIKELY(btPtr == NULL)) {
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
	personName.setFirstName(fromCStr(boi->personName->pnFirstName));
	personName.setMiddleName(fromCStr(boi->personName->pnMiddleName));
	personName.setLastName(fromCStr(boi->personName->pnLastName));
	personName.setLastNameAtBirth(fromCStr(boi->personName->pnLastNameAtBirth));
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

	toCStrCopy(&boi->personName->pnFirstName, pn.firstName());
	toCStrCopy(&boi->personName->pnMiddleName, pn.middleName());
	toCStrCopy(&boi->personName->pnLastName, pn.lastName());
	toCStrCopy(&boi->personName->pnLastNameAtBirth, pn.lastNameAtBirth());
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

//Address address(void) const;
//void setAddress(const Address &a);

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

//enum Type::DbState dbState(void) const;
//void setDbState(enum Type::DbState bs);

//enum Type::NilBool dbEffectiveOVM(void) const;
//void setDbEffectiveOVM(enum Type::NilBool eo);

//enum Type::NilBool dbOpenAddressing(void) const;
//void setDbOpenAddressing(enum Type::NilBool oa)
