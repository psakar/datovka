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
		m_ui->certificateLabel->setEnabled(true);
		m_ui->addCertificateButton->setEnabled(true);
		m_ui->passwordLabel->setEnabled(false);
		m_ui->passwordLineEdit->setEnabled(false);
		m_ui->rememberPswcheckBox->setEnabled(false);
		break;
	case USER_CERTIFICATE:
		m_ui->certificateLabel->setEnabled(true);
		m_ui->addCertificateButton->setEnabled(true);
		m_ui->passwordLabel->setEnabled(true);
		m_ui->passwordLineEdit->setEnabled(true);
		m_ui->rememberPswcheckBox->setEnabled(true);
		break;
	default:
		m_ui->certificateLabel->setEnabled(false);
		m_ui->addCertificateButton->setEnabled(false);
		m_ui->passwordLabel->setEnabled((true));
		m_ui->passwordLineEdit->setEnabled((true));
		m_ui->rememberPswcheckBox->setEnabled(true);
		break;
	}
	m_ui->usernameLineEdit->setEnabled(m_action == ACT_ADDNEW);
	m_ui->testAccountCheckBox->setEnabled(m_action == ACT_ADDNEW);

	checkInputFields();
}

void DlgCreateAccount::checkInputFields(void)
{
	bool buttonEnabled;

	switch (m_loginmethod) {
	case CERTIFICATE:
		buttonEnabled = !m_ui->accountLineEdit->text().isEmpty()
		    && !m_ui->usernameLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	case USER_CERTIFICATE:
		buttonEnabled = !m_ui->accountLineEdit->text().isEmpty()
		    && !m_ui->usernameLineEdit->text().isEmpty()
		    && !m_ui->passwordLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	default:
		buttonEnabled = !m_ui->accountLineEdit->text().isEmpty()
		    && !m_ui->usernameLineEdit->text().isEmpty()
		    && !m_ui->passwordLineEdit->text().isEmpty();
		break;
	}

	m_ui->accountButtonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}

void DlgCreateAccount::addCertificateFile(void)
{
	QString certFileName = QFileDialog::getOpenFileName(this,
	    tr("Open Certificate"), "",
	    tr("Certificate Files (*.p12 *.pem)"));
	if (!certFileName.isEmpty()) {
		m_ui->addCertificateButton->setText(certFileName);
		m_ui->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
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
	m_ui->loginmethodComboBox->addItem(tr("Password"));
	m_ui->loginmethodComboBox->addItem(tr("Certificate"));
	m_ui->loginmethodComboBox->addItem(tr("Certificate + Password"));
	m_ui->loginmethodComboBox->addItem(tr("Password + Secure code"));
	m_ui->loginmethodComboBox->addItem(tr("Password + Secure SMS"));
	m_ui->certificateLabel->setEnabled(false);
	m_ui->accountButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	m_ui->addCertificateButton->setEnabled(false);

	connect(m_ui->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(activateContent(int)));
	connect(m_ui->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFile()));
	connect(m_ui->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(saveAccount(void)));
	connect(m_ui->accountLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->usernameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)),
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
		m_ui->accountLineEdit->setEnabled(false);
		m_ui->loginmethodComboBox->setEnabled(false);
		m_ui->addCertificateButton->setEnabled(false);
		break;
	case ACT_CERT:
		windowTitle = tr("Set certificate for account %1")
		    .arg(acntData.accountName());
		m_ui->infoLabel->setEnabled(false);
		m_ui->accountLineEdit->setEnabled(false);
		m_ui->loginmethodComboBox->setEnabled(false);
		m_ui->passwordLineEdit->setEnabled(false);
		break;
	case ACT_CERTPWD:
		windowTitle = tr("Enter password/certificate for account %1")
		    .arg(acntData.accountName());
		m_ui->infoLabel->setEnabled(false);
		m_ui->accountLineEdit->setEnabled(false);
		m_ui->loginmethodComboBox->setEnabled(false);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	setWindowTitle(windowTitle);
	m_ui->accountLineEdit->setText(acntData.accountName());
	m_ui->usernameLineEdit->setText(acntData.userName());
	m_ui->usernameLineEdit->setEnabled(false);
	m_ui->testAccountCheckBox->setEnabled(false);

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

	m_ui->loginmethodComboBox->setCurrentIndex(itemIdx);
	activateContent(itemIdx);

	m_ui->passwordLineEdit->setText(acntData.password());
	m_ui->testAccountCheckBox->setChecked(acntData.isTestAccount());
	m_ui->rememberPswcheckBox->setChecked(acntData.rememberPwd());
	m_ui->synchroCheckBox->setChecked(acntData.syncWithAll());

	if (!acntData.p12File().isEmpty()) {
		m_ui->addCertificateButton->setText(
		    QDir::toNativeSeparators(acntData.p12File()));
		m_ui->addCertificateButton->setIcon(
		    QIcon(ICON_3PARTY_PATH + QString("key_16.png")));
		m_certPath = QDir::toNativeSeparators(acntData.p12File());
	}

	checkInputFields();
}


AcntSettings DlgCreateAccount::getContent(void) const
{
	AcntSettings newAccountSettings;

	switch (m_action) {
	case ACT_ADDNEW:
		/*
		 * Set to true only for newly created account.
		 * Don't set it to false when account is edited because a
		 * newly created account can also be edited in case the first
		 * account creation attempt failed.
		 */
		newAccountSettings._setCreatedFromScratch(true);
		break;
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
		{
			const QString userName(m_accountInfo.userName());
			Q_ASSERT(!userName.isEmpty());
			Q_ASSERT(userName == m_ui->usernameLineEdit->text().trimmed());
			newAccountSettings = m_accountInfo;
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* set account items */
	newAccountSettings.setAccountName(m_ui->accountLineEdit->text().trimmed());
	newAccountSettings.setUserName(m_ui->usernameLineEdit->text().trimmed());
	newAccountSettings.setRememberPwd(m_ui->rememberPswcheckBox->isChecked());
	newAccountSettings.setPassword(m_ui->passwordLineEdit->text().trimmed());
	newAccountSettings.setTestAccount(m_ui->testAccountCheckBox->isChecked());
	newAccountSettings.setSyncWithAll(m_ui->synchroCheckBox->isChecked());

	switch (m_ui->loginmethodComboBox->currentIndex()) {
	case USER_NAME:
		newAccountSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD);
		newAccountSettings.setP12File(QString());
		break;
	case CERTIFICATE:
		newAccountSettings.setLoginMethod(AcntSettings::LIM_UNAME_CRT);
		newAccountSettings.setPassword(QString());
		newAccountSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
		break;
	case USER_CERTIFICATE:
		newAccountSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_CRT);
		newAccountSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
		break;
	case HOTP:
		newAccountSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_HOTP);
		newAccountSettings.setP12File(QString());
		break;
	case TOTP:
		newAccountSettings.setLoginMethod(AcntSettings::LIM_UNAME_PWD_TOTP);
		newAccountSettings.setP12File(QString());
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return newAccountSettings;
}
