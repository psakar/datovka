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

#include <cstdlib> // malloc
#include <cstring> // memcpy
#include <isds.h>

#include "src/isds/box_conversion.h"
#include "src/isds/internal_conversion.h"

/*!
 * @brief Set address according to the libisds address structure.
 */
static
bool setAddressContent(Isds::Address &tgt, const struct isds_Address *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		return true;
	}

	tgt.setCity(Isds::fromCStr(src->adCity));
	//tgt.setDistrict(Isds::fromCStr(src->adDistrict));
	tgt.setStreet(Isds::fromCStr(src->adStreet));
	tgt.setNumberInStreet(Isds::fromCStr(src->adNumberInStreet));
	tgt.setNumberInMunicipality(Isds::fromCStr(src->adNumberInMunicipality));
	tgt.setZipCode(Isds::fromCStr(src->adZipCode));
	tgt.setState(Isds::fromCStr(src->adState));
	//tgt.setAmCode(Isds::fromLongInt(src->adCode));
	return true;
}

Isds::Address Isds::libisds2address(const struct isds_Address *ia, bool *ok)
{
	Address address;
	bool ret = setAddressContent(address, ia);
	if (ok != Q_NULLPTR) {
		*ok = ret;
	}
	return address;
}

/*!
 * @brief Set libisds address structure according to the address.
 */
static
bool setLibisdsAddressContent(struct isds_Address *tgt,
    const Isds::Address &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adCity, src.city()))) {
		return false;
	}
	//if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adDistrict, src.district()))) {
	//	return false;
	//}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adStreet, src.street()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adNumberInStreet, src.numberInStreet()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adNumberInMunicipality, src.numberInMunicipality()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adZipCode, src.zipCode()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->adState, src.state()))) {
		return false;
	}
	//if (Q_UNLIKELY(!Isds::toLongInt(&tgt->adCode, src.amCode()))) {
	//	return false;
	//}

	return true;
}

struct isds_Address *Isds::address2libisds(const Address &a, bool *ok)
{
	if (Q_UNLIKELY(a.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_Address *ia = (struct isds_Address *)std::malloc(sizeof(*ia));
	if (Q_UNLIKELY(ia == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ia, 0, sizeof(*ia));

	if (Q_UNLIKELY(!setLibisdsAddressContent(ia, a))) {
		isds_Address_free(&ia);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ia;
}

/*!
 * @brief Set birth info according to the libisds birth info structure.
 */
static
bool setBirthInfoContent(Isds::BirthInfo &tgt,
     const struct isds_BirthInfo *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		return true;
	}

	tgt.setDate(Isds::dateFromStructTM(src->biDate));
	tgt.setCity(Isds::fromCStr(src->biCity));
	tgt.setCounty(Isds::fromCStr(src->biCounty));
	tgt.setState(Isds::fromCStr(src->biState));
	return true;
}

Isds::BirthInfo Isds::libisds2birthInfo(const struct isds_BirthInfo *ibi,
    bool *ok)
{
	BirthInfo birthInfo;
	bool ret = setBirthInfoContent(birthInfo, ibi);
	if (ok != Q_NULLPTR) {
		*ok = ret;
	}
	return birthInfo;
}

/*!
 * @brief Set libisds birth info structure according to the birth info.
 */
static
bool setLibisdsBirthInfoContent(struct isds_BirthInfo *tgt,
    const Isds::BirthInfo &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	if (Q_UNLIKELY(!Isds::toCDateCopy(&tgt->biDate, src.date()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->biCity, src.city()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->biCounty, src.county()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->biState, src.state()))) {
		return false;
	}

	return true;
}

struct isds_BirthInfo *Isds::birthInfo2libisds(const BirthInfo &bi, bool *ok)
{
	if (Q_UNLIKELY(bi.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_BirthInfo *ibi =
	    (struct isds_BirthInfo *)std::malloc(sizeof(*ibi));
	if (Q_UNLIKELY(ibi == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ibi, 0, sizeof(*ibi));

	if (Q_UNLIKELY(!setLibisdsBirthInfoContent(ibi, bi))) {
		isds_BirthInfo_free(&ibi);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ibi;
}

/*!
 * @brief Set person name according to the libisds person name structure.
 */
static
bool setPersonNameContent(Isds::PersonName &tgt,
    const struct isds_PersonName *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		return true;
	}

	tgt.setFirstName(Isds::fromCStr(src->pnFirstName));
	tgt.setMiddleName(Isds::fromCStr(src->pnMiddleName));
	tgt.setLastName(Isds::fromCStr(src->pnLastName));
	tgt.setLastNameAtBirth(Isds::fromCStr(src->pnLastNameAtBirth));
	return true;
}

Isds::PersonName Isds::libisds2personName(const struct isds_PersonName *ipn,
    bool *ok)
{
	PersonName personName;
	bool ret = setPersonNameContent(personName, ipn);
	if (ok != Q_NULLPTR) {
		*ok = ret;
	}
	return personName;
}

/*!
 * @brief Set libisds person name structure according to the person name.
 */
static
bool setLibisdsPersonNameContent(struct isds_PersonName *tgt,
    const Isds::PersonName &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->pnFirstName, src.firstName()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->pnMiddleName, src.middleName()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->pnLastName, src.lastName()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->pnLastNameAtBirth, src.lastNameAtBirth()))) {
		return false;
	}

	return true;
}

struct isds_PersonName *Isds::personName2libisds(const PersonName &pn, bool *ok)
{
	if (Q_UNLIKELY(pn.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_PersonName *ipn =
	    (struct isds_PersonName *)std::malloc(sizeof(*ipn));
	if (Q_UNLIKELY(ipn == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ipn, 0, sizeof(*ipn));

	if (Q_UNLIKELY(!setLibisdsPersonNameContent(ipn, pn))) {
		isds_PersonName_free(&ipn);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ipn;
}
