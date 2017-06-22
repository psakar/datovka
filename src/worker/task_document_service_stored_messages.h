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

#ifndef _TASK_DOCUMENT_SERVICE_STORED_MESSAGES_H_
#define _TASK_DOCUMENT_SERVICE_STORED_MESSAGES_H_

#include <QList>
#include <QString>

#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing downloading information about stored messages.
 */
class TaskDocumentServiceStoredMessages : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		DS_DSM_SUCCESS, /*!< Operation was successful. */
		DS_DSM_COM_ERROR, /*!< Error communicating with ISDS. */
		DS_DSM_DB_INS_ERR, /*!< Error inserting into database. */
		DS_DSM_ERR /*!< Other error. */
	};

	/*!
	 * @brief Operation to be performed.
	 */
	enum Operation {
		DS_UPDATE_STORED, /*!< Update only messages in document service database. */
		DS_DOWNLOAD_ALL /*!< Download all messages that are held in database set. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] urlStr Document service URL.
	 * @param[in] tokenStr Document service access token.
	 * @param[in] operation Actual action to be performed.
	 * @param[in] dbSet Database set to be used to obtain message identifiers.
	 * @patam[in] exludedDmIds Message identifiers that should not be queried.
	 */
	explicit TaskDocumentServiceStoredMessages(
	    const QString &urlStr, const QString &tokenStr,
	    enum Operation operation, const MessageDbSet *dbSet,
	    const QList<qint64> &exludedDmIds = QList<qint64>());

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Result m_result; /*!< Return state. */

private:
	/*!
	 * @brief Download stored files information and save to document service
	 *     database.
	 *
	 * @param[in] urlStr Document service URL.
	 * @param[in] tokenStr Document service access token.
	 * @param[in] operation Actual action to be performed.
	 * @param[in] dbSet Database set to be used to obtain message identifiers.
	 * @patam[in] exludedDmIds Message identifiers that should not be queried.
	 * @return Return state.
	 */
	static
	enum Result downloadStoredMessages(const QString &urlStr,
	    const QString &tokenStr, enum Operation operation,
	    const MessageDbSet *dbSet, const QList<qint64> &exludedDmIds);

	const QString m_url; /*!< String containing document service URL. */
	const QString m_token; /*!< Document service access token. */

	const enum Operation m_operation; /*!< Operation to be performed. */

	const MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const QList<qint64> m_exludedDmIds; /*!<
	                                     * List of messages that should
	                                     * not be queried.
	                                     */
};

#endif /* _TASK_DOCUMENT_SERVICE_STORED_MESSAGES_H_ */
