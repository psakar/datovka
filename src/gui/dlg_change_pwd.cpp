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

#include <QMessageBox>

#include "src/gui/dlg_change_pwd.h"
#include "src/io/isds_sessions.h"
#include "src/settings/accounts.h"
#include "src/worker/pool.h"
#include "src/worker/task_change_pwd.h"

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
	this->userNameLneEdit->setText(m_userName);
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
	connect(this->secCodeLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(changePassword(void)));

	this->secCodeLineEdit->setEnabled(false);
	this->otpLabel->setEnabled(false);
	this->smsPushButton->setEnabled(false);

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP) {
		this->secCodeLineEdit->setEnabled(true);
		this->otpLabel->setText(tr("Enter security code:"));
		this->otpLabel->setEnabled(true);
	}

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		this->secCodeLineEdit->setEnabled(true);
		this->smsPushButton->setEnabled(true);
		this->otpLabel->setText(tr("Enter SMS code:"));
		this->otpLabel->setEnabled(true);
		connect(this->smsPushButton, SIGNAL(clicked()), this,
		    SLOT(sendSmsCode()));
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
	if (globIsdsSessions.isConnectedToIsds(m_userName)) {
		qDebug("%s", "Connection to ISDS is alive :)");
	} else {
		qDebug("%s", "Connection to ISDS is dead :(");
	}
}

/* ========================================================================= */
/*
 * Fill the new password in the textlines
 */
void DlgChangePwd::generatePassword(void)
/* ========================================================================= */
{
	/* set one digit as last char */
	QString pwd = generateRandomString(randomStringLength) + "0";
	this->newPwdLineEdit->setText(pwd);
	this->NewPwdLineEdit2->setText(pwd);
}


/* ========================================================================= */
/*
 * Generate a new password string from set of char
 */
QString DlgChangePwd::generateRandomString(int stringLength)
/* ========================================================================= */
{
	QString randomString;

	for(int i = 0; i < stringLength; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	return randomString;
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
	    this->newPwdLineEdit->text().length() >= PWD_MIN_LENGTH &&
	    !this->NewPwdLineEdit2->text().isEmpty() &&
	    this->NewPwdLineEdit2->text().length() >= PWD_MIN_LENGTH &&
	    this->NewPwdLineEdit2->text() == this->newPwdLineEdit->text();

	Q_ASSERT(!m_userName.isEmpty());

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP ||
	    globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		buttonEnabled = buttonEnabled &&
		    !this->secCodeLineEdit->text().isEmpty();
	}

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
 * Send SMS code request into ISDS
 */
void DlgChangePwd::sendSmsCode(void)
/* ========================================================================= */
{
	Q_ASSERT(!m_userName.isEmpty());

	/* show Premium SMS request dialog */
	QMessageBox::StandardButton reply = QMessageBox::question(this,
	    tr("SMS code for account ") +
	    globAccounts[m_userName].accountName(),
	    tr("Account \"%1\" requires authentication via security code "
	    "for connection to databox.")
	        .arg(globAccounts[m_userName].accountName())
	    + "<br/>" +
	    tr("Security code will be sent you via Premium SMS.") +
	    "<br/><br/>" +
	    tr("Do you want to send Premium SMS with "
	    "security code into your mobile phone?"),
	    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

	if (reply == QMessageBox::No) {
		return;
	}

	int status;
	TaskChangePwd *task;

	task = new (std::nothrow) TaskChangePwd(m_userName,
	    this->currentPwdLineEdit->text().toUtf8().constData(),
	    this->newPwdLineEdit->text().toUtf8().constData(),
	    OTP_TIME, QString());
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	status = task->m_isdsRetError;
	delete task;

	if (IE_PARTIAL_SUCCESS == status) {
		QMessageBox::information(this, tr("Enter SMS security code"),
		    tr("SMS security code for account \"%1\"<br/>"
		    "has been sent on your mobile phone...")
		    .arg(globAccounts[m_userName].accountName())
		     + "<br/><br/>" +
		    tr("Enter SMS security code for account")
		    + "<br/><b>"
		    + globAccounts[m_userName].accountName()
		    + " </b>(" + m_userName + ").",
		    QMessageBox::Ok);
		this->otpLabel->setText(tr("Enter SMS code:"));
		this->smsPushButton->setEnabled(false);
	} else {
		QMessageBox::critical(this, tr("Login error"),
		    tr("An error occurred while preparing "
		    "request for SMS with OTP security code.") +
		    "<br/><br/>" +
		    tr("Please try again later or you have to use the "
		    "official web interface of Datové schránky for "
		    "access to your data box."),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Sent new password request into ISDS
 */
void DlgChangePwd::changePassword(void)
/* ========================================================================= */
{
	int status;
	QString errorStr, longErrorStr;
	TaskChangePwd *task;

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP ||
	    globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		task = new (std::nothrow) TaskChangePwd(m_userName,
		    this->currentPwdLineEdit->text().toUtf8().constData(),
		    this->newPwdLineEdit->text().toUtf8().constData(),
		    (globAccounts[m_userName].loginMethod() == AcntSettings::LIM_UNAME_PWD_HOTP) ? OTP_HMAC : OTP_TIME,
		    this->secCodeLineEdit->text());
	} else {
		task = new (std::nothrow) TaskChangePwd(m_userName,
		    this->currentPwdLineEdit->text().toUtf8().constData(),
		    this->newPwdLineEdit->text().toUtf8().constData());
	}
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	status = task->m_isdsRetError;
	errorStr = task->m_isdsError;
	longErrorStr = task->m_isdsLongError;
	delete task;

	if (status == IE_SUCCESS) {
		QMessageBox::information(this, tr("Password has been changed"),
		    tr("Password has been changed "
		        "successfully on the server ISDS.")
		    + "\n\n" +
		    tr("Restart the application. Also don't forget to remember "
		        "the new password so you will still be able to log "
		        "into your data box via the web interface."),
		    QMessageBox::Ok);

		globAccounts[m_userName].setPassword(
		    this->newPwdLineEdit->text());

		/* TODO - delete and create new
		 * isds context with new settings
		 */
	} else {
		Q_ASSERT(!m_userName.isEmpty());
		QString error = tr("Error: ") + errorStr;
		if (!longErrorStr.isEmpty()) {
			error = tr("ISDS returns") + QLatin1String(": ") +
			    longErrorStr;
		}

		QMessageBox::warning(this, tr("Password error"),
		    tr("An error occurred while password was changed.")
		    + "\n\n" + error + "\n\n" +
		    tr("You have to fix the problem and try to again."),
		    QMessageBox::Ok);
	}
}


/* ========================================================================= */
/*
 * Set of possible chars for generation of new password
 */
const QString DlgChangePwd::possibleCharacters(
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!#$%&()*+,-.:=?@[]_{|}~");
const int DlgChangePwd::randomStringLength = PWD_MIN_LENGTH;
/* ========================================================================= */
