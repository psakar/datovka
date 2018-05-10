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

#include <QRunnable>
#include <QString>

#include "src/common.h" // qdatovka_error, enum MessageDirection
#include "src/io/message_db_set.h"

/*!
 * @brief Maximum length of message list to be downloaded.
 */
#define MESSAGE_LIST_LIMIT 100000

/*!
 * @brief Progress-bar labels used.
 */
#define PL_DOWNLOAD_MESSAGE "DownloadMessage"
#define PL_DOWNLOAD_RECEIVED_LIST "DownloadReceivedMessageList"
#define PL_DOWNLOAD_SENT_LIST "DownloadSentMessageList"
#define PL_GET_ACCOUNT_LIST "GetAccounts"
#define PL_IDLE "Idle"
#define PL_IMPORT_MSG "ImportMessage"
#define PL_IMPORT_ZFO_DINFO "ImportZfoDeliveryInfo"
#define PL_IMPORT_ZFO_MSG "ImportZfoMessage"
#define PL_SEND_MESSAGE "SendMessage"
#define PL_SPLIT_DB "DatabaseSplit"
#define PL_SYNC_ACCOUNT "SyncAccount"

/*!
 * @brief This class contains generic functions that can be used in derived
 *        classes.
 */
class Task : public QRunnable {
public:
	/*!
	 * @brief Describes accounts that should be processed.
	 */
	class AccountDescr {
	public:
		/*!
		 * @brief Constructors.
		 */
		AccountDescr(const QString &uN, class MessageDbSet *mDS)
		    : userName(uN), messageDbSet(mDS)
		{ }

		/*!
		 * @brief Checks whether contains valid data.
		 *
		 * @return False if invalid data held.
		 */
		bool isValid(void) const
		{
			return !userName.isEmpty() && (0 != messageDbSet);
		}

		QString userName; /*!< Account identifier (user login name). */
		class MessageDbSet *messageDbSet; /*!< Database set related to account. */
	};

	/*!
	 * @brief Method to be implemented in derived classes.
	 */
	virtual
	void run(void) = 0;

protected:

	/*!
	 * @brief Store message delivery info into database.
	 *
	 * @param[in] signedMsg Whether to store signed message must be true.
	 * @param[in,out] dbSet Database container.
	 * @param[in] message Message structure.
	 * @return Error state.
	 */
	static
	qdatovka_error storeDeliveryInfo(bool signedMsg, MessageDbSet &dbSet,
	    const Isds::Message &message);

	/*!
	 * @brief Store message envelope into database.
	 *
	 * @param[in] msgDirect Received or sent message.
	 * @param[in,out] dbSet Database container.
	 * @param[in] envelope  Message envelope structure.
	 * @return Error state.
	 */
	static
	qdatovka_error storeMessageEnvelope(enum MessageDirection msgDirect,
	    MessageDbSet &dbSet, const Isds::Envelope &envelope);

	/*!
	 * @brief Update message envelope in database.
	 *
	 * @param[in] msgDirect Received or sent message.
	 * @param[in,out] messageDb Database container.
	 * @param[in] envelope Message envelope structure.
	 * @return True on success.
	 */
	static
	qdatovka_error updateMessageEnvelope(enum MessageDirection msgDirect,
	    MessageDb &messageDb, const Isds::Envelope &envelope);

	/*!
	 * @brief Store complete message into database.
	 *
	 * @param[in] signedMsg Whether to store signed message must be true.
	 * @param[in] msgDirect Received or sent message.
	 * @param[in,out] dbSet Database container.
	 * @param[in] message   Message structure.
	 * @param[in] progressLabel Progress-bar label.
	 * @return Error state.
	 */
	static
	qdatovka_error storeMessage(bool signedMsg,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    const Isds::Message &message, const QString &progressLabel);

	/*!
	 * @brief Store attachments into database.
	 *
	 * @param[in,out] messageDb Database.
	 * @param[in] dmId Message identifier.
	 * @param[in] documents Attachment list.
	 * @return Error state.
	 */
	static
	qdatovka_error storeAttachments(MessageDb &messageDb, qint64 dmId,
	    const QList<Isds::Document> &documents);
};
