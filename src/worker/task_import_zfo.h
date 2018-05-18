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

#ifndef _TASK_IMPORT_ZFO_H_
#define _TASK_IMPORT_ZFO_H_

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QString>

#include "src/isds/message_interface.h"
#include "src/worker/task.h"

/*!
 * @brief Task describing ZFO file import.
 */
class TaskImportZfo : public Task {
	Q_DECLARE_TR_FUNCTIONS(TaskImportZfo)

public:
	/*!
	 * @brief Return state describing what happened.
	 */
	enum Result {
		IMP_SUCCESS, /*!< Import was successful. */
		IMP_DATA_ERROR, /*!< Data couldn't be read or do not match supplied type. */
		IMP_AUTH_ERR, /*!< Data couldn't be authenticated. */
		IMP_ISDS_ERROR, /*!< Error communicating with ISDS. */
		IMP_DB_INS_ERR, /*!< Error inserting into database. */
		IMP_DB_MISSING_MSG, /*!< Related message to delivery info is missing. */
		IMP_DB_EXISTS, /*!< Message already exists. */
		IMP_ERR /*!< Other error. */
	};

	/*!
	 * @brief ZFO type.
	 */
	enum ZfoType {
		ZT_UKNOWN = 0, /*!< Unknown format. */
		ZT_MESSAGE = 1, /*!< ZFO holds message. */
		ZT_DELIVERY_INFO = 2 /*!< ZFO holds delivery information. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] accounts     List of account identifiers.
	 * @param[in] fileName     Full path to ZFO file.
	 * @param[in] type         ZFO file type; type is determined if unknown.
	 * @param[in] authenticate True if you want to authenticate message
	 *                         before importing.
	 */
	explicit TaskImportZfo(const QList<Task::AccountDescr> &accounts,
	    const QString &fileName, enum ZfoType type, bool authenticate);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

	/*!
	 * @brief Determines the type of the ZFO file.
	 *
	 * @param[in] fileName Full path to ZFO file.
	 * @return Type of ZFO file. Returns unknown format on all errors.
	 */
	static
	enum ZfoType determineFileType(const QString &fileName);

	const QString m_fileName; /*!< Full file path to imported file. */

	enum Result m_result; /*!< Import outcome. */
//	QString m_isdsError; /*!< Error description. */
//	QString m_isdsLongError; /*!< Long error description. */
	QString m_resultDesc; /*!<
	                       * Result description that has mostly nothing
	                       * to do with libisds.
	                       */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskImportZfo(const TaskImportZfo &);
	TaskImportZfo &operator=(const TaskImportZfo &);

	/*!
	 * @brief Tries to import a single message ZFO file into a single
	 *     account.
	 *
	 * @param[in]  acnt          Account to try to insert into.
	 * @param[in]  message       Parsed message.
	 * @param[in]  dmId          Message identifier.
	 * @param[in]  deliveryTime  Message delivery time.
	 * @param[in]  direct        Whether it is sent or received message.
	 * @param[in]  fileName      If supplied then the file is going to be
	 *                           authenticated on ISDS server.
	 * @param[out] isdsError     Error description.
	 * @param[out] isdsLongError Long error description.
	 * @param[out] resultDesc    Result description.
	 * @returns Error identifier.
	 */
	static
	enum Result importMessageZfoSingle(const Task::AccountDescr &acnt,
	    const Isds::Message &message, qint64 dmId,
	    const QDateTime &deliveryTime, enum MessageDirection direct,
	    const QString &fileName, QString &isdsError, QString &isdsLongError,
	    QString &resultDesc);

	/*!
	 * @brief Imports message info database.
	 *
	 * @param[in] accounts     List of accounts to try to import data into.
	 * @param[in] fileName     Full name of ZFO file holding the message.
	 * @param[in] authenticate True if data should be authenticated before
	 *                         inserting.
	 * @param[in] resultDesc   String holding result description.
	 * @return Status or error code.
	 */
	static
	enum Result importMessageZfo(const QList<Task::AccountDescr> &accounts,
	    const QString &fileName, bool authenticate, QString &resultDesc);

	/*!
	 * @brief Tries to import a single delivery info ZFO file into a single
	 *     account.
	 *
	 * @param[in]  acnt          Account to try to insert into.
	 * @param[in]  message       Parsed delivery info.
	 * @param[in]  dmId          Message identifier.
	 * @param[in]  deliveryTime  Message delivery time.
	 * @param[in]  fileName      If supplied then the file is going to be
	 *                           authenticated on ISDS server.
	 * @param[out] isdsError     Error description.
	 * @param[out] isdsLongError Long error description.
	 * @param[out] resultDesc    Result description.
	 * @returns Error identifier.
	 */
	static
	enum Result importDeliveryZfoSingle(const Task::AccountDescr &acnt,
	    const Isds::Message &message, qint64 dmId,
	    const QDateTime &deliveryTime, const QString &fileName,
	    QString &isdsError, QString &isdsLongError, QString &resultDesc);

	/*!
	 * @brief Imports delivery info into databases.
	 *
	 * @param[in] accounts     List of accounts to try to import data into.
	 * @param[in] fileName     Full name of ZFO file holding delivery info.
	 * @param[in] authenticate True if data should be authenticated before
	 *                         inserting.
	 * @param[in] resultDesc   String holding result description.
	 * @return Status or error code.
	 */
	static
	enum Result importDeliveryZfo(const QList<Task::AccountDescr> &accounts,
	    const QString &fileName, bool authenticate, QString &resultDesc);

	QList<Task::AccountDescr> m_accounts; /*!< List of accounts to be inserted into. */
	enum ZfoType m_zfoType; /*!< Type of the ZFO file. */
	const bool m_auth; /*!< True if authentication before importing. */
};

#endif /* _TASK_IMPORT_ZFO_H_ */
