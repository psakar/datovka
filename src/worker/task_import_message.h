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

#ifndef _TASK_IMPORT_MESSAGE_H_
#define _TASK_IMPORT_MESSAGE_H_

#include "src/io/message_db_single.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing messages import from external database file.
 */
class TaskImportMessage : public Task {
public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		IMP_SUCCESS, /*!< Import was successful. */
		IMP_DB_ERROR, /*!< Open database fail. */
		IMP_MSG_ID_ERR, /*!< Message ID is wrong. */
		IMP_DB_INS_ERR, /*!< Error inserting into database. */
		IMP_DB_EXISTS, /*!< Message already exists. */
		IMP_ERR /*!< Other error. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName - Account user name.
	 * @param[in] dbSet - Non-null pointer to target database container.
	 * @param[in,out] dbFileList - List of source databases.
	 * @param[in,out] dbId - Databox ID of target account.
	 */
	explicit TaskImportMessage(const QString &userName, MessageDbSet *dbSet,
	    const QStringList &dbFileList, const QString &dbId);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	enum Result m_result; /*!< Import outcome. */
	QStringList m_resultDescList; /*!< List of unsuccess import messages. */
	int m_msgCntTotal; /*!< Holds total number of messages in source db. */
	int m_importedMsg; /*!< Holds number of success imported messages */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskImportMessage(const TaskImportMessage &);
	TaskImportMessage &operator=(const TaskImportMessage &);

	/*!
	 * @brief Tries to import a single message into a single database.
	 *
	 * @param[in] userName - Account user name.
	 * @param[in,out] dbSet - Non-null pointer to database container.
	 * @param[in] dbSet - Non-null pointer to target database container.
	 * @param[in] srcDbSingle - Non-null pointer to source database.
	 * @param[in] dbFile - Path to database file.
	 * @param[in] mId - Mesasge ID.
	 * @param[in] dbId - Databox ID of target account.
	 * @param[out] resultDesc - Result description.
	 * @returns Error identifier.
	 */
	static
	enum Result importSingleMessage(const QString &userName,
	     MessageDbSet *dbSet, const MessageDbSingle *srcDbSingle,
	    const QString &dbFile, const MessageDb::MsgId &mId,
	    const QString &dbId, QString &resultDesc);

	/*!
	 * @brief Imports messages into database.
	 *
	 * @param[in] userName - Account user name.
	 * @param[in] dbSet - Non-null pointer to target database container.
	 * @param[in] dbFileList - List of source databases.
	 * @param[in] dbId - Databox ID of target account.
	 * @param[out] resultDescList - List of import results.
	 * @param[out] msgCntTotal - Total number of messages in source db.
	 * @param[out] importedMsg - Number of success imported messages.
	 * @returns Error identifier.
	 */
	static
	enum Result importMessages(const QString &userName,
	    MessageDbSet *dbSet, const QStringList &dbFileList,
	    const QString &dbId, QStringList &resultDescList,
	    int &msgCntTotal, int &importedMsg);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to target database container. */
	const QStringList m_dbFileList;  /*!< List of source databases. */
	const QString m_dbId; /*!< Databox ID of target account. */
	QString m_resultDesc; /*!< Result description of imported message. */
};

#endif /* _TASK_IMPORT_MESSAGE_H_ */
