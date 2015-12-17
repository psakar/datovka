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

#ifndef _TASK_ERASE_MESSAGE_H_
#define _TASK_ERASE_MESSAGE_H_

#include <QDateTime>
#include <QString>

#include "src/io/message_db_set.h"
#include "src/worker/task.h"

class TaskEraseMessage : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		NOT_DELETED, /*!< Nothing succeeded. */
		DELETED_ISDS, /*!< Only deletion in ISDS succeeded. */
		DELETED_LOCAL, /*!< Only deletion in local db succeeded. */
		DELETED_ISDS_LOCAL /*!< Both deletions succeeded. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName     Account identifier (user login name).
	 * @param[in,out] dbSet        Non-null pointer to database container.
	 * @param[in]     dmId         Message identifier.
	 * @param[in]     deliveryTime Message delivery time.
	 * @param[in]     incoming    True if message is received.
	 * @param[in]     delFromIsds  True if also delete from ISDS.
	 */
	explicit TaskEraseMessage(const QString &userName, MessageDbSet *dbSet,
	    qint64 dmId, const QDateTime &deliveryTime, bool incoming,
	    bool delFromIsds);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	enum Result m_result; /*!< Deletion outcome. */
	QString m_isdsError; /*!< Error description. */
	QString m_isdsLongError; /*!< Long error description. */

private:
	/*!
	 * @brief Erases messages form ISDS and/or from local database.
	 *
	 * @param[in]     userName     Account identifier (user login name).
	 * @param[in,out] dbSet        Non-null pointer to database container.
	 * @param[in]     dmId         Message identifier.
	 * @param[in]     deliveryTime Message delivery time.
	 * @param[in]     incoming    True if message is received.
	 * @param[in]     delFromIsds  True if also delete from ISDS.
	 * @param[out]    error        Error description.
	 * @param[out]    longError    Long error description.
	 * @return Deletion result.
	 */
	static
	enum Result eraseMessage(const QString &userName, MessageDbSet *dbSet,
	    qint64 dmId, const QDateTime &deliveryTime, bool incoming,
	    bool delFromIsds, QString &error, QString &longError);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const qint64 m_dmId; /*!< Message identifier. */
	const QDateTime m_deliveryTime; /*!< Message delivery time. */
	const bool m_incoming; /*!< True if message is received. */
	const bool m_delFromIsds; /*!< True is also delete from ISDS. */
};

#endif /* _TASK_ERASE_MESSAGE_H_ */
