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
#include "src/isds/type_conversion.h"

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

/*!
 * @brief Converts data box types.
 */
static
enum Isds::Type::DbType libisdsDbType2DbType(const isds_DbType *bt,
    bool *ok = Q_NULLPTR)
{
	if (bt == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BT_NULL;
	}

	bool iOk = true;
	enum Isds::Type::DbType type = Isds::Type::BT_NULL;

	switch (*bt) {
	case DBTYPE_SYSTEM: type = Isds::Type::BT_SYSTEM; break;
	case DBTYPE_OVM: type = Isds::Type::BT_OVM; break;
	case DBTYPE_OVM_NOTAR: type = Isds::Type::BT_OVM_NOTAR; break;
	case DBTYPE_OVM_EXEKUT: type = Isds::Type::BT_OVM_EXEKUT; break;
	case DBTYPE_OVM_REQ: type = Isds::Type::BT_OVM_REQ; break;
	case DBTYPE_OVM_FO: type = Isds::Type::BT_OVM_FO; break;
	case DBTYPE_OVM_PFO: type = Isds::Type::BT_OVM_PFO; break;
	case DBTYPE_OVM_PO: type = Isds::Type::BT_OVM_PO; break;
	case DBTYPE_PO: type = Isds::Type::BT_PO; break;
	case DBTYPE_PO_ZAK: type = Isds::Type::BT_PO_ZAK; break;
	case DBTYPE_PO_REQ: type = Isds::Type::BT_PO_REQ; break;
	case DBTYPE_PFO: type = Isds::Type::BT_PFO; break;
	case DBTYPE_PFO_ADVOK: type = Isds::Type::BT_PFO_ADVOK; break;
	case DBTYPE_PFO_DANPOR: type = Isds::Type::BT_PFO_DANPOR; break;
	case DBTYPE_PFO_INSSPR: type = Isds::Type::BT_PFO_INSSPR; break;
	case DBTYPE_PFO_AUDITOR: type = Isds::Type::BT_PFO_AUDITOR; break;
	case DBTYPE_FO: type = Isds::Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		type = Isds::Type::BT_SYSTEM; /* FIXME */
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

/*!
 * @brief Converts data box accessibility state.
 */
static
enum Isds::Type::DbState longPtr2DbState(const long int *bs,
    bool *ok = Q_NULLPTR)
{
	if (bs == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BS_ERROR;
	}

	return Isds::long2DbState(*bs, ok);
}

Isds::DbOwnerInfo Isds::libisds2dbOwnerInfo(const struct isds_DbOwnerInfo *idoi,
    bool *ok)
{
	if (Q_UNLIKELY(idoi == Q_NULLPTR)) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return DbOwnerInfo();
	}

	bool iOk = false;
	DbOwnerInfo ownerInfo;

	ownerInfo.setDbID(Isds::fromCStr(idoi->dbID));
	ownerInfo.setDbType(libisdsDbType2DbType(idoi->dbType, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	ownerInfo.setIc(Isds::fromCStr(idoi->ic));
	ownerInfo.setPersonName(libisds2personName(idoi->personName, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	ownerInfo.setFirmName(Isds::fromCStr(idoi->firmName));
	ownerInfo.setBirthInfo(libisds2birthInfo(idoi->birthInfo, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	ownerInfo.setAddress(libisds2address(idoi->address, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	ownerInfo.setNationality(Isds::fromCStr(idoi->nationality));
	ownerInfo.setEmail(Isds::fromCStr(idoi->email));
	ownerInfo.setTelNumber(Isds::fromCStr(idoi->telNumber));
	ownerInfo.setIdentifier(Isds::fromCStr(idoi->identifier));
	ownerInfo.setRegistryCode(Isds::fromCStr(idoi->registryCode));
	ownerInfo.setDbState(longPtr2DbState(idoi->dbState, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	ownerInfo.setDbEffectiveOVM(fromBool(idoi->dbEffectiveOVM));
	ownerInfo.setDbOpenAddressing(fromBool(idoi->dbOpenAddressing));

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ownerInfo;

fail:
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return DbOwnerInfo();
}

/*!
 * @brief Converts data box types.
 */
static
bool dbType2libisdsDbType(isds_DbType **tgt, enum Isds::Type::DbType src)
{
	if (Q_UNLIKELY(tgt == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (src == Isds::Type::BT_NULL) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return true;
	}
	if (*tgt == NULL) {
		*tgt = (isds_DbType *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	switch (src) {
	case Isds::Type::BT_NULL:
		std::free(*tgt); *tgt = NULL;
		break;
	case Isds::Type::BT_SYSTEM: **tgt = DBTYPE_SYSTEM; break;
	case Isds::Type::BT_OVM: **tgt = DBTYPE_OVM; break;
	case Isds::Type::BT_OVM_NOTAR: **tgt = DBTYPE_OVM_NOTAR; break;
	case Isds::Type::BT_OVM_EXEKUT: **tgt = DBTYPE_OVM_EXEKUT; break;
	case Isds::Type::BT_OVM_REQ: **tgt = DBTYPE_OVM_REQ; break;
	case Isds::Type::BT_OVM_FO: **tgt = DBTYPE_OVM_FO; break;
	case Isds::Type::BT_OVM_PFO: **tgt = DBTYPE_OVM_PFO; break;
	case Isds::Type::BT_OVM_PO: **tgt = DBTYPE_OVM_PO; break;
	case Isds::Type::BT_PO: **tgt = DBTYPE_PO; break;
	case Isds::Type::BT_PO_ZAK: **tgt = DBTYPE_PO_ZAK; break;
	case Isds::Type::BT_PO_REQ: **tgt = DBTYPE_PO_REQ; break;
	case Isds::Type::BT_PFO: **tgt = DBTYPE_PFO; break;
	case Isds::Type::BT_PFO_ADVOK: **tgt = DBTYPE_PFO_ADVOK; break;
	case Isds::Type::BT_PFO_DANPOR: **tgt = DBTYPE_PFO_DANPOR; break;
	case Isds::Type::BT_PFO_INSSPR: **tgt = DBTYPE_PFO_INSSPR; break;
	case Isds::Type::BT_PFO_AUDITOR: **tgt = DBTYPE_PFO_AUDITOR; break;
	case Isds::Type::BT_FO: **tgt = DBTYPE_FO; break;
	default:
		Q_ASSERT(0);
		std::free(*tgt); *tgt = NULL;
		return false;
		break;
	}

	return true;
}

/*!
 * @brief Converts data box accessibility state.
 */
static
bool dbState2longPtr(long int **tgt, enum Isds::Type::DbState src)
{
	if (Q_UNLIKELY(tgt == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (*tgt == NULL) {
		*tgt = (long int *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(tgt == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	switch (src) {
	case Isds::Type::BS_ERROR:
		std::free(*tgt); *tgt = NULL;
		break;
	default:
		**tgt = src;
		break;
	}

	return true;
}

/*!
 * @brief Set libisds box owner info structure according to the owner info.
 */
static
bool setLibisdsDbOwnerInfoContent(struct isds_DbOwnerInfo *tgt,
    const Isds::DbOwnerInfo &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	bool iOk = false;

	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->dbID, src.dbID()))) {
		return false;
	}
	if (Q_UNLIKELY(!dbType2libisdsDbType(&tgt->dbType, src.dbType()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->ic, src.ic()))) {
		return false;
	}
	tgt->personName = Isds::personName2libisds(src.personName(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->firmName, src.firmName()))) {
		return false;
	}
	tgt->birthInfo = Isds::birthInfo2libisds(src.birthInfo(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	tgt->address = Isds::address2libisds(src.address(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->nationality, src.nationality()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->email, src.email()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->telNumber, src.telNumber()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->identifier, src.identifier()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->registryCode, src.registryCode()))) {
		return false;
	}
	if (Q_UNLIKELY(!dbState2longPtr(&tgt->dbState, src.dbState()))) {
		return false;
	}
	if (Q_UNLIKELY(!toBool(&tgt->dbEffectiveOVM, src.dbEffectiveOVM()))) {
		return false;
	}
	if (Q_UNLIKELY(!toBool(&tgt->dbOpenAddressing, src.dbOpenAddressing()))) {
		return false;
	}

	return true;
}

struct isds_DbOwnerInfo *Isds::dbOwnerInfo2libisds(const DbOwnerInfo &doi,
    bool *ok)
{
	if (Q_UNLIKELY(doi.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_DbOwnerInfo *idoi =
	    (struct isds_DbOwnerInfo *)std::malloc(sizeof(*idoi));
	if (Q_UNLIKELY(idoi == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(idoi, 0, sizeof(*idoi));

	if (Q_UNLIKELY(!setLibisdsDbOwnerInfoContent(idoi, doi))) {
		isds_DbOwnerInfo_free(&idoi);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return idoi;
}

/*!
 * @brief Converts user types.
 */
static
enum Isds::Type::UserType libisdsUserType2UserType(const isds_UserType *ut,
    bool *ok = Q_NULLPTR)
{
	if (ut == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::UT_NULL;
	}

	bool iOk = true;
	enum Isds::Type::UserType type = Isds::Type::UT_NULL;

	switch (*ut) {
	case USERTYPE_PRIMARY: type = Isds::Type::UT_PRIMARY; break;
	case USERTYPE_ENTRUSTED: type = Isds::Type::UT_ENTRUSTED; break;
	case USERTYPE_ADMINISTRATOR: type = Isds::Type::UT_ADMINISTRATOR; break;
	case USERTYPE_OFFICIAL: type = Isds::Type::UT_OFFICIAL; break;
	case USERTYPE_OFFICIAL_CERT: type = Isds::Type::UT_OFFICIAL_CERT; break;
	case USERTYPE_LIQUIDATOR: type = Isds::Type::UT_LIQUIDATOR; break;
	case USERTYPE_RECEIVER: type = Isds::Type::UT_RECEIVER; break;
	case USERTYPE_GUARDIAN: type = Isds::Type::UT_GUARDIAN; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		type = Isds::Type::UT_NULL;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

/*!
 * @brief Converts privileges.
 */
static
Isds::Type::Privileges long2Privileges(long int *ip)
{
	if (ip == NULL) {
		return Isds::Type::PRIVIL_NONE;
	}

	const qint64 privNum = *ip;
	Isds::Type::Privileges privileges = Isds::Type::PRIVIL_NONE;
	if (privNum & Isds::Type::PRIVIL_READ_NON_PERSONAL) {
		privileges |= Isds::Type::PRIVIL_READ_NON_PERSONAL;
	}
	if (privNum & Isds::Type::PRIVIL_READ_ALL) {
		privileges |= Isds::Type::PRIVIL_READ_ALL;
	}
	if (privNum & Isds::Type::PRIVIL_CREATE_DM) {
		privileges |= Isds::Type::PRIVIL_CREATE_DM;
	}
	if (privNum & Isds::Type::PRIVIL_VIEW_INFO) {
		privileges |= Isds::Type::PRIVIL_VIEW_INFO;
	}
	if (privNum & Isds::Type::PRIVIL_SEARCH_DB) {
		privileges |= Isds::Type::PRIVIL_SEARCH_DB;
	}
	if (privNum & Isds::Type::PRIVIL_OWNER_ADM) {
		privileges |= Isds::Type::PRIVIL_OWNER_ADM;
	}
	if (privNum & Isds::Type::PRIVIL_READ_VAULT) {
		privileges |= Isds::Type::PRIVIL_READ_VAULT;
	}
	if (privNum & Isds::Type::PRIVIL_ERASE_VAULT) {
		privileges |= Isds::Type::PRIVIL_ERASE_VAULT;
	}
	return privileges;
}

Isds::DbUserInfo Isds::libisds2dbUserInfo(const struct isds_DbUserInfo *idui,
    bool *ok)
{
	if (Q_UNLIKELY(idui == Q_NULLPTR)) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return DbUserInfo();
	}

	bool iOk = false;
	DbUserInfo userInfo;

	userInfo.setPersonName(libisds2personName(idui->personName, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	userInfo.setAddress(libisds2address(idui->address, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	userInfo.setBiDate(dateFromStructTM(idui->biDate));
	userInfo.setUserId(fromCStr(idui->userID));
	userInfo.setUserType(libisdsUserType2UserType(idui->userType, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	userInfo.setUserPrivils(long2Privileges(idui->userPrivils));
	userInfo.setIc(fromCStr(idui->ic));
	userInfo.setFirmName(fromCStr(idui->firmName));
	userInfo.setCaStreet(fromCStr(idui->caStreet));
	userInfo.setCaCity(fromCStr(idui->caCity));
	userInfo.setCaZipCode(fromCStr(idui->caZipCode));
	userInfo.setCaState(fromCStr(idui->caState));

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return userInfo;

fail:
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return DbUserInfo();
}

/*!
 * @brief Converts user types.
 */
static
bool userType2libisdsUserType(isds_UserType **tgt,
    enum Isds::Type::UserType src)
{
	if (Q_UNLIKELY(tgt == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (src == Isds::Type::UT_NULL) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return true;
	}
	if (*tgt == NULL) {
		*tgt = (isds_UserType *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	switch (src) {
	case Isds::Type::UT_NULL:
		std::free(*tgt); *tgt = NULL;
		break;
	case Isds::Type::UT_PRIMARY: **tgt = USERTYPE_PRIMARY; break;
	case Isds::Type::UT_ENTRUSTED: **tgt = USERTYPE_ENTRUSTED; break;
	case Isds::Type::UT_ADMINISTRATOR: **tgt = USERTYPE_ADMINISTRATOR; break;
	case Isds::Type::UT_OFFICIAL: **tgt = USERTYPE_OFFICIAL; break;
	case Isds::Type::UT_OFFICIAL_CERT: **tgt = USERTYPE_OFFICIAL_CERT; break;
	case Isds::Type::UT_LIQUIDATOR: **tgt = USERTYPE_LIQUIDATOR; break;
	case Isds::Type::UT_RECEIVER: **tgt = USERTYPE_RECEIVER; break;
	case Isds::Type::UT_GUARDIAN: **tgt = USERTYPE_GUARDIAN; break;
	default:
		Q_ASSERT(0);
		std::free(*tgt); *tgt = NULL;
		return false;
		break;
	}

	return true;
}

/*!
 * @brief Converts privileges.
 */
static
bool privileges2long(long int **tgt, Isds::Type::Privileges src)
{
	if (Q_UNLIKELY(tgt == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (*tgt == NULL) {
		*tgt = (long int *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(tgt == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}

	**tgt = src;
	return true;
}

/*!
 * @brief Set libisds box user info structure according to the user info.
 */
static
bool setLibisdsDbUserInfoContent(struct isds_DbUserInfo *tgt,
    const Isds::DbUserInfo &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	bool iOk = false;

	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->userID, src.userID()))) {
		return false;
	}
	if (Q_UNLIKELY(!userType2libisdsUserType(&tgt->userType, src.userType()))) {
		return false;
	}
	if (Q_UNLIKELY(!privileges2long(&tgt->userPrivils, src.userPrivils()))) {
		return false;
	}
	tgt->personName = personName2libisds(src.personName(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	tgt->address = address2libisds(src.address(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCDateCopy(&tgt->biDate, src.biDate()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->ic, src.ic()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->firmName, src.firmName()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->caStreet, src.caStreet()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->caCity, src.caCity()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->caZipCode, src.caZipCode()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->caState, src.caState()))) {
		return false;
	}

	return true;
}

struct isds_DbUserInfo *Isds::dbUserInfo2libisds(const DbUserInfo &dui,
    bool *ok)
{
	if (Q_UNLIKELY(dui.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_DbUserInfo *idui =
	    (struct isds_DbUserInfo *)std::malloc(sizeof(*idui));
	if (Q_UNLIKELY(idui == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(idui, 0, sizeof(*idui));

	if (Q_UNLIKELY(!setLibisdsDbUserInfoContent(idui, dui))) {
		isds_DbUserInfo_free(&idui);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return idui;
}
