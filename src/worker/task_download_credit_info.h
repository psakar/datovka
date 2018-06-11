/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#ifndef _TASK_DOWNLOAD_CREDIT_INFO_H_
#define _TASK_DOWNLOAD_CREDIT_INFO_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing download credit information.
 */
class TaskDownloadCreditInfo : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] dbId     Data box identifier.
	 */
	explicit TaskDownloadCreditInfo(const QString &userName,
	    const QString &dbId);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	qint64 m_heller; /*!< Credit in hundredths of crowns. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskDownloadCreditInfo(const TaskDownloadCreditInfo &);
	TaskDownloadCreditInfo &operator=(const TaskDownloadCreditInfo &);

	/*!
	 * @brief Download credit information from ISDS.
	 *
	 * @param[in]  userName Account identifier (user login name).
	 * @param[in]  dbId Data box identifier.
	 * @param[out] error Error description.
	 * @param[out] longError Long error description.
	 * @retrun Credit in heller, -1 on error.
	 */
	static
	qint64 downloadCreditFromISDS(const QString &userName,
	    const QString &dbId, QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_dbId; /*!< Data box identifier. */
};

#endif /* _TASK_DOWNLOAD_CREDIT_INFO_H_ */
