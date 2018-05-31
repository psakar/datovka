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

#include <QByteArray>
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
	class DbOwnerInfo;
	class DbUserInfo;
	class Error;
	class FulltextResult;
	class Hash;
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
	/* Box interface: */
		/*!
		 * @brief Service FindDataBox.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     criteria Search criteria.
		 * @param[out]    boxes Found boxes.
		 * @return Error description.
		 */
		static
		Error findDataBox(struct isds_ctx *ctx,
		    const DbOwnerInfo &criteria, QList<DbOwnerInfo> &boxes);

		/*!
		 * @brief Service GetOwnerInfoFromLogin.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[out]    ownerInfo Obtained owner info.
		 * @return Error description.
		 */
		static
		Error getOwnerInfoFromLogin(struct isds_ctx *ctx,
		    DbOwnerInfo &ownerInfo);

		/*!
		 * @brief Service GetUserInfoFromLogin.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[out]    userInfo Obtained user info.
		 * @return Error description.
		 */
		static
		Error getUserInfoFromLogin(struct isds_ctx *ctx,
		    DbUserInfo &userInfo);

		/*!
		 * @brief Service ISDSSearch2.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     soughtText Text to search for.
		 * @param[in]     soughtType Information search type to search in.
		 * @param[in]     soughtBoxType Type of box to search for.
		 *                              Value BT_SYSTEM means to search
		 *                              in all box types. Pass BT_NULL
		 *                              to let server to use default
		 *                              value which is BT_SYSTEM.
		 * @param[in]     pageSize Number of results in one page.
		 * @param[in]     pageNum Number of page.
		 * @param[in]     highlight Set to true to track sought
		 *                          expressions in received response.
		 *                          Pass BOOL_NULL to let the server decide.
		 * @param[out]    totalMatchingBoxes Number of found boxes.
		 * @param[out]    currentPagePosition Position of first entry
		 *                                    of the current page.
		 * @param[out]    currentPageSize     Size of current page.
		 * @param[out]    lastPage Set to true if last page acquired.
		 * @param[out]    boxes Found boxes.
		 * @return Error description.
		 */
		static
		Error isdsSearch2(struct isds_ctx *ctx, const QString &soughtText,
		    enum Type::FulltextSearchType soughtType,
		    enum Type::DbType soughtBoxType, quint64 pageSize,
		    quint64 pageNum, enum Type::NilBool highlight,
		    quint64 &totalMatchingBoxes, quint64 &currentPagePosition,
		    quint64 &currentPageSize, enum Type::NilBool &lastPage,
		    QList<FulltextResult> &boxes);

	/* Message interface: */
		/*!
		 * @brief Service AuthenticateMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     raw Raw message or delivery info content.
		 * @return Error description.
		 *    (ERR_SUCCESS - if data originate from ISDS;
		 *    ERR_NOTEQUAL - if data are unknown to ISDS)
		 */
		static
		Error authenticateMessage(struct isds_ctx *ctx,
		    const QByteArray &raw);

		/*!
		 * @brief Service CreateMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     message Sent message content.
		 * @param[out]    dmId Identifier of successfully sent message.
		 * @return Error description.
		 */
		static
		Error createMessage(struct isds_ctx *ctx,
		    const Message &message, qint64 &dmId);

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

		/*!
		 * @brief Service VerifyMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    hash Hash.
		 * @return Error description.
		 */
		static
		Error verifyMessage(struct isds_ctx *ctx, qint64 dmId,
		    Hash &hash);
	};

}
