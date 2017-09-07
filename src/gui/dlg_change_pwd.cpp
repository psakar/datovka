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
#include "ui_dlg_change_pwd.h"

#define PWD_MIN_LENGTH 8 /* Minimal password length is 8 characters. */

DlgChangePwd::DlgChangePwd(const QString &boxId, const QString &userName,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgChangePwd),
    m_pingTimer(this),
    m_userName(userName)
{
	m_ui->setupUi(this);

	m_ui->userNameLine->setText(m_userName);
	m_ui->accountLine->setText(boxId);
	connect(m_ui->togglePwdVisibilityButton, SIGNAL(clicked()), this,
	    SLOT(togglePwdVisibility()));
	connect(m_ui->generateButton, SIGNAL(clicked()), this,
	    SLOT(generatePassword()));

	connect(m_ui->currentPwdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->newPwdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->newPwdLine2, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->secCodeLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	m_ui->otpLabel->setEnabled(false);
	m_ui->secCodeLine->setEnabled(false);
	m_ui->smsPushButton->setEnabled(false);

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP) {
		m_ui->otpLabel->setText(tr("Enter security code:"));
		m_ui->otpLabel->setEnabled(true);
		m_ui->secCodeLine->setEnabled(true);
	}

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		m_ui->otpLabel->setText(tr("Enter SMS code:"));
		m_ui->otpLabel->setEnabled(true);
		m_ui->secCodeLine->setEnabled(true);
		m_ui->smsPushButton->setEnabled(true);
		connect(m_ui->smsPushButton, SIGNAL(clicked()), this,
		    SLOT(sendSmsCode()));
	}

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(m_ui->buttonBox, SIGNAL(accepted()), this,
	    SLOT(changePassword()));

	m_pingTimer.start(DLG_ISDS_KEEPALIVE_MS);
	connect(&m_pingTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));
}

DlgChangePwd::~DlgChangePwd(void)
{
	delete m_ui;
}

void DlgChangePwd::togglePwdVisibility(void)
{
	enum QLineEdit::EchoMode echoMode =
	    (m_ui->currentPwdLine->echoMode() == QLineEdit::Password) ?
	        QLineEdit::Normal : QLineEdit::Password;
	QString buttonText(
	    (m_ui->currentPwdLine->echoMode() == QLineEdit::Password) ?
	        tr("Hide") : tr("Show"));

	m_ui->currentPwdLine->setEchoMode(echoMode);
	m_ui->newPwdLine->setEchoMode(echoMode);
	m_ui->newPwdLine2->setEchoMode(echoMode);
	m_ui->togglePwdVisibilityButton->setText(buttonText);
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
	m_ui->newPwdLine->setText(pwd);
	m_ui->newPwdLine2->setText(pwd);
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
	bool buttonEnabled = !m_ui->currentPwdLine->text().isEmpty() &&
	    !m_ui->newPwdLine->text().isEmpty() &&
	    m_ui->newPwdLine->text().length() >= PWD_MIN_LENGTH &&
	    !m_ui->newPwdLine2->text().isEmpty() &&
	    m_ui->newPwdLine2->text().length() >= PWD_MIN_LENGTH &&
	    m_ui->newPwdLine->text() == m_ui->newPwdLine2->text();

	Q_ASSERT(!m_userName.isEmpty());

	if (globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP ||
	    globAccounts[m_userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		buttonEnabled = buttonEnabled &&
		    !m_ui->secCodeLine->text().isEmpty();
	}

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    buttonEnabled);
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
	    m_ui->currentPwdLine->text().toUtf8().constData(),
	    m_ui->newPwdLine->text().toUtf8().constData(),
	    OTP_TIME, QString());
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	status = task->m_isdsRetError;
	delete task; task = Q_NULLPTR;

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
		m_ui->otpLabel->setText(tr("Enter SMS code:"));
		m_ui->smsPushButton->setEnabled(false);
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
		    m_ui->currentPwdLine->text().toUtf8().constData(),
		    m_ui->newPwdLine->text().toUtf8().constData(),
		    (globAccounts[m_userName].loginMethod() == AcntSettings::LIM_UNAME_PWD_HOTP) ? OTP_HMAC : OTP_TIME,
		    m_ui->secCodeLine->text());
	} else {
		task = new (std::nothrow) TaskChangePwd(m_userName,
		    m_ui->currentPwdLine->text().toUtf8().constData(),
		    m_ui->newPwdLine->text().toUtf8().constData());
	}
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	status = task->m_isdsRetError;
	errorStr = task->m_isdsError;
	longErrorStr = task->m_isdsLongError;
	delete task; task = Q_NULLPTR;

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
		    m_ui->newPwdLine->text());

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
