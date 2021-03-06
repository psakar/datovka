/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#pragma once

#include <QDialog>
#include <QTimer>

namespace Ui {
	class DlgChangePwd;
}

class DlgChangePwd : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] boxId Data-box identifier.
	 * @param[in] userName Account username.
	 * @param[in] parent Parent widget.
	 */
	DlgChangePwd(const QString &boxId, const QString &userName,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgChangePwd(void);

	/*!
	 * @brief Changes password for account.
	 *
	 * @param[in] boxId Data-box identifier.
	 * @param[in] userName Account username.
	 * @param[in] parent Parent widget.
	 * @return True on success.
	 */
	static
	bool changePassword(const QString &boxId, const QString &userName,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Show/hide text representation of password in the text lines.
	 */
	void togglePwdVisibility(void);

	/*!
	 * @brief Fill the new password into the text lines.
	 */
	void generatePassword(void);

	/*!
	 * @brief Check input text lines, password length. Activated OK button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief ISDS connection keep-alive function.
	 */
	void pingIsdsServer(void);

	/*!
	 * @brief Send SMS code request into ISDS.
	 */
	void sendSmsCode(void);

private:
	Ui::DlgChangePwd *m_ui; /*!< UI generated from UI file. */

	QTimer m_keepAliveTimer; /*!< Keeps connection to ISDS alive. */

	const QString m_userName; /*!< Account username. */
};
