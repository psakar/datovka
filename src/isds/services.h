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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QList>
#include <QString>

#include "src/isds/types.h"

extern "C" {
	/* TODO -- The context structure needs to be encapsulated. */
	struct isds_ctx;
}

namespace Isds {

	/* Forward declaration. */
	class Error;
	class Message;

	/*!
	 * @brief Encapsulates ISDS services.
	 */
	class Service {
		Q_DECLARE_TR_FUNCTIONS(Service)

	private:
		/*!
		 * @brief Private constructor.
		 */
		Service(void);

	public:
		/*!
		 * @brief Service GetListOfReceivedMessages.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmStatusFilter Status filter, MFS_ANY for all.
		 * @param[in]     dmOffset Sequence number of first requested record.
		 * @param[in,out] dmLimit Message list length limit.
		 * @param[out]    messages Message list.
		 * @return Error description.
		 */
		static
		Error getListOfReceivedMessages(struct isds_ctx *ctx,
		    Type::DmFiltStates dmStatusFilter, unsigned long int dmOffset,
		    unsigned long int *dmLimit, QList<Message> &messages);

		/*!
		 * @brief Service GetListOfSentMessages.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmStatusFilter Status filter, MFS_ANY for all.
		 * @param[in]     dmOffset Sequence number of first requested record.
		 * @param[in,out] dmLimit Message list length limit.
		 * @param[out]    messages Message list.
		 * @return Error description.
		 */
		static
		Error getListOfSentMessages(struct isds_ctx *ctx,
		    Type::DmFiltStates dmStatusFilter, unsigned long int dmOffset,
		    unsigned long int *dmLimit, QList<Message> &messages);

		/*!
		 * @brief Service GetMessageAuthor.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    userType Message sender type.
		 * @param[out]    authorName Message sender name.
		 * @return Error description.
		 */
		static
		Error getMessageAuthor(struct isds_ctx *ctx, qint64 dmId,
		    enum Type::SenderType &userType, QString &authorName);

		/*!
		 * @brief Service GetSignedDeliveryInfo.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    message Signed delivery info.
		 * @return Error description.
		 */
		static
		Error getSignedDeliveryInfo(struct isds_ctx *ctx, qint64 dmId,
		    Message &message);

		/*!
		 * @brief Service MarkMessageAsDownloaded.
		 */
		static
		Error markMessageAsDownloaded(struct isds_ctx *ctx,
		    qint64 dmId);

		/*!
		 * @brief Service SignedMessageDownload.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    message Message.
		 * @return Error description.
		 */
		static
		Error signedReceivedMessageDownload(struct isds_ctx *ctx,
		    qint64 dmId, Message &message);

		/*!
		 * @brief Service SignedSentMessageDownload.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    message Message.
		 * @return Error description.
		 */
		static
		Error signedSentMessageDownload(struct isds_ctx *ctx,
		    qint64 dmId, Message &message);
	};

}
