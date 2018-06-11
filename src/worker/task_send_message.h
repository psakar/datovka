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
#include <QList>
#include <QString>

#include "src/datovka_shared/isds/message_interface.h"
#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing sending message.
 */
class TaskSendMessage : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		SM_SUCCESS, /*!< Operation was successful. */
		SM_ISDS_ERROR, /*!< Error communicating with ISDS. */
		SM_DB_INS_ERR, /*!< Error inserting into database. */
		SM_ERR /*!< Other error. */
	};

	/*!
	 * @brief Gives more detailed information about sending outcome.
	 */
	class ResultData {
	public:
		/*!
		 * @brief Constructors.
		 */
		ResultData(void);
		ResultData(enum Result res, const QString &eInfo,
		    const QString &recId, const QString &recName, bool pdz,
		    qint64 mId);

		enum Result result; /*!< Return state. */
		QString errInfo; /*!< Error description. */
		QString dbIDRecipient; /*!< Recipient identifier. */
		QString recipientName; /*!< Recipient name. */
		bool isPDZ; /*!< True if message was sent as PDZ. */
		qint64 dmId; /*!< Sent message identifier. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName         Account identifier (user login name).
	 * @param[in,out] dbSet            Non-null pointer to database
	 *                                 container.
	 * @param[in]     transactId       Unique transaction identifier.
	 * @param[in]     message          Message to be sent.
	 * @param[in]     recipientName    Message recipient name.
	 * @param[in]     recipientAddress Message recipient address.
	 * @param[in]     isPDZ            True if message is a PDZ.
	 */
	explicit TaskSendMessage(const QString &userName,
	    MessageDbSet *dbSet, const QString &transactId,
	    const Isds::Message &message,
	    const QString &recipientName, const QString &recipientAddress,
	    bool isPDZ);

	/*!
	 * @brief Performs actual message sending.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	ResultData m_resultData; /*!< Return state data. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSendMessage(const TaskSendMessage &);
	TaskSendMessage &operator=(const TaskSendMessage &);

	/*!
	 * @brief Sends a single message to ISDS fro given account.
	 *
	 * @param[in]     userName         Account identifier (user login name).
	 * @param[in,out] dbSet            Database container.
	 * @param[in]     message          Message being sent.
	 * @param[in]     recipientName    Message recipient name.
	 * @param[in]     recipientAddress Message recipient address.
	 * @param[in]     isPDZ            True if message is a PDZ.
	 * @param[in]     progressLabel    Progress-bar label.
	 * @param[out]    result           Results, pass NULL if not desired.
	 * @return Error state.
	 */
	static
	enum Result sendMessage(const QString &userName, MessageDbSet &dbSet,
	    const Isds::Message &message, const QString &recipientName,
	    const QString &recipientAddress, bool isPDZ,
	    const QString &progressLabel, ResultData *result);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const QString m_transactId; /*!< Unique transaction identifier. */
	const Isds::Message m_message; /*!< Message to be sent. */
	const QString m_recipientName; /*!< Message recipient name. */
	const QString m_recipientAddress; /*!< Message recipient address. */
	const bool m_isPDZ; /*!< True if message is a PDZ. */
};
