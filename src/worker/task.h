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

#ifndef _TASK_H_
#define _TASK_H_

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
 * @brief Progress-bar labels used.
 */
#define PL_IDLE "Idle"
#define PL_DOWNLOAD_MESSAGE "DownloadMessage"
#define PL_DOWNLOAD_RECEIVED_LIST "DownloadReceivedMessageList"
#define PL_DOWNLOAD_SENT_LIST "DownloadSentMessageList"
#define PL_IMPORT_ZFO_DINFO "ImportZfoDeliveryInfo"
#define PL_IMPORT_ZFO_MSG "ImportZfoMessage"
#define PL_SEND_MESSAGE "SendMessage"

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

protected:
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

private:
//	/*!
//	 * Disable copy and assignment.
//	 */
//	Task(const Task &);
//	Task &operator=(const Task &);
};

#endif /* _TASK_H_ */
