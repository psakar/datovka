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

#include <QString>

#include "src/datovka_shared/isds/message_interface.h"
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
		VERIFY_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] dmId Message identifier.
	 * @param[in] hashLocal Local has to compare with the value obtained from ISDS.
	 */
	explicit TaskVerifyMessage(const QString &userName, qint64 dmId,
	    const Isds::Hash &hashLocal);

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
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  dmId Message identifier.
	 * @param[in]  hashLocal Local has to compare with the value obtained from ISDS.
	 * @param[out] error Error description.
	 * @param[out] longError Long error description.
	 * @return Verification result.
	 */
	static
	enum Result verifyMessage(const QString &userName, qint64 dmId,
	    const Isds::Hash &hashLocal, QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	const qint64 m_dmId; /*!< Message identifier. */
	const Isds::Hash &m_hashLocal; /*!< Compared hash value. */
};
