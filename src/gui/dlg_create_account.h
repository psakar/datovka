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

#ifndef _DLG_CREATE_ACCOUNT_H_
#define _DLG_CREATE_ACCOUNT_H_

#include <QDialog>
#include <QString>

#include "src/settings/account.h"

namespace Ui {
	class DlgCreateAccount;
}

/*!
 * @brief Account properties dialogue.
 */
class DlgCreateAccount : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Specifies the action to be performed.
	 */
	enum Action {
		ACT_ADDNEW,
		ACT_EDIT,
		ACT_PWD,
		ACT_CERT,
		ACT_CERTPWD
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] accountInfo Account settings.
	 * @param[in] action Specifies which parts of dialogue to be enabled.
	 * @param[in] parent Parent object.
	 */
	DlgCreateAccount(const AcntSettings &accountInfo, enum Action action,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgCreateAccount(void);

	/*!
	 * @brief Obtains account data as the have been submitted by the user.
	 *
	 * @return Data that have been submitted by the user when he pressed
	 *     the Accept/OK button.
	 */
	AcntSettings getSubmittedData(void) const;

private slots:
	/*!
	 * @brief Activates parts of the dialogue depending on the login method.
	 *
	 * @param[in] loginMethodIdx Index of the login method to use.
	 */
	void activateContent(int loginMethodIdx);

	/*!
	 * @brief Checks input sanity, also activates the save/store button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief Opens a dialogue in order to select a certificate file.
	 */
	void addCertificateFile(void);

	/*!
	 * @brief Saves current account information.
	 */
	void saveAccount(void);

private:
	/*!
	 * @brief Sets dialogue content from supplied account data.
	 *
	 * @param[in] acntData Account data to use when setting content.
	 */
	void setContent(const AcntSettings &acntData);

	/*!
	 * @brief Constructs account data from dialogue content.
	 *
	 * @return Account settings according to the dialogue state.
	 */
	AcntSettings getContent(void) const;

	Ui::DlgCreateAccount *m_ui; /*!< UI generated from UI file. */

	AcntSettings m_accountInfo; /*!< Account data with submitted changes. */
	const enum Action m_action; /*!< Actual action the dialogue should be configured to. */
	int m_loginmethod; /*!< Specifies the method the user uses for logging in. */
	QString m_certPath; /*!< Path to certificate. */
};

#endif /* _DLG_CREATE_ACCOUNT_H_ */
