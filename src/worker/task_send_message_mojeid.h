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

#ifndef _TASK_SEND_MESSAGE_MOJEID_H_
#define _TASK_SEND_MESSAGE_MOJEID_H_

#include <QList>
#include <QString>

#include "src/worker/task.h"
#include "src/web/json.h"

/*!
 * @brief Task describing sending message.
 */
class TaskSendMessageMojeId : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		SM_SUCCESS, /*!< Operation was successful. */
		SM_NET_ERROR, /*!< Error communicating with webdatovka. */
		SM_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]      accountID       Account ID of webdatovka.
	 * @param[in]      recipientList   List of recipients.
	 * @param[in]      envelope        Envelope data.
	 * @param[in]      fileList        List of attachments.
	 */
	explicit TaskSendMessageMojeId(const QString &userName,
	    int accountID, const QList<JsonLayer::Recipient> &recipientList,
	    const JsonLayer::Envelope &envelope,
	    const QList<JsonLayer::File> &fileList);

	/*!
	 * @brief Performs actual message sending.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSendMessageMojeId(const TaskSendMessageMojeId &);
	TaskSendMessageMojeId &operator=(const TaskSendMessageMojeId &);

	/*!
	 * @brief Sends a single message to ISDS fro given account.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]      accountID       Account ID of webdatovka.
	 * @param[in]      recipientList   List of recipients.
	 * @param[in]      envelope        Envelope data.
	 * @param[in]      fileList        List of attachments.
	 * @param[in]      progressLabel   Progress-bar label.
	 * @param[out]     resultList      ResultList.
	 * @param[out]     error           Error message.
	 * @return Error state.
	 */
	static
	enum Result sendMessage(const QString &userName,
	    int accountID, const QList<JsonLayer::Recipient> &recipientList,
	    const JsonLayer::Envelope &envelope,
	    const QList<JsonLayer::File> &fileList,
	    const QString &progressLabel,
	    QStringList &resultList,
	    QString &error);

	const QString m_userName; /*!< Account identifier (user login name). */
	const int m_accountID; /*!< Account id of webdatovka. */
	const QList<JsonLayer::Recipient> m_recipientList; /*!< List of recipients. */
	const JsonLayer::Envelope m_envelope; /*!< Envelope data. */
	const QList<JsonLayer::File> m_fileList; /*!< List of attachments. */
	QStringList m_resultList; /*!< ResultList. */
	QString m_error;
};

#endif /* _TASK_SEND_MESSAGE_MOJEID_H_ */
