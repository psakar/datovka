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


#include <QMessageBox>

#include "dlg_change_pwd.h"
#include "src/io/isds_sessions.h"
#include "src/models/accounts_model.h"


DlgChangePwd::DlgChangePwd(const QString &boxId, const QString &userName,
    QWidget *parent)
    : QDialog(parent),
    m_boxId(boxId),
    m_userName(userName)
{
	setupUi(this);
	initPwdChangeDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgChangePwd::initPwdChangeDialog(void)
/* ========================================================================= */
{
	this->userNameLneEdit->setText(m_accountInfo.userName());
	this->accountLineEdit->setText(m_boxId);
	connect(this->generateButton, SIGNAL(clicked()), this,
	    SLOT(generatePassword()));
	connect(this->showHideButton, SIGNAL(clicked()), this,
	    SLOT(showHidePasswordLine()));
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(this->newPwdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->currentPwdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->NewPwdLineEdit2, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(changePassword(void)));

	if (AccountModel::globAccounts[m_userName].loginMethod() == LIM_HOTP ||
	    AccountModel::globAccounts[m_userName].loginMethod() == LIM_TOTP) {
		this->secCodeLineEdit->setEnabled(true);
		this->label_7->setEnabled(true);
	} else {
		this->secCodeLineEdit->setEnabled(false);
		this->label_7->setEnabled(false);
	}

	pingTimer = new QTimer(this);
	pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(pingTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));
}


/* ========================================================================= */
/*
 * Ping isds server, test if connection on isds server is active
 */
void DlgChangePwd::pingIsdsServer(void)
/* ========================================================================= */
{
	if (isdsSessions.isConnectedToIsds(m_userName)) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}

/* ========================================================================= */
/*
 * Fill the new password in the textlines
 */
void DlgChangePwd::generatePassword(void)
/* ========================================================================= */
{
	QString pwd = generateRandomString();
	this->newPwdLineEdit->setText(pwd);
	this->NewPwdLineEdit2->setText(pwd);
}


/* ========================================================================= */
/*
 * Generate a new password string from set of char
 */
QString DlgChangePwd::generateRandomString(void)
/* ========================================================================= */
{
	QString randomString;
	for(int i=0; i<randomStringLength; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	/* set one digit as last char */
	return randomString + "0";
}


/* ========================================================================= */
/*
 * Check input textline, passwor length and activated OK button
 */
void DlgChangePwd::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled = !this->currentPwdLineEdit->text().isEmpty() &&
	    !this->newPwdLineEdit->text().isEmpty() &&
	    this->newPwdLineEdit->text().length() > 7 &&
	    !this->NewPwdLineEdit2->text().isEmpty() &&
	    this->NewPwdLineEdit2->text().length() > 7 &&
	    this->NewPwdLineEdit2->text() == this->newPwdLineEdit->text();
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    buttonEnabled);
}

/* ========================================================================= */
/*
 * Show/hide text represantion of password in the textlines
 */
void DlgChangePwd::showHidePasswordLine(void)
/* ========================================================================= */
{
	if (this->currentPwdLineEdit->echoMode() == QLineEdit::Password) {
		this->currentPwdLineEdit->setEchoMode(QLineEdit::Normal);
		this->newPwdLineEdit->setEchoMode(QLineEdit::Normal);
		this->NewPwdLineEdit2->setEchoMode(QLineEdit::Normal);
		this->showHideButton->setText(tr("Hide"));
	} else {
		this->currentPwdLineEdit->setEchoMode(QLineEdit::Password);
		this->newPwdLineEdit->setEchoMode(QLineEdit::Password);
		this->NewPwdLineEdit2->setEchoMode(QLineEdit::Password);
		this->showHideButton->setText(tr("Show"));
	}
}


/* ========================================================================= */
/*
 * Sent new password request into ISDS
 */
void DlgChangePwd::changePassword(void)
/* ========================================================================= */
{
	isds_error status;
	char * refnumber = NULL;

	if (AccountModel::globAccounts[m_userName].loginMethod() == LIM_HOTP ||
	    AccountModel::globAccounts[m_userName].loginMethod() == LIM_TOTP) {
		struct isds_otp *otp = NULL;
		otp = (struct isds_otp *) malloc(sizeof(struct isds_otp));
		memset(otp, 0, sizeof(struct isds_otp));

		if (AccountModel::globAccounts[m_userName].loginMethod() ==
		    LIM_HOTP) {
			otp->method = OTP_HMAC;
		} else {
			otp->method = OTP_TIME;
		}

		otp->otp_code = !this->secCodeLineEdit->text().isEmpty() ?
		    strdup(this->secCodeLineEdit->text().toUtf8().constData())
		    : NULL;

		status = isds_change_password(isdsSessions.session(m_userName),
		    this->currentPwdLineEdit->text().toUtf8().constData(),
		    this->newPwdLineEdit->text().toUtf8().constData(),
		    otp, &refnumber);

		free(otp->otp_code);
		free(otp);
	} else {
		status = isds_change_password(isdsSessions.session(m_userName),
		    this->currentPwdLineEdit->text().toUtf8().constData(),
		    this->newPwdLineEdit->text().toUtf8().constData(),
		    NULL, &refnumber);
	}

	free(refnumber);

	if (status == IE_SUCCESS) {
		QMessageBox::information(this, tr("Password has been changed"),
		    tr("Password has been changed "
		        "successfully on the server ISDS.")
		    + "\n\n" +
		    tr("Please, set your new password in the account "
		        "settings and restarts the application. "
		        "Otherwise you can not connect to your databox."),
		    QMessageBox::Ok);

		AccountModel::globAccounts[m_userName].setPassword(
		    this->newPwdLineEdit->text());
	} else {
		QMessageBox::warning(this, tr("Password error"),
		    tr("An error occurred while password was changed.")
		    + "\n\n" + tr("ISDS returns: ")
		    + isds_long_message(
		        isdsSessions.session(m_accountInfo.userName()))
		    + "\n\n" +
		    tr("You have to fix the problem and try to again."),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Set of possilbe chars for generation of new passsword
 */
const QString DlgChangePwd::possibleCharacters(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!#$%&()*+,-.:=?@[]_{|}~");
const int DlgChangePwd::randomStringLength = 9;
/* ========================================================================= */
