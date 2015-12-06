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

class TaskDownloadMessage : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName  Account identifier (user login name).
	 * @param[in,out] dbSet     Non-null pointer to database container.
	 * @param[in]     msgDirect Received or sent list.
	 * @param[in]     dmId      Message identifier.
	 * @param[in]     dTime     Delivery time.
	 */
	explicit TaskDownloadMessage(const QString &userName,
	    MessageDbSet *dbSet, enum MessageDirection msgDirect,
	    qint64 dmId, const QDateTime &dTime);

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void);

private:
	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	enum MessageDirection m_msgDirect; /*!< Sent or received message. */
	MessageDb::MsgId m_mId; /*!< Message identifier. */
};

#endif /* _TASK_DOWNLOAD_MESSAGE_H_ */
