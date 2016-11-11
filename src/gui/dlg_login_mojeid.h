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


#ifndef _DLG_LOGIN_MOJEID_H_
#define _DLG_LOGIN_MOJEID_H_

#include <QDialog>

#include "src/common.h"
#include "ui_dlg_login_mojeid.h"

class DlgLoginToMojeId : public QDialog, public Ui::LoginToMojeId {
	Q_OBJECT

public:

	DlgLoginToMojeId(const QString &userName, const QString &lastUrl,
	    QWidget *parent = 0);

private slots:
	void setActiveButton(int);
	void addCertificateFromFile(void);
	void sendData(void);
	void checkInputFields(void);

signals:
	void callMojeId(QString, QString, QString,
	    QString, QString, QString, bool, QString);

private:
	void initAccountDialog(void);

	int m_loginmethod;
	QString m_certPath;
	QString m_userName;
	QString m_lastUrl;
	QString m_token;
};

#endif /* _DLG_LOGIN_MOJEID_H_ */
