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

#include "src/isds/error_conversion.h"
#include "src/isds/error.h"
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

/*!
 * @brief Converts sender type.
 */
static
enum Isds::Type::SenderType libisdsSenderType2SenderType(isds_sender_type st,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	enum Isds::Type::SenderType type = Isds::Type::ST_NULL;

	switch (st) {
	case SENDERTYPE_PRIMARY: type = Isds::Type::ST_PRIMARY; break;
	case SENDERTYPE_ENTRUSTED: type = Isds::Type::ST_ENTRUSTED; break;
	case SENDERTYPE_ADMINISTRATOR: type = Isds::Type::ST_ADMINISTRATOR; break;
	case SENDERTYPE_OFFICIAL: type = Isds::Type::ST_OFFICIAL; break;
	case SENDERTYPE_VIRTUAL: type = Isds::Type::ST_VIRTUAL; break;
	case SENDERTYPE_OFFICIAL_CERT: type = Isds::Type::ST_OFFICIAL_CERT; break;
	case SENDERTYPE_LIQUIDATOR: type = Isds::Type::ST_LIQUIDATOR; break;
	case SENDERTYPE_RECEIVER: type = Isds::Type::ST_RECEIVER; break;
	case SENDERTYPE_GUARDIAN: type = Isds::Type::ST_GUARDIAN; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

Isds::Error Isds::getMessageAuthor(struct isds_ctx *ctx, qint64 dmId,
    enum Type::SenderType &userType, QString &authorName)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || (dmId < 0))) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		return err;
	}

	isds_sender_type *sender_type = NULL;
	char *sender_name = NULL;

	isds_error ret = isds_get_message_sender(ctx,
	    QString::number(dmId).toUtf8().constData(),
	    &sender_type, NULL, &sender_name);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		return err;
	}

	if (sender_type != NULL) {
		userType = libisdsSenderType2SenderType(*sender_type);
		std::free(sender_type); sender_type = NULL;
	} else {
		userType = Type::ST_NULL;
	}

	if (sender_name != NULL) {
		authorName = sender_name;
		std::free(sender_name); sender_name = NULL;
	} else {
		authorName = QString();
	}

	err.setCode(Type::ERR_SUCCESS);
	return err;
}
