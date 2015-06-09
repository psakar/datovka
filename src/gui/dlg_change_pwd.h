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


#ifndef _DLG_CHANGE_PWD_H_
#define _DLG_CHANGE_PWD_H_


#include <QDialog>
#include <QTimer>
#include <QTreeView>

#include "src/common.h"
#include "src/models/accounts_model.h"
#include "ui_dlg_change_pwd.h"


class DlgChangePwd : public QDialog, public Ui::ChangePwd {
	Q_OBJECT

public:
	DlgChangePwd(const QString &boxId, const QString &userName,
	    QWidget *parent = 0);

private slots:
	void generatePassword(void);
	void showHidePasswordLine(void);
	void changePassword(void);
	void checkInputFields(void);
	void pingIsdsServer(void);

private:
	QTimer *pingTimer;
	void initPwdChangeDialog(void);

	static
	QString generateRandomString(void);

	static
	const QString possibleCharacters;
	static
	const int randomStringLength;

	const QString m_boxId;
	const QString m_userName;
};


#endif /* _DLG_CHANGE_PWD_H_ */
