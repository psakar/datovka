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
#include "src/isds/internal_type_conversion.h"
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
enum Isds::Type::DbType libisdsDbTypePtr2DbType(const isds_DbType *bt,
    bool *ok = Q_NULLPTR)
{
	if (bt == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BT_NULL;
	}

	return IsdsInternal::libisdsDbType2DbType(*bt, ok);
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
	ownerInfo.setDbType(libisdsDbTypePtr2DbType(idoi->dbType, &iOk));
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
	ownerInfo.setDbEffectiveOVM(fromBoolPtr(idoi->dbEffectiveOVM));
	ownerInfo.setDbOpenAddressing(fromBoolPtr(idoi->dbOpenAddressing));

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
bool dbType2libisdsDbTypePtr(isds_DbType **tgt, enum Isds::Type::DbType src)
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

	bool ok = false;
	**tgt = IsdsInternal::dbType2libisdsDbType(src, &ok);
	if (Q_UNLIKELY(!ok)) {
		std::free(*tgt); *tgt = NULL;
		return false;
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
	if (Q_UNLIKELY(!dbType2libisdsDbTypePtr(&tgt->dbType, src.dbType()))) {
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
	if (Q_UNLIKELY(!toBoolPtr(&tgt->dbEffectiveOVM, src.dbEffectiveOVM()))) {
		return false;
	}
	if (Q_UNLIKELY(!toBoolPtr(&tgt->dbOpenAddressing, src.dbOpenAddressing()))) {
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

QList<Isds::DbOwnerInfo> Isds::libisds2dbOwnerInfoList(
    const struct isds_list *ioil, bool *ok)
{
	/* Owner info destructor function type. */
	typedef void (*own_info_destr_func_t)(struct isds_DbOwnerInfo **);

	QList<DbOwnerInfo> ownerInfoList;

	while (ioil != NULL) {
		const struct isds_DbOwnerInfo *ioi = (struct isds_DbOwnerInfo *)ioil->data;
		own_info_destr_func_t idestr = (own_info_destr_func_t)ioil->destructor;
		/* Destructor function must be set. */
		if (idestr != isds_DbOwnerInfo_free) {
			Q_ASSERT(0);
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList<DbOwnerInfo>();
		}

		if (ioi != NULL) {
			bool iOk = false;
			ownerInfoList.append(libisds2dbOwnerInfo(ioi, &iOk));
			if (!iOk) {
				if (ok != Q_NULLPTR) {
					*ok = false;
				}
				return QList<DbOwnerInfo>();
			}
		}

		ioil = ioil->next;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ownerInfoList;
}

struct isds_list *Isds::dbOwnerInfoList2libisds(
    const QList<DbOwnerInfo> &oil, bool *ok)
{
	if (Q_UNLIKELY(oil.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_list *ioil = NULL;
	struct isds_list *lastItem = NULL;
	foreach (const DbOwnerInfo &oi, oil) {
		struct isds_list *item =
		    (struct isds_list *)std::malloc(sizeof(*item));
		if (Q_UNLIKELY(item == NULL)) {
			Q_ASSERT(0);
			goto fail;
		}
		std::memset(item, 0, sizeof(*item));

		bool iOk = false;
		struct isds_DbOwnerInfo *ioi = dbOwnerInfo2libisds(oi, &iOk);
		if (Q_UNLIKELY(!iOk)) {
			std::free(item); item = NULL;
			goto fail;
		}

		/* Set list item. */
		item->next = NULL;
		item->data = ioi;
		item->destructor = (void (*)(void **))isds_DbOwnerInfo_free;

		/* Append item. */
		if (lastItem == NULL) {
			ioil = item;
		} else {
			lastItem->next = item;
		}
		lastItem = item;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ioil;

fail:
	isds_list_free(&ioil);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
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
Isds::Type::Privileges longPtr2Privileges(long int *ip)
{
	if (ip == NULL) {
		return Isds::Type::PRIVIL_NONE;
	}

	return Isds::long2Privileges((qint64)*ip);
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
	userInfo.setUserPrivils(longPtr2Privileges(idui->userPrivils));
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
	/*
	 * Isds::Type::UT_NULL cannot be reached here.
	 *
	 * case Isds::Type::UT_NULL:
	 * 	std::free(*tgt); *tgt = NULL;
	 * 	break;
	 */
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
bool privileges2longPtr(long int **tgt, Isds::Type::Privileges src)
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
	if (Q_UNLIKELY(!privileges2longPtr(&tgt->userPrivils, src.userPrivils()))) {
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

/*!
 * @brief Compute number of characters between pointers.
 *
 * @note This method is needed to adjust for UTF encoding.
 *
 * @param[in]  start Start of string.
 * @param[in]  stop End of string.
 * @param[out] ok Set to false on error, true on success.
 * @return Number of characters (not bytes) between the two pointers.
 */
static
int charactersBetweenPtrs(const char *start, const char *stop,
    bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(start > stop)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return -1;
	}

	ssize_t len = stop - start;
	if (len == 0) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return 0;
	}

	len = QString(QByteArray(start, len)).size();
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return len;
}

/*!
 * @brief Converts list of start/stop pointers into indexes into QString.
 *
 * @param[in]  str Start of C string.
 * @param[in]  starts List of start indexes.
 * @param[in]  stops List of stop indexes.
 * @param[out] ok Set to false on error, true on success.
 * @return List of start/stop index pairs.
 */
static
QList< QPair<int, int> > libisdsStartStop2startStop(const char *str,
    struct isds_list *starts, struct isds_list *stops, bool *ok = Q_NULLPTR)
{
	if ((str == NULL) || ((starts == NULL) && (stops == NULL))) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return QList< QPair<int, int> >();
	}

	if ((starts == NULL) || (stops == NULL)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return QList< QPair<int, int> >();
	}

	QList< QPair<int, int> > resList;

	while ((starts != NULL) && (stops != NULL)) {
		const char *start = (char *)starts->data;
		const char *stop = (char *)stops->data;

		if (Q_UNLIKELY((starts->destructor != NULL) || (stops->destructor != NULL))) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList< QPair<int, int> >();
		}

		qint64 diffStart;
		qint64 diffStop;
		/*
		 * UTF8 characters skew the positions, therefore simple pointer subtraction
		 * diffStart = start - str;
		 * diffStop = stop - str;
		 * canot be performed.
		 */
		diffStart = charactersBetweenPtrs(str, start);
		diffStop = charactersBetweenPtrs(str, stop);
		if (Q_UNLIKELY((diffStart < 0) || (diffStop < 0) || (diffStart > diffStop))) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList< QPair<int, int> >();
		}

		resList.append(QPair<int, int>(diffStart, diffStop));

		starts = starts->next;
		stops = stops->next;
	}

	/* Both lists must be equally long. */
	if (Q_UNLIKELY((starts != NULL) || (stops != NULL))) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return QList< QPair<int, int> >();
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return resList;
}

Isds::FulltextResult Isds::libisds2fulltextResult(
    const struct isds_fulltext_result *ifr, bool *ok)
{
	if (Q_UNLIKELY(ifr == Q_NULLPTR)) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return FulltextResult();
	}

	bool iOk = false;
	FulltextResult fulltextResult;

	fulltextResult.setDbID(fromCStr(ifr->dbID));
	fulltextResult.setDbType(IsdsInternal::libisdsDbType2DbType(ifr->dbType, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	fulltextResult.setDbName(fromCStr(ifr->name));
	fulltextResult.setDbAddress(fromCStr(ifr->address));
	fulltextResult.setDbBiDate(dateFromStructTM(ifr->biDate));
	fulltextResult.setIc(fromCStr(ifr->ic));
	fulltextResult.setDbEffectiveOVM(ifr->dbEffectiveOVM);
	fulltextResult.setActive(ifr->active);
	fulltextResult.setPublicSending(ifr->public_sending);
	fulltextResult.setCommercialSending(ifr->commercial_sending);

	/* Convert pointers to lists. */
	fulltextResult.setNameMatches(libisdsStartStop2startStop(ifr->name,
	    ifr->name_match_start, ifr->name_match_end, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	fulltextResult.setAddressMatches(libisdsStartStop2startStop(ifr->address,
	    ifr->address_match_start, ifr->address_match_end, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return fulltextResult;

fail:
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return FulltextResult();
}

QList<Isds::FulltextResult> Isds::libisds2fulltextResultList(
    const struct isds_list *ifrl, bool *ok)
{
	/* Full-text result destructor function type. */
	typedef void (*ft_result_destr_func_t)(struct isds_fulltext_result **);

	QList<FulltextResult> resultList;

	while (ifrl != NULL) {
		const struct isds_fulltext_result *ifr = (struct isds_fulltext_result *)ifrl->data;
		ft_result_destr_func_t idestr = (ft_result_destr_func_t)ifrl->destructor;
		/* Destructor function must be set. */
		if (idestr != isds_fulltext_result_free) {
			Q_ASSERT(0);
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList<FulltextResult>();
		}

		if (ifr != NULL) {
			bool iOk = false;
			resultList.append(libisds2fulltextResult(ifr, &iOk));
			if (!iOk) {
				if (ok != Q_NULLPTR) {
					*ok = false;
				}
				return QList<FulltextResult>();
			}
		}

		ifrl = ifrl->next;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return resultList;
}
