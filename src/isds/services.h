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
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>

#include "src/datovka_shared/isds/types.h"

namespace Isds {

	/* Forward declaration. */
	class CreditEvent;
	class DbOwnerInfo;
	class DbUserInfo;
	class Error;
	class FulltextResult;
	class Hash;
	class Message;
	class Otp;
	class Session;

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
	/* Account interface: */
		/*!
		 * @brief Service ChangeISDSPassword.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     oldPwd Password currently in use.
		 * @param[in]     newPwd New password.
		 * @param[on,out] otp Auxiliary data required if one-time
		 *                    password authentication is in use. Passes
		 *                    OTP code (if known) and returns fine grade
		 *                    resolution of OTP procedure. Pass null,
		 *                    if one-time password authentication isn't
		 *                    needed. Please note this argument must
		 *                    match the OTP method used at log-in time.
		 *                    See login function for more detail.
		 * @param[out]    refNum Serial number of request assigned by ISDS.
		 * @return Error description.
		 */
		static
		Error changeISDSPassword(Session *ctx, const QString &oldPwd,
		    const QString &newPwd, Otp &otp, QString &refNum);

		/*!
		 * @brief Service GetPasswordInfo.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[out]    pswExpDate Password expiration date, null
		 *                           value if password does not exist.
		 * @return Error description.
		 */
		static
		Error getPasswordInfo(Session *ctx, QDateTime &pswExpDate);

	/* Box interface: */
		/*!
		 * @brief Service DataBoxCreditInfo.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     fromDate First day of credit history,
		 *                         pass null value if you don't care.
		 * @param[in]     toDate Last day of credit history,
		 *                       pass null value if you don't care.
		 * @param[out]    currentCredit Current credit in Heller.
		 * @param[out]    email Box-related notification email.
		 * @param[out]    history Credit events in given interval.
		 * @return Error description.
		 */
		static
		Error dataBoxCreditInfo(Session *ctx, const QString &dbID,
		    const QDate &fromDate, const QDate &toDate,
		    qint64 &currentCredit, QString &email,
		    QList<CreditEvent> &history);

		/*!
		 * @brief Service DummyOperation.
		 *
		 * @param[in,out] ctx Communication context.
		 * @return Error description.
		 */
		static
		Error dummyOperation(Session *ctx);

		/*!
		 * @brief Service FindDataBox.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     criteria Search criteria.
		 * @param[out]    boxes Found boxes.
		 * @return Error description.
		 */
		static
		Error findDataBox(Session *ctx, const DbOwnerInfo &criteria,
		    QList<DbOwnerInfo> &boxes);

		/*!
		 * @brief Service GetOwnerInfoFromLogin.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[out]    ownerInfo Obtained owner info.
		 * @return Error description.
		 */
		static
		Error getOwnerInfoFromLogin(Session *ctx,
		    DbOwnerInfo &ownerInfo);

		/*!
		 * @brief Service GetUserInfoFromLogin.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[out]    userInfo Obtained user info.
		 * @return Error description.
		 */
		static
		Error getUserInfoFromLogin(Session *ctx, DbUserInfo &userInfo);

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
		Error isdsSearch2(Session *ctx, const QString &soughtText,
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
		Error authenticateMessage(Session *ctx, const QByteArray &raw);

		/*!
		 * @brief Service CreateMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     message Sent message content.
		 * @param[out]    dmId Identifier of successfully sent message.
		 * @return Error description.
		 */
		static
		Error createMessage(Session *ctx, const Message &message,
		    qint64 &dmId);

		/*!
		 * @brief Service EraseMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Identifier of message in state 10.
		 * @param[in]     dmIncoming True for received message.
		 * @return Error description.
		 */
		static
		Error eraseMessage(Session *ctx, qint64 dmId, bool dmIncoming);

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
		Error getListOfReceivedMessages(Session *ctx,
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
		Error getListOfSentMessages(Session *ctx,
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
		Error getMessageAuthor(Session *ctx, qint64 dmId,
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
		Error getSignedDeliveryInfo(Session *ctx, qint64 dmId,
		    Message &message);

		/*!
		 * @brief Service MarkMessageAsDownloaded.
		 */
		static
		Error markMessageAsDownloaded(Session *ctx, qint64 dmId);

		/*!
		 * @brief Service SignedMessageDownload.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    message Message.
		 * @return Error description.
		 */
		static
		Error signedReceivedMessageDownload(Session *ctx, qint64 dmId,
		    Message &message);

		/*!
		 * @brief Service SignedSentMessageDownload.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    message Message.
		 * @return Error description.
		 */
		static
		Error signedSentMessageDownload(Session *ctx, qint64 dmId,
		    Message &message);

		/*!
		 * @brief Service VerifyMessage.
		 *
		 * @param[in,out] ctx Communication context.
		 * @param[in]     dmId Message identifier.
		 * @param[out]    hash Hash.
		 * @return Error description.
		 */
		static
		Error verifyMessage(Session *ctx, qint64 dmId, Hash &hash);
	};

}
