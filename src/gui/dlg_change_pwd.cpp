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
#include "src/worker/task_keep_alive.h"
#include "ui_dlg_change_pwd.h"

#define PWD_MIN_LENGTH 8 /* Minimal password length is 8 characters. */

DlgChangePwd::DlgChangePwd(const QString &boxId, const QString &userName,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgChangePwd),
    m_keepAliveTimer(this),
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

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);
	connect(&m_keepAliveTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));
}

DlgChangePwd::~DlgChangePwd(void)
{
	delete m_ui;
}

/*!
 * @brief Send new password request to ISDS.
 *
 * @param[in] userName Account username.
 * @param[in] currentPwd Current password.
 * @param[in] newPwd New password.
 * @param[in] secCode Security code.
 * @param[in] parent Parent widget.
 * @return True on success.
 */
static
bool sendChangePwdRequest(const QString &userName,
    const QString &currentPwd, const QString &newPwd,
    const QString &secCode, QWidget *parent = Q_NULLPTR)
{
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	TaskChangePwd *task = Q_NULLPTR;

	if (globAccounts[userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_HOTP ||
	    globAccounts[userName].loginMethod() ==
	    AcntSettings::LIM_UNAME_PWD_TOTP) {
		task = new (std::nothrow) TaskChangePwd(userName,
		    currentPwd, newPwd,
		    (globAccounts[userName].loginMethod() == AcntSettings::LIM_UNAME_PWD_HOTP) ? OTP_HMAC : OTP_TIME,
		    secCode);
	} else {
		task = new (std::nothrow) TaskChangePwd(userName, currentPwd,
		    newPwd);
	}
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	int taskStatus = task->m_isdsRetError;
	QString errorStr(task->m_isdsError);
	QString longErrorStr(task->m_isdsLongError);
	delete task; task = Q_NULLPTR;

	if (taskStatus == IE_SUCCESS) {
		QMessageBox::information(parent,
		    DlgChangePwd::tr("Password has been changed"),
		    DlgChangePwd::tr("Password has been successfully changed on the ISDS server.") +
		    "\n\n" +
		    DlgChangePwd::tr("Restart the application. "
		        "Also don't forget to remember the new password so you will still be able to log into your data box via the web interface."),
		    QMessageBox::Ok);

		globAccounts[userName].setPassword(newPwd);

		/*
		 * TODO - Delete and create new ISDS context with new settings.
		 */
	} else {
		QString error(DlgChangePwd::tr("Error: ") + errorStr);
		if (!longErrorStr.isEmpty()) {
			error += "\n" + DlgChangePwd::tr("ISDS returns") +
			    QLatin1String(": ") + longErrorStr;
		}

		QMessageBox::warning(parent, DlgChangePwd::tr("Password error"),
		    DlgChangePwd::tr("An error occurred during an attempt to change the password.") +
		    "\n\n" + error + "\n\n" +
		    DlgChangePwd::tr("Fix the problem and try it again."),
		    QMessageBox::Ok);
	}

	return taskStatus == IE_SUCCESS;
}

bool DlgChangePwd::changePassword(const QString &boxId, const QString &userName,
    QWidget *parent)
{
	if (Q_UNLIKELY(boxId.isEmpty() || userName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	DlgChangePwd dlg(boxId, userName, parent);
	if (QDialog::Accepted == dlg.exec()) {
		return sendChangePwdRequest(userName,
		    dlg.m_ui->currentPwdLine->text(),
		    dlg.m_ui->newPwdLine->text(), dlg.m_ui->secCodeLine->text(),
		    parent);
	} else {
		return false;
	}
}

QString DlgChangePwd::generateRandomString(int length)
{
	static const QString possibleCharacters(
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789"
	    "!#$%&()*+,-.:=?@[]_{|}~");

	QString randomString;

	for(int i = 0; i < length; ++i) {
		int index = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at(index);
		randomString.append(nextChar);
	}
	return randomString;
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

void DlgChangePwd::generatePassword(void)
{
	/* Set one digit as last character. */
	const QString pwd(generateRandomString(PWD_MIN_LENGTH) + "0");
	m_ui->newPwdLine->setText(pwd);
	m_ui->newPwdLine2->setText(pwd);
}

void DlgChangePwd::checkInputFields(void)
{
	bool buttonEnabled = !m_ui->currentPwdLine->text().isEmpty() &&
	    !m_ui->newPwdLine->text().isEmpty() &&
	    m_ui->newPwdLine->text().length() >= PWD_MIN_LENGTH &&
	    !m_ui->newPwdLine2->text().isEmpty() &&
	    m_ui->newPwdLine2->text().length() >= PWD_MIN_LENGTH &&
	    m_ui->newPwdLine->text() == m_ui->newPwdLine2->text();

	if (Q_UNLIKELY(m_userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

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

void DlgChangePwd::pingIsdsServer(void)
{
	TaskKeepAlive *task = new (std::nothrow) TaskKeepAlive(m_userName);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return;
	}
	task->setAutoDelete(true);
	globWorkPool.assignHi(task);
}

void DlgChangePwd::sendSmsCode(void)
{
	if (Q_UNLIKELY(m_userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	/* Show Premium SMS request dialogue. */
	QMessageBox::StandardButton reply = QMessageBox::question(this,
	    tr("SMS code for account '%1'").arg(globAccounts[m_userName].accountName()),
	    tr("Account \"%1\" requires authentication via security code for connection to data box.")
	        .arg(globAccounts[m_userName].accountName()) +
	    "<br/>" +
	    tr("Security code will be sent you via Premium SMS.") +
	    "<br/><br/>" +
	    tr("Do you want to send Premium SMS with security code into your mobile phone?"),
	    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

	if (reply == QMessageBox::No) {
		return;
	}

	TaskChangePwd *task = new (std::nothrow) TaskChangePwd(m_userName,
	    m_ui->currentPwdLine->text(), m_ui->newPwdLine->text(),
	    OTP_TIME, QString());
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	int taskStatus = task->m_isdsRetError;
	delete task; task = Q_NULLPTR;

	if (IE_PARTIAL_SUCCESS == taskStatus) {
		QMessageBox::information(this, tr("Enter SMS security code"),
		    tr("SMS security code for account \"%1\"<br/>has been sent on your mobile phone...")
		        .arg(globAccounts[m_userName].accountName()) +
		    "<br/><br/>" +
		    tr("Enter SMS security code for account") +
		        "<br/><b>" +
		        globAccounts[m_userName].accountName() +
		        "</b> (" + m_userName + ").",
		    QMessageBox::Ok);
		m_ui->otpLabel->setText(tr("Enter SMS code:"));
		m_ui->smsPushButton->setEnabled(false);
	} else {
		QMessageBox::critical(this, tr("Login error"),
		    tr("An error occurred while preparing request for SMS with OTP security code.") +
		    "<br/><br/>" +
		    tr("Please try again later or you have to use the official web interface of Datové schránky to access to your data box."),
		    QMessageBox::Ok);
	}
}
