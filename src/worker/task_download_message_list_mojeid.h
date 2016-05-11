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

#ifndef _TASK_DOWNLOAD_MESSAGE_LIST_MOJEID_H_
#define _TASK_DOWNLOAD_MESSAGE_LIST_MOJEID_H_

#include <QString>
#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing download message list.
 */
class TaskDownloadMessageListMojeID : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		DL_SUCCESS, /*!< Operation was successful. */
		DL_NET_ERROR, /*!< Error communicating with webdatovka. */
		DL_DB_INS_ERR, /*!< Error inserting into database. */
		DL_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName      Account identifier (user login name).
	 * @param[in,out] dbSet         Non-null pointer to database container.
	 * @param[in]     msgDirect     Received or sent list.
	 * @param[in]     downloadWhole True to plan downloading whole messages.
	 * @param[in]     dmLimit       Message list length limit.
	 * @param[in]     accountID      Webdatovka account ID.
	 * @param[in]     dmOffset       ISDS message offset, default 0.
	 */
	explicit TaskDownloadMessageListMojeID(const QString &userName,
	    MessageDbSet *dbSet, enum MessageDirection msgDirect,
	    bool downloadWhole, int dmLimit, int accountID, int dmOffset);

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void);

	/*!
	 * @brief Download message list from ISDS for given account.
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]     userName       Account identifier (user login name).
	 * @param[in]     msgDirect      Received or sent message.
	 * @param[in,out] dbSet          Database container.
	 * @param[in]     downloadWhole  True to plan downloading whole
	 *                               messages.
	 * @param[out]    error          Error description.
	 * @param[in]     progressLabel  Progress-bar label.
	 * @param[out]    total          Total number of messages.
	 * @param[out]    news           Number of new messages.
	 * @param[in]     dmLimit        Maximum number of message list;
	 *                               NULL if you don't care.
	 * @param[in]     accountID      Webdatovka account ID.
	 * @param[in]     dmOffset       ISDS message offset, default 0.
	 * @return Error state.
	 */
	static
	enum Result downloadMessageList(const QString &userName,
	    enum MessageDirection msgDirect, MessageDbSet &dbSet,
	    bool downloadWhole, QString &error,
	    const QString &progressLabel, int &total, int &news,
	    int dmLimit, int accountID, int dmOffset);

	enum Result m_result; /*!< Return state. */
	QString m_error;

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskDownloadMessageListMojeID(const TaskDownloadMessageListMojeID &);
	TaskDownloadMessageListMojeID &operator=(const TaskDownloadMessageListMojeID &);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	enum MessageDirection m_msgDirect; /*!< Sent or received list. */
	const bool m_downloadWhole; /*!< Plan downloading whole messages. */
	int m_dmLimit; /*!< List length limit. */
	int m_accountID; /*!< Webdatovka account ID. */
	int m_dmOffset; /*!< ISDS message offset, default 0 */
};

#endif /* _TASK_DOWNLOAD_MESSAGE_LIST_MOJEID_H_ */
