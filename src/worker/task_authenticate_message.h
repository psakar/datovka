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

#ifndef _TASK_AUTHENTICATE_MESSAGE_H_
#define _TASK_AUTHENTICATE_MESSAGE_H_

#include <QByteArray>
#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing message authentication.
 */
class TaskAuthenticateMessage : public Task {
public:
	/*!
	 * @Brief Return state describing what happened.
	 */
	enum Result {
		AUTH_SUCCESS, /*!< Authentication was successful. */
		AUTH_DATA_ERROR, /*!< Data to be authenticated are empty. */
		AUTH_NOT_EQUAL, /*!< Data could not be authenticated. */
		AUTH_ISDS_ERROR, /*!< Error communicating with ISDS. */
		AUTH_ERR, /*!< Other error. */
		AUTH_CANCELLED /*!< Operation cancelled, used outside. */
	};

	/*!
	 * @brief Constructor, from data.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] fileName Name of file containing message data.
	 */
	explicit TaskAuthenticateMessage(const QString &userName,
	    const QString &fileName);

	/*!
	 * @brief Constructor, from file.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] data     Message data.
	 */
	explicit TaskAuthenticateMessage(const QString &userName,
	    const QByteArray &data);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	/*!
	 * @brief Authenticates a message.
	 *
	 * TODO -- This method ought to be protected.
	 *
	 * @param[in]  userName  Account identifier (user login name).
	 * @param[in]  data      Message data.
	 * @param[out] error     Error description.
	 * @param[out] longError Long error description.
	 * @return Authentication result.
	 */
	static
	enum Result authenticateMessage(const QString &userName,
	    const QByteArray &data, QString &error, QString &longError);

	enum Result m_result; /*!< Authentication outcome. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskAuthenticateMessage(const TaskAuthenticateMessage &);
	TaskAuthenticateMessage &operator=(const TaskAuthenticateMessage &);

	const QString m_userName; /*!< Account identifier (user login name). */
	QByteArray m_data; /*!< Message data. */
};

#endif /* _TASK_AUTHENTICATE_MESSAGE_H_ */
