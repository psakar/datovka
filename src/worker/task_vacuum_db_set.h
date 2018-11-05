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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QString>

#include "src/io/message_db_set.h"
#include "src/worker/task.h"

class TaskVacuumDbSet : public Task {
	Q_DECLARE_TR_FUNCTIONS(TaskVacuumDbSet)

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in,out] dbSet Non-null pointer to database container.
	 */
	explicit TaskVacuumDbSet(MessageDbSet *dbSet);

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
	TaskVacuumDbSet(const TaskVacuumDbSet &);
	TaskVacuumDbSet &operator=(const TaskVacuumDbSet &);

	/*!
	 * @brief Returns full path to database file directory.
	 *
	 * @param dbSet Non-null pointer to database container.
	 * @return Null string on error.
	 */
	static
	QString storagePlace(MessageDbSet *dbSet);

	/*!
	 * @brief Check whether free space in directory available.
	 *
	 * @param storagePlace Full path to database directory.
	 * @param spaceSize Size of space requested.
	 * @return True if requested amount of space is available.
	 */
	static
	bool haveStorageSpace(const QString &storagePlace, qint64 spaceSize);

	/*!
	 * @brief Calls VACUUM on all databases in database container.
	 *
	 * @param[in,out] dbSet Non-null pointer to database container.
	 * @param[out]    error Error string to be set in case of an error.
	 * @return True on success, false on error.
	 */
	static
	bool vacuumDbSet(MessageDbSet *dbSet, QString &error);

	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
};
