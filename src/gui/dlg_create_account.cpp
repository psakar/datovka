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

#include <QFileDialog>

#include "src/common.h"
#include "src/gui/dlg_create_account.h"
#include "src/io/dbs.h"
#include "src/log/log.h"
#include "ui_dlg_create_account.h"

DlgCreateAccount::DlgCreateAccount(const AcntSettings &accountInfo,
    enum Action action, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgCreateAccount),
    m_accountInfo(accountInfo),
    m_action(action),
    m_loginmethod(USER_NAME),
    m_certPath()
{
	m_ui->setupUi(this);

	initialiseDialogue();

	/* Set dialogue content for existing account. */
	if (ACT_ADDNEW != m_action) {
		setContent(m_accountInfo);
	}
}

DlgCreateAccount::~DlgCreateAccount(void)
{
	delete m_ui;
}

AcntSettings DlgCreateAccount::getSubmittedData(void) const
{
	return m_accountInfo;
}

void DlgCreateAccount::activateContent(int loginMethodIdx)
{
	m_loginmethod = loginMethodIdx;

	switch (m_loginmethod) {
	case CERTIFICATE:
		m_ui->pwdLabel->setEnabled(false);
		m_ui->pwdLine->setEnabled(false);
		m_ui->rememberPwdCheckBox->setEnabled(false);
		m_ui->certLabel->setEnabled(true);
		m_ui->addCertButton->setEnabled(true);
		break;
	case USER_CERTIFICATE:
		m_ui->pwdLabel->setEnabled(true);
		m_ui->pwdLine->setEnabled(true);
		m_ui->rememberPwdCheckBox->setEnabled(true);
		m_ui->certLabel->setEnabled(true);
		m_ui->addCertButton->setEnabled(true);
		break;
	default:
		m_ui->pwdLabel->setEnabled(true);
		m_ui->pwdLine->setEnabled(true);
		m_ui->rememberPwdCheckBox->setEnabled(true);
		m_ui->certLabel->setEnabled(false);
		m_ui->addCertButton->setEnabled(false);
		break;
	}
	m_ui->usernameLine->setEnabled(m_action == ACT_ADDNEW);
	m_ui->testAcntCheckBox->setEnabled(m_action == ACT_ADDNEW);

	checkInputFields();
}

void DlgCreateAccount::checkInputFields(void)
{
	bool enabled = false;

	switch (m_loginmethod) {
	case CERTIFICATE:
		enabled = !m_ui->acntNameLine->text().isEmpty()
		    && !m_ui->usernameLine->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	case USER_CERTIFICATE:
		enabled = !m_ui->acntNameLine->text().isEmpty()
		    && !m_ui->usernameLine->text().isEmpty()
		    && !m_ui->pwdLine->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	default:
		enabled = !m_ui->acntNameLine->text().isEmpty()
		    && !m_ui->usernameLine->text().isEmpty()
		    && !m_ui->pwdLine->text().isEmpty();
		break;
	}

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void DlgCreateAccount::addCertificateFile(void)
{
	const QString certFileName(QFileDialog::getOpenFileName(this,
	    tr("Open Certificate"), QString(),
	    tr("Certificate Files (*.p12 *.pem)")));
	if (!certFileName.isEmpty()) {
		m_ui->addCertButton->setText(certFileName);
		m_ui->addCertButton->setIcon(
		    QIcon(ICON_3PARTY_PATH + QString("key_16.png")));
		m_certPath = certFileName;
		checkInputFields();
	}
}

void DlgCreateAccount::saveAccount(void)
{
	debugSlotCall();

	/* Store the submitted settings. */
	m_accountInfo = getContent();
}

void DlgCreateAccount::initialiseDialogue(void)
{
	m_ui->loginMethodComboBox->addItem(tr("Password"));
	m_ui->loginMethodComboBox->addItem(tr("Certificate"));
	m_ui->loginMethodComboBox->addItem(tr("Certificate + Password"));
	m_ui->loginMethodComboBox->addItem(tr("Password + Secure code"));
	m_ui->loginMethodComboBox->addItem(tr("Password + Secure SMS"));

	m_ui->certLabel->setEnabled(false);
	m_ui->addCertButton->setEnabled(false);

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(m_ui->loginMethodComboBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(activateContent(int)));
	connect(m_ui->addCertButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFile()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(saveAccount()));
	connect(m_ui->acntNameLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->usernameLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->pwdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
}

void DlgCreateAccount::setContent(const AcntSettings &acntData)
{
	if (acntData.userName().isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	QString windowTitle(this->windowTitle());

	switch (m_action) {
	case ACT_EDIT:
		windowTitle = tr("Update account %1")
		    .arg(acntData.accountName());
		break;
	case ACT_PWD:
		windowTitle = tr("Enter password for account %1")
		    .arg(acntData.accountName());
		m_ui->infoLabel->setEnabled(false);
		m_ui->acntNameLine->setEnabled(false);
		m_ui->loginMethodComboBox->setEnabled(false);
		m_ui->addCertButton->setEnabled(false);
		break;
	case ACT_CERT:
		windowTitle = tr("Set certificate for account %1")
		    .arg(acntData.accountName());
		m_ui->infoLabel->setEnabled(false);
		m_ui->acntNameLine->setEnabled(false);
		m_ui->loginMethodComboBox->setEnabled(false);
		m_ui->pwdLine->setEnabled(false);
		break;
	case ACT_CERTPWD:
		windowTitle = tr("Enter password/certificate for account %1")
		    .arg(acntData.accountName());
		m_ui->infoLabel->setEnabled(false);
		m_ui->acntNameLine->setEnabled(false);
		m_ui->loginMethodComboBox->setEnabled(false);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	setWindowTitle(windowTitle);
	m_ui->acntNameLine->setText(acntData.accountName());
	m_ui->usernameLine->setText(acntData.userName());
	m_ui->usernameLine->setEnabled(false);
	m_ui->testAcntCheckBox->setEnabled(false);

	int itemIdx;
	switch (acntData.loginMethod()) {
	case AcntSettings::LIM_UNAME_PWD:
		itemIdx = USER_NAME;
		break;
	case AcntSettings::LIM_UNAME_CRT:
		itemIdx = CERTIFICATE;
		break;
	case AcntSettings::LIM_UNAME_PWD_CRT:
		itemIdx = USER_CERTIFICATE;
		break;
	case AcntSettings::LIM_UNAME_PWD_HOTP:
		itemIdx = HOTP;
		break;
	case AcntSettings::LIM_UNAME_PWD_TOTP:
		itemIdx = TOTP;
		break;
	default:
		Q_ASSERT(0);
		itemIdx = USER_NAME;
		break;
	}

	m_ui->loginMethodComboBox->setCurrentIndex(itemIdx);
	activateContent(itemIdx);

	m_ui->testAcntCheckBox->setChecked(acntData.isTestAccount());
	m_ui->pwdLine->setText(acntData.password());
	m_ui->rememberPwdCheckBox->setChecked(acntData.rememberPwd());
	m_ui->syncAllCheckBox->setChecked(acntData.syncWithAll());

	if (!acntData.p12File().isEmpty()) {
		m_ui->addCertButton->setText(
		    QDir::toNativeSeparators(acntData.p12File()));
		m_ui->addCertButton->setIcon(
		    QIcon(ICON_3PARTY_PATH + QString("key_16.png")));
		m_certPath = QDir::toNativeSeparators(acntData.p12File());
	}

	checkInputFields();
}


AcntSettings DlgCreateAccount::getContent(void) const
{
	AcntSettings newAcntSettings;

	switch (m_action) {
	case ACT_ADDNEW:
		/*
		 * Set to true only for newly created account.
		 * Don't set it to false when account is edited because a
		 * newly created account can also be edited in case the first
		 * account creation attempt failed.
		 */
		newAcntSettings._setCreatedFromScratch(true);
		break;
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
		{
			const QString userName(m_accountInfo.userName());
			Q_ASSERT(!userName.isEmpty());
			Q_ASSERT(userName == m_ui->usernameLine->text().trimmed());
			newAcntSettings = m_accountInfo;
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* Set account entries. */
	newAcntSettings.setAccountName(m_ui->acntNameLine->text().trimmed());
	newAcntSettings.setUserName(m_ui->usernameLine->text().trimmed());
	newAcntSettings.setTestAccount(m_ui->testAcntCheckBox->isChecked());
	newAcntSettings.setPassword(m_ui->pwdLine->text().trimmed());
	newAcntSettings.setRememberPwd(m_ui->rememberPwdCheckBox->isChecked());
	newAcntSettings.setSyncWithAll(m_ui->syncAllCheckBox->isChecked());

	switch (m_ui->loginMethodComboBox->currentIndex()) {
	case USER_NAME:
		newAcntSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD);
		newAcntSettings.setP12File(QString());
		break;
	case CERTIFICATE:
		newAcntSettings.setLoginMethod(AcntSettings::LIM_UNAME_CRT);
		newAcntSettings.setPassword(QString());
		newAcntSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
		break;
	case USER_CERTIFICATE:
		newAcntSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_CRT);
		newAcntSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
		break;
	case HOTP:
		newAcntSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_HOTP);
		newAcntSettings.setP12File(QString());
		break;
	case TOTP:
		newAcntSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_TOTP);
		newAcntSettings.setP12File(QString());
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return newAcntSettings;
}
