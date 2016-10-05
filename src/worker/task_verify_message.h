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

#ifndef _TASK_VERIFY_MESSAGE_H_
#define _TASK_VERIFY_MESSAGE_H_

#include <QDateTime>
#include <QString>

#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing message verification.
 */
class TaskVerifyMessage : public Task {
public:
	/*!
	 * @Brief Return state describing what happened.
	 */
	enum Result {
		VERIFY_SUCCESS, /*!< Verification was successful. */
		VERIFY_NOT_EQUAL, /*!< Hashes are different. */
		VERIFY_ISDS_ERR, /*!< Hash cannot be obtained from ISDS. */
		VERIFY_SQL_ERR, /*!< Hash cannot be obtained from database. */
		VERIFY_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName     Account identifier (user login name).
	 * @param[in,out] dbSet        Non-null pointer to database container.
	 * @param[in]     dmId         Message identifier.
	 * @param[in]     deliveryTime Message delivery time.
	 */
	explicit TaskVerifyMessage(const QString &userName, MessageDbSet *dbSet,
	    qint64 dmId, const QDateTime &deliveryTime);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Result m_result; /*!< Verification outcome. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskVerifyMessage(const TaskVerifyMessage &);
	TaskVerifyMessage &operator=(const TaskVerifyMessage &);

	/*!
	 * @brief Verifies a message.
	 *
	 * @param[in]     userName     Account identifier (user login name).
	 * @param[in,out] dbSet        Non-null pointer to database container.
	 * @param[in]     dmId         Message identifier.
	 * @param[in]     deliveryTime Message delivery time.
	 * @param[out]    error        Error description.
	 * @param[out]    longError    Long error description.
	 * @return Verification result.
	 */
	static
	enum Result verifyMessage(const QString &userName, MessageDbSet *dbSet,
	    qint64 dmId, const QDateTime &deliveryTime, QString &error,
	    QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const qint64 m_dmId; /*!< Message identifier. */
	const QDateTime m_deliveryTime; /*!< Message delivery time. */
};

#endif /* _TASK_VERIFY_MESSAGE_H_ */
