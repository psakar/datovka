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

#ifndef _TASK_DOWNLOAD_USER_INFO_H_
#define _TASK_DOWNLOAD_USER_INFO_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing download user information.
 */
class TaskDownloadUserInfo : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] info     Sought box identifiers.
	 */
	explicit TaskDownloadUserInfo(const QString &userName,
	    const struct isds_DbOwnerInfo *info);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~TaskDownloadUserInfo(void);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	/*!
	 * @brief Search for data boxes matching supplied criteria.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  info     Sought box identifiers.
	 * @param[out] results  List of found data boxes.
	 * @return Value of isds_error.
	 */
	static
	int isdsSearch(const QString &userName,
	    const struct isds_DbOwnerInfo *info, struct isds_list **results);

	int m_isdsError; /*!< Returned error code. */
	struct isds_list *m_results; /*!< List of found data boxes. */

private:
	const QString m_userName; /*!< Account identifier (user login name). */
	const struct isds_DbOwnerInfo *m_info; /*!< Sought box identifiers. */
};

#endif /* _TASK_DOWNLOAD_USER_INFO_H_ */
