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
#include "src/isds/message_conversion.h"
#include "src/isds/message_interface.h"
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
 * @brief Converts message filter state.
 */
static
int dmFiltState2libisdsMessageStatus(Isds::Type::DmFiltStates fs)
{
	return (int)fs;
}

Isds::Error Isds::Service::getListOfReceivedMessages(struct isds_ctx *ctx,
    Type::DmFiltStates dmStatusFilter, unsigned long int dmOffset,
    unsigned long int *dmLimit, QList<Message> &messages)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_list *msgList = NULL;
	bool ok = true;

	isds_error ret = isds_get_list_of_received_messages(ctx, NULL, NULL, NULL,
	    dmFiltState2libisdsMessageStatus(dmStatusFilter), dmOffset, dmLimit,
	    &msgList);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	messages = (msgList != NULL) ?
	    libisds2messageList(msgList, &ok) : QList<Message>();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (msgList != NULL) {
		isds_list_free(&msgList);
	}

	return err;
}

Isds::Error Isds::Service::getListOfSentMessages(struct isds_ctx *ctx,
    Type::DmFiltStates dmStatusFilter, unsigned long int dmOffset,
    unsigned long int *dmLimit, QList<Message> &messages)
{
	Error err;

	if (Q_UNLIKELY(ctx == NULL)) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_list *msgList = NULL;
	bool ok = true;

	isds_error ret = isds_get_list_of_sent_messages(ctx, NULL, NULL, NULL,
	    dmFiltState2libisdsMessageStatus(dmStatusFilter), dmOffset, dmLimit,
	    &msgList);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	messages = (msgList != NULL) ?
	    libisds2messageList(msgList, &ok) : QList<Message>();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (msgList != NULL) {
		isds_list_free(&msgList);
	}

	return err;
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

Isds::Error Isds::Service::getMessageAuthor(struct isds_ctx *ctx, qint64 dmId,
    enum Type::SenderType &userType, QString &authorName)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || (dmId < 0))) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	isds_sender_type *s_type = NULL;
	char *s_name = NULL;
	bool ok = true;

	isds_error ret = isds_get_message_sender(ctx,
	    QString::number(dmId).toUtf8().constData(), &s_type, NULL, &s_name);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	userType = (s_type != NULL) ?
	    libisdsSenderType2SenderType(*s_type, &ok) : Type::ST_NULL;
	authorName = (s_name != NULL) ? QString(s_name) : QString();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (s_type != NULL) {
		std::free(s_type); s_type = NULL;
	}
	if (s_name != NULL) {
		std::free(s_name); s_name = NULL;
	}

	return err;
}

Isds::Error Isds::Service::getSignedDeliveryInfo(struct isds_ctx *ctx,
    qint64 dmId, Message &message)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || (dmId < 0))) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_message *msg = NULL;
	bool ok = true;

	isds_error ret = isds_get_signed_delivery_info(ctx,
	    QString::number(dmId).toUtf8().constData(), &msg);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	message = (msg != NULL) ? libisds2message(msg, &ok) : Message();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (msg != NULL) {
		isds_message_free(&msg);
	}

	return err;
}

Isds::Error Isds::Service::SignedReceivedMessageDownload(struct isds_ctx *ctx,
    qint64 dmId, Message &message)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || (dmId < 0))) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_message *msg = NULL;
	bool ok = true;

	isds_error ret = isds_get_signed_received_message(ctx,
	    QString::number(dmId).toUtf8().constData(), &msg);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	message = (msg != NULL) ? libisds2message(msg, &ok) : Message();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (msg != NULL) {
		isds_message_free(&msg);
	}

	return err;
}

Isds::Error Isds::Service::SignedSentMessageDownload(struct isds_ctx *ctx,
    qint64 dmId, Message &message)
{
	Error err;

	if (Q_UNLIKELY((ctx == NULL) || (dmId < 0))) {
		Q_ASSERT(0);
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Insufficient input."));
		return err;
	}

	struct isds_message *msg = NULL;
	bool ok = true;

	isds_error ret = isds_get_signed_sent_message(ctx,
	    QString::number(dmId).toUtf8().constData(), &msg);
	if (ret != IE_SUCCESS) {
		err.setCode(libisds2Error(ret));
		err.setLongDescr(isdsLongMessage(ctx));
		goto fail;
	}

	message = (msg != NULL) ? libisds2message(msg, &ok) : Message();

	if (ok) {
		err.setCode(Type::ERR_SUCCESS);
	} else {
		err.setCode(Type::ERR_ERROR);
		err.setLongDescr(tr("Error converting types."));
	}

fail:
	if (msg != NULL) {
		isds_message_free(&msg);
	}

	return err;
}
