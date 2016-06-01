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

#ifndef _TASK_GET_ACCOUNT_LIST_H_
#define _TASK_GET_ACCOUNT_LIST_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing download account list and its information data.
 */
class TaskGetAccountListMojeId : public Task {
public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in]   sessionid     Session cookie from mojeid.
	 * @param[in]   syncWithAll   If will be synchronized with all accounts.
	 * @param[in]   accountModel  Pointer to account model.
	 */
	explicit TaskGetAccountListMojeId(const QNetworkCookie &sessionid,
	    bool syncWithAll, AccountModel *accountModel);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	bool m_success; /*!< True on success. */
	QString m_error; /*!< Error description. */
	const QNetworkCookie m_sessionid;/*!< Session cookie from mojeid. */
	bool m_syncWithAll; /*!< If will be synchronized with all accounts. */
	AccountModel *m_accountModel; /*!< Pointer to account model. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskGetAccountListMojeId(const TaskGetAccountListMojeId &);
	TaskGetAccountListMojeId &operator=(const TaskGetAccountListMojeId &);

	/*!
	 * @brief Download account list,
	 *
	 * @param[in]   sessionid     Session cookie from mojeid.
	 * @param[in]   syncWithAll   If will be synchronized with all accounts.
	 * @param[in]   accountModel  Pointer to account model.
	 * @param[out]  error          Error description.
	 * @return True on success.
	 */
	static
	bool getAccountList(const QNetworkCookie &sessionid,
	    bool syncWithAll, AccountModel *accountModel, QString &error);

	/*!
	 * @brief Insert/update owner and user information.
	 *
	 * @param[in]    userName      Account username.
	 * @param[in]    aData         Account data.
	 * @return True on success.
	 */
	static
	bool updateMojeIdAccountData(const QString &userName,
	    const JsonLayer::AccountData &aData);
};

#endif /* _TASK_GET_ACCOUNT_LIST_H_ */
