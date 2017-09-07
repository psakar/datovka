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

#ifndef _DLG_CHANGE_PWD_H_
#define _DLG_CHANGE_PWD_H_

#include <QDialog>
#include <QTimer>

namespace Ui {
	class DlgChangePwd;
}

class DlgChangePwd : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] boxId Data-box identifier.
	 * @param[in] userName Account username.
	 * @param[in] parent Parent widget.
	 */
	DlgChangePwd(const QString &boxId, const QString &userName,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgChangePwd(void);

	static
	QString generateRandomString(int stringLength);

private slots:
	/*!
	 * @brief Show/hide text representation of password in the text lines.
	 */
	void togglePwdVisibility(void);

	void generatePassword(void);
	void changePassword(void);
	void checkInputFields(void);
	void pingIsdsServer(void);
	void sendSmsCode(void);

private:
	Ui::DlgChangePwd *m_ui; /*!< UI generated from UI file. */

	QTimer m_pingTimer; /*!< Fires ISDS ping operation. */

	static
	const QString possibleCharacters;
	static
	const int randomStringLength;

	const QString m_userName;
};

#endif /* _DLG_CHANGE_PWD_H_ */
