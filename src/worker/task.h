/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#ifndef _TASK_MESSAGE_GENERAL_H_
#define _TASK_MESSAGE_GENERAL_H_

#include <QRunnable>
#include <QString>

#include "src/common.h" // qdatovka_error, enum MessageDirection
#include "src/io/message_db.h" // MessageDb::MsgId
#include "src/io/message_db_set.h"

/*!
 * @brief Maximum length of message list to be downloaded.
 */
#define MESSAGE_LIST_LIMIT 100000

/*!
 * @brief This class contains generic functions that can be used in derived
 *     classes.
 */
class Task : public QRunnable {
public:
	/*!
	 * @brief Method to be implemented in derived classes.
	 */
	virtual
	void run(void) = 0;

	/*!
	 * @brief Download delivery info for message.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in]     dmId      Message identifier.
	 * @param[in]     signedMsg Whether to download signed data;
	 *                          must be true.
	 * @param[in,out] dbSet     Database container.
	 * @return Error State.
	 */
	static
	qdatovka_error downloadDeliveryInfo(const QString &userName,
	    qint64 dmId, bool signedMsg, MessageDbSet &dbSet);

	/*!
	 * @brief Download whole message (envelope, attachments, raw).
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     userName      Account identifier (user login name).
	 * @param[in]     mId           Message identifier.
	 * @param[in]     signedMsg     Whether to download signed message;
	 *                              must be true.
	 * @param[in]     msgDirect     Received or sent message.
	 * @param[in,out] dbSet         Database container.
	 * @param[out]    errMsg        Error message.
	 * @param[in]     progressLabel Progress-bar label.
	 * @return Error state.
	 */
	static
	qdatovka_error downloadMessage(const QString &userName,
	    MessageDb::MsgId mId, bool signedMsg,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    QString &errMsg, const QString &progressLabel);

	/*!
	 * @brief Download message list from ISDS for given account.
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     userName       Account identifier (user login name).
	 * @param[in]     msgDirect      Received or sent message.
	 * @param[in,out] dbSet          Database container.
	 * @param[out]    errMsg         Error message.
	 * @param[in]     progressLabel  Progress-bar label.
	 * @param[out]    total          Total number of messages.
	 * @param[out]    news           Number of new messages.
	 * @param[out]    newMsgIdList   Identifiers of new messages.
	 * @param[in]     dmLimit        Maximum number of message list;
	 *                               NULL if you don't care.
	 * @param[in]     dmStatusFilter Libisds status filter.
	 * @return Error state.
	 */
	static
	qdatovka_error downloadMessageList(const QString &userName,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    QString &errMsg, const QString &progressLabel, int &total,
	    int &news, QStringList &newMsgIdList, ulong *dmLimit,
	    int dmStatusFilter);

	/*!
	 * @brief Gives more detailed information about sending outcome.
	 */
	class MsgSendingResult {
	public:
		/*!
		 * @brief Constructor.
		 */
		MsgSendingResult(void)
		    : sendStatus(IE_ERROR), dbIDRecipient(), recipientName(),
		    dmId(-1), isPDZ(false), errInfo()
		{ }

		isds_error sendStatus; /*!< Status as returned by libisds. */
		QString dbIDRecipient; /*!< Recipient identifier. */
		QString recipientName; /*!< Recipient name. */
		qint64 dmId; /*!< Sent message identifier. */
		bool isPDZ; /*!< True if message was sent as PDZ. */
		QString errInfo; /*!< Error description. */
	};

	/*!
	 * @brief Sends a single message to ISDS fro given account.
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     userName         Account identifier (user login name).
	 * @param[in,out] dbSet            Database container.
	 * @param[in,out] message          Message being sent.
	 * @param[in]     recipientName    Message recipient name.
	 * @param[in]     recipientAddress Message recipient address.
	 * @param[in]     isPDZ            True if message is a PDZ.
	 * @param[out]    result           Results, pass NULL if not desired.
	 * @return Error state.
	 */
	static
	qdatovka_error sendMessage(const QString &userName,
	    MessageDbSet &dbSet, struct isds_message *message,
	    const QString &recipientName, const QString &recipientAddress,
	    bool isPDZ, MsgSendingResult *result);

	/*!
	 * @brief Store message delivery information into database.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]     signedMsg Whether to store signed message;
	 *                          must be true.
	 * @param[in,out] dbSet     Database container.
	 * @param[in]     msg       Message.
	 * @return Error state.
	 */
	static
	qdatovka_error storeDeliveryInfo(bool signedMsg, MessageDbSet &dbSet,
	    const struct isds_message *msg);

	/*!
	 * @brief Store envelope into database.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]     msgDirect Received or sent message.
	 * @param[in,out] dbSet     Database container.
	 * @param[in]     envel     Message envelope.
	 * @return Error state.
	 */
	static
	qdatovka_error storeEnvelope(enum MessageDirection msgDirect,
	    MessageDbSet &dbSet, const struct isds_envelope *envel);

	/*!
	 * @brief Store message into database.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]     signedMsg     Whether to store signed message;
	 *                              must be true.
	 * @param[in]     msgDirect     Received or sent message.
	 * @param[in,out] dbSet         Database container.
	 * @param[in]     msg           Message.
	 * @param[in]     progressLabel Progress-bar label.
	 * @return Error state.
	 */
	static
	qdatovka_error storeMessage(bool signedMsg,
	    enum MessageDirection msgDirect,
	    MessageDbSet &dbSet, const struct isds_message *msg,
	    const QString &progressLabel);

private:
	/*!
	 * @brief Store attachments into database.
	 *
	 * @param[in,out] messageDb Database.
	 * @param[in]     dmId      Message identifier.
	 * @param[in]     documents Attachments.
	 * @return Error state.
	 */
	static
	qdatovka_error storeAttachments(MessageDb &messageDb, qint64 dmId,
	    const struct isds_list *documents);

	/*!
	 * @brief Download additional info about author (sender).
	 *
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in]     dmId      Message identifier.
	 * @param[in,out] messageDb Database.
	 * @return True on success.
	 */
	static
	bool downloadMessageAuthor(const QString &userName, qint64 dmId,
	    MessageDb &messageDb);

	/*!
	 * @brief Download sent message delivery info and get list of events.
	 *
	 * @param[in]     msgDirect Received or sent message.
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in]     dmId      Message identifier.
	 * @param[in]     signedMsg Whether to store signed message;
	 *                          must be true.
	 * @param[in,out] dbSet     Database container.
	 * @return True on success.
	 */
	static
	bool downloadMessageState(enum MessageDirection msgDirect,
	    const QString &userName, qint64 dmId, bool signedMsg,
	    MessageDbSet &dbSet);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 *
	 * TODO -- Is there a way how to download the information about read
	 *     messages and apply it on the database?
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] dmId     Message identifier.
	 * @return True on success.
	 */
	static
	bool markMessageAsDownloaded(const QString &userName, qint64 dmId);

	/*!
	 * @brief Update message envelope.
	 *
	 * @param[in]     msgDirect Received or sent message.
	 * @param[in,out] messageDb Database.
	 * @param[in]     envel     Message envelope.
	 * @return True on success.
	 */
	static
	qdatovka_error updateEnvelope(enum MessageDirection msgDirect,
	    MessageDb &messageDb, const struct isds_envelope *envel);

	/*!
	 * @brief Update message information according to supplied envelope.
	 *
	 * @param[in]     msgDirect Received or sent message.
	 * @param[in,out] dbSet     Database container.
	 * @param[in]     envel     Message envelope.
	 * @return True on success.
	 */
	static
	qdatovka_error updateMessageState(enum MessageDirection msgDirect,
	    MessageDbSet &dbSet, const struct isds_envelope *envel);
};

#endif /* _TASK_MESSAGE_GENERAL_H_ */
