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

#include <QThread>

#include "src/io/account_db.h"
#include "src/models/accounts_model.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_get_account_list_mojeid.h"
#include "src/web/json.h"
#include "src/io/wd_sessions.h"

TaskGetAccountListMojeId::TaskGetAccountListMojeId(
   const QNetworkCookie &sessionid, bool syncWithAll,
   AccountModel *accountModel)
    : m_success(false),
    m_error(),
    m_sessionid(sessionid),
    m_syncWithAll(syncWithAll),
    m_accountModel(accountModel)
{
}

void TaskGetAccountListMojeId::run(void)
{

	logDebugLv0NL("Starting download account list task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = getAccountList(m_sessionid, m_syncWithAll, m_accountModel,
	    m_error);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download account list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

bool TaskGetAccountListMojeId::getAccountList(const QNetworkCookie &sessionid,
    bool syncWithAll, AccountModel *accountModel, QString &error)
{
	QList<JsonLayer::AccountData> accountList;

	emit globMsgProcEmitter.progressChange(PL_GET_ACCOUNT_LIST, -1);

	jsonlayer.getAccountList(sessionid, accountList, error);

	if (accountList.isEmpty()) {
		return false;
	}

	QModelIndex index;

	for (int i = 0; i < accountList.count(); ++i) {
		const QString userName = getWebDatovkaUsername(
		/* TODO - uncommented */
		    "1",
		    //QString::number(accountList.at(i).userId),
		    QString::number(accountList.at(i).accountId));
		if (!accountModel->globAccounts.contains(userName)) {

			AcntSettings aSet;
			aSet.setUserName(userName);
			aSet.setAccountName(accountList.at(i).name);
			aSet.setLoginMethod(LIM_MOJEID);
			aSet.setSyncWithAll(syncWithAll);
			accountModel->addAccount(aSet, &index);
		} else {
			accountModel->globAccounts[userName].setAccountName(
			    accountList.at(i).name);
		}

		updateMojeIdAccountData(userName, accountList.at(i));

		wdSessions.createCleanSession(userName);
		wdSessions.setSessionCookie(userName, sessionid);
		emit globMsgProcEmitter.refreshAccountList(userName);
	}

	return true;
}

bool TaskGetAccountListMojeId::updateMojeIdAccountData(const QString &userName,
    const JsonLayer::AccountData &aData)
{
	bool ret = globAccountDbPtr->insertAccountIntoDb(
	    userName + "___True",
	    aData.ownerInfo.dbID,
	    aData.ownerInfo.dbType,
	    aData.ownerInfo.ic.toInt(),
	    aData.ownerInfo.pnFirstName,
	    aData.ownerInfo.pnMiddleName,
	    aData.ownerInfo.pnLastName,
	    aData.ownerInfo.pnLastNameAtBirth,
	    aData.ownerInfo.firmName,
	    aData.ownerInfo.biDate,
	    aData.ownerInfo.biCity,
	    aData.ownerInfo.biCounty,
	    aData.ownerInfo.biState,
	    aData.ownerInfo.adCity,
	    aData.ownerInfo.adStreet,
	    aData.ownerInfo.adNumberInStreet,
	    aData.ownerInfo.adNumberInMunicipality,
	    aData.ownerInfo.adZipCode,
	    aData.ownerInfo.adState,
	    aData.ownerInfo.nationality,
	    aData.ownerInfo.identifier,
	    aData.ownerInfo.registryCode,
	    aData.ownerInfo.dbState,
	    aData.ownerInfo.dbEffectiveOVM,
	    aData.ownerInfo.dbOpenAddressing
	);

	ret = globAccountDbPtr->insertUserIntoDb(
	    userName + "___True",
	    aData.userInfo.userType,
	    aData.userInfo.userPrivils,
	    aData.userInfo.pnFirstName,
	    aData.userInfo.pnMiddleName,
	    aData.userInfo.pnLastName,
	    aData.userInfo.pnLastNameAtBirth,
	    aData.userInfo.adCity,
	    aData.userInfo.adStreet,
	    aData.userInfo.adNumberInStreet,
	    aData.userInfo.adNumberInMunicipality,
	    aData.userInfo.adZipCode,
	    aData.userInfo.adState,
	    aData.userInfo.biDate,
	    aData.userInfo.ic,
	    aData.userInfo.firmName,
	    aData.userInfo.caStreet,
	    aData.userInfo.caCity,
	    aData.userInfo.caZipCode,
	    aData.userInfo.caState
	);

	return ret;
}
