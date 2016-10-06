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

#include "src/models/accounts_model.h"
#include "ui_dlg_create_account.h"

/*!
 * @brief Account properties dialogue.
 */
class DlgCreateAccount : public QDialog, public Ui::CreateAccount {
	Q_OBJECT
public:
	/*!
	 * Login method order as they are listed in the dialogue.
	 */
	enum LoginMethodIndex {
		USER_NAME = 0,
		CERTIFICATE = 1,
		USER_CERTIFICATE = 2,
		HOTP = 3,
		TOTP = 4
	};

	/*!
	 * @brief Specifies the action to be performed.
	 */
	enum Action {
		ACT_ADDNEW,
		ACT_EDIT,
		ACT_PWD,
		ACT_CERT,
		ACT_CERTPWD,
		ACT_IDBOX
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] accountInfo Account settings.
	 * @param[in] action Specifies which parts of dialogue to be enabled.
	 * @param[in] parent Parent object.
	 */
	DlgCreateAccount(const AcntSettings &accountInfo, Action action,
	    QWidget *parent = 0);

	/*!
	 * @brief Obtains account data as the have been submitted by the user.
	 *
	 * @return Data that have been submitted by the user when he pressed
	 *     the Accept/OK button.
	 */
	AcntSettings getSubmittedData(void) const;

private slots:
	void setActiveButton(int);
	void addCertificateFromFile(void);
	void saveAccount(void);
	void checkInputFields(void);

signals:
	void getAccountUserDataboxInfo(AcntSettings);

private:
	void initAccountDialog(void);
	void setCurrentAccountData(void);

	AcntSettings m_accountInfo;
	const Action m_action;
	int m_loginmethod;
	QString m_certPath;
};

#endif /* _DLG_CREATE_ACCOUNT_H_ */
