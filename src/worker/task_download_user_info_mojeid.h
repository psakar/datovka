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

#ifndef _TASK_DOWNLOAD_USER_INFO_MOJEID_H_
#define _TASK_DOWNLOAD_USER_INFO_MOJEID_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing download user information.
 */
class TaskDownloadUserInfoMojeId : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] id Account identifier (account id).
	 */
	explicit TaskDownloadUserInfoMojeId(int id);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	bool m_success; /*!< True on success. */
	QString m_isdsError; /*!< Error description. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskDownloadUserInfoMojeId(const TaskDownloadUserInfoMojeId &);
	TaskDownloadUserInfoMojeId &operator=(const TaskDownloadUserInfoMojeId &);

	/*!
	 * @brief Download user information.
	 *
	 * @param[in]     id     Account identifier (account id).
	 * @param[out]    error        Error description.
	 * @return True on success.
	 */
	static
	bool downloadUserInfo(int id, QString &error);

	int m_id; /*!< Account identifier (account id). */
};


#endif /* _TASK_DOWNLOAD_USER_INFO_MOJEID_H_ */
