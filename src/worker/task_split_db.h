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

#ifndef _TASK_SPLIT_DB_H_
#define _TASK_SPLIT_DB_H_

#include "src/worker/task.h"

/*!
 * @brief Task describing database split.
 */
class TaskSplitDb : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in,out] dbSet     Non-null pointer to database container.
	 * @param[in] userName      Account user name.
	 * @param[in] dbDir         Path to database set.
	 * @param[in] newDbDir      Path to new database set.
	 * @param[in] isTestAccount True if account is in the test ISDS.
	 */
	explicit TaskSplitDb(MessageDbSet *dbSet, const QString &userName,
	    const QString &dbDir, const QString &newDbDir, bool isTestAccount);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	bool m_success; /*!< True if successfully finished. */
	QString m_error; /*!< Error description. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSplitDb(const TaskSplitDb &);
	TaskSplitDb &operator=(const TaskSplitDb &);

	/*!
	 * @brief Set back original database path if error
	 *        during database splitting.
	 *
	 * @param[in,out] dbSet     Non-null pointer to database container.
	 * @param[in] dbDir         Path to database set.
	 * @return True on success
	 */
	static
	bool setBackOriginDb(MessageDbSet *dbset, const QString &dbDir);

	/*!
	 * @brief Split message database into new databases contain messages
	 *        for single years.
	 *
	 * @param[in,out] dbSet     Non-null pointer to database container.
	 * @param[in] userName      Account user name.
	 * @param[in] dbDir         Path to database set.
	 * @param[in] newDbDir      Path to new database set.
	 * @param[in] isTestAccount True if account is in the test ISDS.
	 * @param[out] errStr       Error string.
	 * @return True on success
	 */
	static
	bool splitMsgDbByYears(MessageDbSet *dbSet, const QString &userName,
	    const QString &dbDir, const QString &newDbDir,
	    bool isTestAccount, QString &errStr);

	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const QString m_userName; /*!< Account user name. */
	const QString m_dbDir; /*!< Path to database set. */
	const QString m_newDbDir; /*!< Path to new database set. */
	const bool m_isTestAccount;/*!< True if account is in the test ISDS. */
};

#endif /* _TASK_SPLIT_DB_H_ */
