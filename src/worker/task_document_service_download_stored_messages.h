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

#ifndef _TASK_DOCUMENT_SERVICE_DOWNLOAD_STORED_MESSAGES_H_
#define _TASK_DOCUMENT_SERVICE_DOWNLOAD_STORED_MESSAGES_H_

#include <QList>

#include "src/document_service/io/document_service_connection.h"
#include "src/io/document_service_db.h"
#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing downloading information about stored messages.
 */
class TaskDocumentServiceDownloadStoredMessages : public Task {
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
	 * @brief Constructor.
	 */
	explicit TaskDocumentServiceDownloadStoredMessages(
	    const QString &urlStr, const QString &tokenStr, MessageDbSet *dbSet,
	    const QList<qint64> &exludedDmIds = QList<qint64>());

	/*!
	 * @brief Performs actual message download.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Result m_result; /*!< Return state. */

private:
	static
	enum Result downloadStoredMessages(MessageDbSet *dbSet,
	    DocumentServiceConnection &dsc, const QList<qint64> &exludedDmIds);

	DocumentServiceConnection m_dsc; /*!< Connection to document service. */

	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const QList<qint64> m_exludedDmIds; /*!<
	                                     * List of messages that should
	                                     * not be queried.
	                                     */
};

#endif /* _TASK_DOCUMENT_SERVICE_DOWNLOAD_STORED_MESSAGES_H_ */
