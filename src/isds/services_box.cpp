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
#include "src/isds/box_interface.h"
#include "src/isds/error_conversion.h"
#include "src/isds/error.h"
#include "src/isds/internal_conversion.h"
#include "src/isds/internal_type_conversion.h"
#include "src/isds/services.h"

/*!
 * @brief Wraps the isds_long_message().
 *
 * @param[in] ctx LIbisds context.
 */
static inline
QString isdsLongMessage(const struct isds_ctx *ctx)
{
#ifdef WIN32
	/* The function returns strings in local encoding. */
	return QString::fromLocal8Bit(isds_long_message(ctx));
	/*
	 * TODO -- Is there a mechanism how to force the local encoding
	 * into libisds to be UTF-8?
	 */
#else /* !WIN32 */
	return QString::fromUtf8(isds_long_message(ctx));
#endif /* WIN32 */
}

Isds::Error Isds::Service::dataBoxCreditInfo(struct isds_ctx *ctx,
    const QString &dbID, const QDate &fromDate, const QDate &toDate,
    qint64 &currentCredit, QString &email, QList<CreditEvent> &history)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || dbID.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	bool ok = false;
	struct tm *iFromDate = NULL;
	struct tm *iToDate = NULL;
	isds_error ret = IE_SUCCESS;
	long int iCredit = 0;
	char *iEmail = NULL;
	struct isds_list *iHistory = NULL;

	if (Q_UNLIKELY((!toCDateCopy(&iFromDate, fromDate)) ||
	               (!toCDateCopy(&iToDate, toDate)))) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
		goto fail;
	}

	ret = isds_get_commercial_credit(ctx, dbID.toUtf8().constData(),
	    iFromDate, iToDate, &iCredit, &iEmail, &iHistory);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	ok = true;
	currentCredit = iCredit;
	email = (iEmail != NULL) ? QString(iEmail) : QString();
	history = (iHistory != NULL) ?
	    libisds2creditEventList(iHistory, &ok) : QList<CreditEvent>();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (iFromDate != NULL) {
		std::free(iFromDate);
	}
	if (iToDate != NULL) {
		std::free(iToDate);
	}
	if (iEmail != NULL) {
		std::free(iEmail);
	}
	if (iHistory != NULL) {
		isds_list_free(&iHistory);
	}

	return err;
}

Isds::Error Isds::Service::dummyOperation(struct isds_ctx *ctx)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	isds_error ret = isds_ping(ctx);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		return err;
	}

	err.setCode(Type::ERR_SUCCESS);
	return err;
}

Isds::Error Isds::Service::findDataBox(struct isds_ctx *ctx,
    const DbOwnerInfo &criteria, QList<DbOwnerInfo> &boxes)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || criteria.isNull())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	bool ok = false;
	isds_error ret = IE_SUCCESS;
	struct isds_DbOwnerInfo *iCrit = NULL;
	struct isds_list *iBoxes = NULL;

	iCrit = dbOwnerInfo2libisds(criteria, &ok);
	if (!ok) {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
		goto fail;
	}

	ret = isds_FindDataBox(ctx, iCrit, &iBoxes);
	if ((ret != IE_SUCCESS) && (ret != IE_2BIG)) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	ok = true;
	boxes = (iBoxes != NULL) ?
	    libisds2dbOwnerInfoList(iBoxes, &ok) : QList<DbOwnerInfo>();

	if (ok) {
		/* May also be ERR_2BIG. */
		err.setCode(libisds2Error(ret));
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (iCrit != NULL) {
		isds_DbOwnerInfo_free(&iCrit);
	}
	if (iBoxes != NULL) {
		isds_list_free(&iBoxes);
	}

	return err;
}

Isds::Error Isds::Service::getOwnerInfoFromLogin(struct isds_ctx *ctx,
    DbOwnerInfo &ownerInfo)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_DbOwnerInfo *oInfo = NULL;
	bool ok = true;

	isds_error ret = isds_GetOwnerInfoFromLogin(ctx, &oInfo);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	ownerInfo = (oInfo != NULL) ?
	    libisds2dbOwnerInfo(oInfo, &ok) : DbOwnerInfo();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (oInfo != NULL) {
		isds_DbOwnerInfo_free(&oInfo);
	}

	return err;
}

Isds::Error Isds::Service::getUserInfoFromLogin(struct isds_ctx *ctx,
    DbUserInfo &userInfo)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_DbUserInfo *uInfo = NULL;
	bool ok = true;

	isds_error ret = isds_GetUserInfoFromLogin(ctx, &uInfo);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	userInfo = (uInfo != NULL) ?
	    libisds2dbUserInfo(uInfo, &ok) : DbUserInfo();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (uInfo != NULL) {
		isds_DbUserInfo_free(&uInfo);
	}

	return err;
}

/*!
 * @brief Converts full-text search type.
 */
static
isds_fulltext_target fulltextSearchType2libisdsFulltextSearchType(
    enum Isds::Type::FulltextSearchType fst)
{
	switch (fst) {
	case Isds::Type::FST_GENERAL: return FULLTEXT_ALL; break;
	case Isds::Type::FST_ADDRESS: return FULLTEXT_ADDRESS; break;
	case Isds::Type::FST_IC: return FULLTEXT_IC; break;
	case Isds::Type::FST_BOX_ID: return FULLTEXT_BOX_ID; break;
	default:
		Q_ASSERT(0);
		return FULLTEXT_ALL;
		break;
	}
}

Isds::Error Isds::Service::isdsSearch2(struct isds_ctx *ctx,
    const QString &soughtText, enum Type::FulltextSearchType soughtType,
    enum Type::DbType soughtBoxType, quint64 pageSize, quint64 pageNum,
    enum Type::NilBool highlight, quint64 &totalMatchingBoxes,
    quint64 &currentPagePosition, quint64 &currentPageSize,
    enum Type::NilBool &lastPage, QList<FulltextResult> &boxes)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || soughtText.isEmpty())) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	unsigned long int *iTotalMatchingBoxes = NULL;
	unsigned long int *iCurrentPagePosition = NULL;
	unsigned long int *iCurrentPageSize = NULL;
	bool *iLastPage = NULL;
	struct isds_list *iBoxes = NULL;
	bool ok = true;

	isds_fulltext_target iSoughtType =
	    fulltextSearchType2libisdsFulltextSearchType(soughtType);
	isds_DbType iSoughtBoxType = DBTYPE_SYSTEM;
	const isds_DbType *iSoughtBoxTypePtr = NULL;
	if (soughtBoxType != Type::BT_NULL) {
		iSoughtBoxType =
		    IsdsInternal::dbType2libisdsDbType(soughtBoxType, &ok);
		if (!ok) {
			err.setCode(Type::ERR_ERROR);
			err.setLongDescr(tr("Error converting types."));
			return err;
		}
		iSoughtBoxTypePtr = &iSoughtBoxType;
	}
	unsigned long int iPageSize = pageSize;
	unsigned long int iPageNum = pageNum;
	bool iHighlight = false;
	const bool *iHighlightPtr = NULL;
	if (highlight != Type::BOOL_NULL) {
		iHighlight = (highlight == Type::BOOL_TRUE);
		iHighlightPtr = &iHighlight;
	}

	isds_error ret = isds_find_box_by_fulltext(ctx,
	    soughtText.toUtf8().constData(), &iSoughtType, iSoughtBoxTypePtr,
	    &iPageSize, &iPageNum, iHighlightPtr, &iTotalMatchingBoxes,
	    &iCurrentPagePosition, &iCurrentPageSize, &iLastPage, &iBoxes);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	totalMatchingBoxes = (iTotalMatchingBoxes != NULL) ? *iTotalMatchingBoxes : 0;
	currentPagePosition = (iCurrentPagePosition != NULL) ? *iCurrentPagePosition : 0;
	currentPageSize = (iCurrentPageSize != NULL) ? *iCurrentPageSize : 0;
	lastPage = (iLastPage != NULL) ? ((*iLastPage) ? Type::BOOL_TRUE : Type::BOOL_FALSE) : Type::BOOL_NULL;
	boxes = libisds2fulltextResultList(iBoxes, &ok);

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (iTotalMatchingBoxes != NULL) {
		std::free(iTotalMatchingBoxes);
	}
	if (iCurrentPagePosition != NULL) {
		std::free(iCurrentPagePosition);
	}
	if (iCurrentPageSize != NULL) {
		std::free(iCurrentPageSize);
	}
	if (iLastPage != NULL) {
		std::free(iLastPage);
	}
	if (iBoxes != NULL) {
		isds_list_free(&iBoxes);
	}

	return err;
}
