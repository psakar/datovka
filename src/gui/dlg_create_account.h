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


#ifndef _DLG_CREATE_ACCOUNT_H_
#define _DLG_CREATE_ACCOUNT_H_


#include <QDialog>
#include <QFileDialog>
#include <QTreeView>

#include "src/common.h"
#include "ui_dlg_create_account.h"
#include "src/models/accounts_model.h"
#include "src/io/isds_sessions.h"
#include "src/io/account_db.h"


class DlgCreateAccount : public QDialog, public Ui::CreateAccount {
	Q_OBJECT

public:
	enum Action {
		ACT_ADDNEW,
		ACT_EDIT,
		ACT_PWD,
		ACT_CERT,
		ACT_CERTPWD,
		ACT_IDBOX
	};

	/* TODO -- What is the purpose of @acntTopIdx ? */
	DlgCreateAccount(const AccountModel::SettingsMap &accountInfo,
	    Action action, QWidget *parent = 0);

private slots:
	void setActiveButton(int);
	void addCertificateFromFile(void);
	void saveAccount(void);
	void checkInputFields(void);

signals:
	void changedAccountProperties(QString);
	void getAccountUserDataboxInfo(AccountModel::SettingsMap);

private:
	void initAccountDialog(void);
	void setCurrentAccountData(void);

	const AccountModel::SettingsMap m_accountInfo;
	const Action m_action;
	int m_loginmethod;
	QString m_certPath;
};


#endif /* _DLG_CREATE_ACCOUNT_H_ */
