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

TaskGetAccountListMojeId::TaskGetAccountListMojeId(const QString &userName,
    const QNetworkCookie &sessionid, bool syncWithAll,
    AccountModel *accountModel, QStringList &deletedAccounts)
    : m_return(ACNTLIST_SUCCESS),
    m_error(),
    m_userName(userName),
    m_sessionid(sessionid),
    m_syncWithAll(syncWithAll),
    m_accountModel(accountModel),
    m_deletedAccounts(deletedAccounts)
{
}

void TaskGetAccountListMojeId::run(void)
{

	logDebugLv0NL("Starting download account list task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_return = getAccountList(m_userName, m_sessionid, m_syncWithAll,
	    m_accountModel, m_error, m_deletedAccounts);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Download account list task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskGetAccountListMojeId::Result TaskGetAccountListMojeId::getAccountList(
    const QString &userName, const QNetworkCookie &sessionid,
    bool syncWithAll, AccountModel *accountModel, QString &error,
    QStringList &deletedAccounts)
{
	int newUserId = 0;
	int userId = 0;
	QString localUserName = userName;
	QList<JsonLayer::AccountData> accountList;
	QModelIndex index;

	emit globMsgProcEmitter.progressChange(PL_GET_ACCOUNT_LIST, -1);

	if (!jsonlayer.getAccountList(sessionid, newUserId, accountList, error)) {
		return ACNTLIST_WEBDAT_ERR;
	}

	/* test, if we are logged for relevant userId (account) */
	if (!localUserName.isEmpty()) {
		userId = getWebDatovkaUserId(localUserName);
		if (userId != newUserId) {
			if (accountList.isEmpty()) {
				return ACNTLIST_WRONGUSER;
			} else {
				return ACNTLIST_WU_HAS_ACNT;
			}
		}
		deletedAccounts.clear();

		/* how many accounts are relevant to given userId */
		AccountsMap::iterator i;
		for (i = accountModel->globAccounts.begin();
		    i != accountModel->globAccounts.end(); ++i) {
			if (isWebDatovkaAccount(i->userName())) {
				if (newUserId == getWebDatovkaUserId(i->userName())) {
					deletedAccounts.append(i->userName());
				}
			}
		}
	}

	/* if account list is empty, do nothing */
	if (accountList.isEmpty()) {
		return ACNTLIST_NONEXIST;
	}

	/* do action with account list (add/update account) */
	for (int i = 0; i < accountList.count(); ++i) {
		localUserName = getWebDatovkaUsername(
		    QString::number(accountList.at(i).userId),
		    QString::number(accountList.at(i).accountId));
		if (!accountModel->globAccounts.contains(localUserName)) {
			AcntSettings aSet;
			aSet.setUserName(localUserName);
			aSet.setAccountName(accountList.at(i).name);
			aSet.setLoginMethod(AcntSettings::LIM_MOJE_ID);
			aSet.setSyncWithAll(syncWithAll);
			accountModel->addAccount(aSet, &index);
		} else {
			deletedAccounts.removeOne(localUserName);
			accountModel->globAccounts[localUserName].setAccountName(
			    accountList.at(i).name);
		}

		updateMojeIdAccountData(localUserName, accountList.at(i));

		wdSessions.createCleanSession(localUserName);
		wdSessions.setSessionCookie(localUserName, sessionid);
		emit globMsgProcEmitter.refreshAccountList(localUserName);
	}

	/* if relevantAccounts are not empty
	 * so any account(s) was/were removed from Webdatovka.
	 */
	if (!deletedAccounts.isEmpty()) {
		return ACNTLIST_DELETE_ACNT;
	}

	return ACNTLIST_SUCCESS;
}

bool TaskGetAccountListMojeId::updateMojeIdAccountData(const QString &userName,
    const JsonLayer::AccountData &aData)
{
	const QString acntDbKey(AccountDb::keyFromLogin(userName));

	bool ret = globAccountDbPtr->insertAccountIntoDb(
	    acntDbKey,
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
	    acntDbKey,
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
