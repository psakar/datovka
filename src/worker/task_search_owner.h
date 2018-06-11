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

#include "src/datovka_shared/isds/box_interface.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing contact searching.
 */
class TaskSearchOwner : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		SO_SUCCESS, /*!< Operation was successful. */
		SO_BAD_DATA, /*!< Data related error, non-existent data. */
		SO_COM_ERROR, /*!< Communication error. */
		SO_ERROR /*!< Other type of error occurred. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] dbOwnerInfo Sought box identifiers.
	 */
	explicit TaskSearchOwner(const QString &userName,
	    const Isds::DbOwnerInfo &dbOwnerInfo);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	/*!
	 * @brief Search for data boxes matching supplied criteria.
	 *
	 * TODO -- This method must be private.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  dbOwnerInfo Sought box identifiers.
	 * @param[out] foundBoxes List of found data boxes to append data to.
	 * @param[out] error Short error description.
	 * @param[out] longError Long error description.
	 * @return Error value.
	 */
	static
	enum Result isdsSearch(const QString &userName,
	    const Isds::DbOwnerInfo &dbOwnerInfo,
	    QList<Isds::DbOwnerInfo> &foundBoxes,
	    QString &error, QString &longError);

	enum Result m_result; /*!< Return state. */
	QString m_isdsError; /*!< Error description.  */
	QString m_isdsLongError; /*!< Long error description. */
	QList<Isds::DbOwnerInfo> m_foundBoxes; /*!< List of found boxes. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSearchOwner(const TaskSearchOwner &);
	TaskSearchOwner &operator=(const TaskSearchOwner &);

	const QString m_userName; /*!< Account identifier (user login name). */
	const Isds::DbOwnerInfo m_dbOwnerInfo; /*!< Sought box identifiers. */
};
