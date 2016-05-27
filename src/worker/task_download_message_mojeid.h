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

#ifndef _TASK_DOWNLOAD_MESSAGE_MOJEID_H_
#define _TASK_DOWNLOAD_MESSAGE_MOJEID_H_

#include <QDateTime>
#include <QString>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing download message.
 */
class TaskDownloadMessageMojeId : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		DM_SUCCESS, /*!< Operation was successful. */
		DM_NET_ERROR, /*!< Error communicating with webdatovka. */
		DM_DB_INS_ERR, /*!< Error inserting into database. */
		DM_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName      Account identifier (user login name).
	 * @param[in,out] dbSet         Non-null pointer to database container.
	 * @param[in]     msgDirect     Received or sent list.
	 * @param[in]     id            Message webdatovka identifier.
	 * @param[in]     dmId          Message isds identifier.
	 * @param[in]     listScheduled True if the task has been scheduled
	 *                              from TaskDownloadMessageList.
	 */
	explicit TaskDownloadMessageMojeId(const QString &userName,
	    MessageDbSet *dbSet, enum MessageDirection msgDirect, int id,
	    qint64 dmId, bool listScheduled);

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void);

	/*!
	 * @brief Download whole message (envelope, attachments, raw).
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     id            Message identifier.
	 * @param[in]     msgDirect     Received or sent message.
	 * @param[in,out] dbSet         Database container.
	 * @param[out]    error         Error description.
	 * @param[in]     progressLabel Progress-bar label.
	 * @return Error state.
	 */
	static
	enum Result downloadMessage(int id,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    QString &error, const QString &progressLabel);

	enum Result m_result; /*!< Return state. */
	QString m_error; /*!< Error description. */
	int m_id; /*!< Message webdatovka identifier. */


private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskDownloadMessageMojeId(const TaskDownloadMessageMojeId &);
	TaskDownloadMessageMojeId &operator=(const TaskDownloadMessageMojeId &);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	enum MessageDirection m_msgDirect; /*!< Sent or received message. */
	qint64 m_dmId; /*!< Message isds ID. */
	bool m_listScheduled; /*<
	                       * Whether the task has been scheduled from
	                       * download message list task.
	                       */
};

#endif /* _TASK_DOWNLOAD_MESSAGE_MOJEID_H_ */
