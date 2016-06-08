/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#ifndef _TASK_KEEP_ALIVE_H_
#define _TASK_KEEP_ALIVE_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Connects to ISDS server end performs dummy operation.
 */
class TaskKeepAlive : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 */
	explicit TaskKeepAlive(const QString &userName);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	bool m_isAlive; /*!< Returned status. */

private:
	const QString m_userName; /*!< Account identifier (user login name). */
};

#endif /* _TASK_KEEP_ALIVE_H_ */
