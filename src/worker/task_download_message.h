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

#ifndef _TASK_DOWNLOAD_MESSAGE_H_
#define _TASK_DOWNLOAD_MESSAGE_H_

#include <QDateTime>
#include <QString>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing download message.
 */
class TaskDownloadMessage : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		DM_SUCCESS, /*!< Operation was successful. */
		DM_ISDS_ERROR, /*!< Error communicating with ISDS. */
		DM_DB_INS_ERR, /*!< Error inserting into database. */
		DM_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName      Account identifier (user login name).
	 * @param[in,out] dbSet         Non-null pointer to database container.
	 * @param[in]     msgDirect     Received or sent list.
	 * @param[in]     dmId          Message identifier.
	 * @param[in]     dTime         Delivery time.
	 * @param[in]     listScheduled True if the task has been scheduled
	 *                              from TaskDownloadMessageList.
	 */
	explicit TaskDownloadMessage(const QString &userName,
	    MessageDbSet *dbSet, enum MessageDirection msgDirect,
	    qint64 dmId, const QDateTime &dTime, bool listScheduled);

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

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
	 * @param[out]    error     Error description.
	 * @param[out]    longError Long error description.
	 * @return Error state.
	 */
	static
	enum Result downloadDeliveryInfo(const QString &userName,
	    qint64 dmId, bool signedMsg, MessageDbSet &dbSet, QString &error,
	    QString &longError);

	/*!
	 * @brief Download whole message (envelope, attachments, raw).
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     userName      Account identifier (user login name).
	 * @param[in,out] mId           Message identifier.
	 * @param[in]     signedMsg     Whether to download signed message;
	 *                              must be true.
	 * @param[in]     msgDirect     Received or sent message.
	 * @param[in,out] dbSet         Database container.
	 * @param[out]    error         Error description.
	 * @param[out]    longError     Long error description.
	 * @param[in]     progressLabel Progress-bar label.
	 * @return Error state.
	 */
	static
	enum Result downloadMessage(const QString &userName,
	    MessageDb::MsgId &mId, bool signedMsg,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    QString &error, QString &longError, const QString &progressLabel);

	enum Result m_result; /*!< Return state. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

	/* Delivery time may change. */
	MessageDb::MsgId m_mId; /*!< Message identifier. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskDownloadMessage(const TaskDownloadMessage &);
	TaskDownloadMessage &operator=(const TaskDownloadMessage &);

	/*!
	 * @brief Download additional info about author (sender).
	 *
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in]     dmId      Message identifier.
	 * @param[in,out] messageDb Database.
	 * @param[out]    error     Error description.
	 * @param[out]    longError Long error description.
	 * @return Error state.
	 */
	static
	enum Result downloadMessageAuthor(const QString &userName, qint64 dmId,
	    MessageDb &messageDb, QString &error, QString &longError);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 *
	 * TODO -- Is there a way how to download the information about read
	 *     messages and apply it on the database?
	 *
	 * @param[in]  userName  Account identifier (user login name).
	 * @param[in]  dmId      Message identifier.
	 * @param[out] error     Error description.
	 * @param[out] longError Long error description.
	 * @return Error state.
	 */
	static
	enum Result markMessageAsDownloaded(const QString &userName,
	    qint64 dmId, QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	enum MessageDirection m_msgDirect; /*!< Sent or received message. */
	bool m_listScheduled; /*<
	                       * Whether the task has been scheduled from
	                       * download message list task.
	                       */
};

#endif /* _TASK_DOWNLOAD_MESSAGE_H_ */
