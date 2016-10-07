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

#include "src/common.h"
#include "src/gui/dlg_create_account.h"
#include "src/io/dbs.h"
#include "src/log/log.h"

DlgCreateAccount::DlgCreateAccount(const AcntSettings &accountInfo,
    Action action, QWidget *parent)
    : QDialog(parent),
    m_accountInfo(accountInfo),
    m_action(action),
    m_loginmethod(0),
    m_certPath()
{
	setupUi(this);
	initialiseDialogue();

	/* Set dialogue content for existing account. */
	if (ACT_ADDNEW != m_action) {
		setContent(m_accountInfo);
	}
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
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->rememberPswcheckBox->setEnabled(false);
		break;
	case USER_CERTIFICATE:
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(true);
		this->passwordLineEdit->setEnabled(true);
		this->rememberPswcheckBox->setEnabled(true);
		break;
	default:
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->rememberPswcheckBox->setEnabled(true);
		break;
	}
	this->usernameLineEdit->setEnabled(m_action == ACT_ADDNEW);
	this->testAccountCheckBox->setEnabled(m_action == ACT_ADDNEW);

	checkInputFields();
}

void DlgCreateAccount::checkInputFields(void)
{
	bool buttonEnabled;

	switch (m_loginmethod) {
	case CERTIFICATE:
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	case USER_CERTIFICATE:
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
		break;
	default:
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty();
		break;
	}

	this->accountButtonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}

void DlgCreateAccount::addCertificateFile(void)
{
	QString certFileName = QFileDialog::getOpenFileName(this,
	    tr("Open Certificate"), "",
	    tr("Certificate Files (*.p12 *.pem)"));
	if (!certFileName.isEmpty()) {
		this->addCertificateButton->setText(certFileName);
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
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

	/* create new account / save current account */
	switch (m_action) {
	case ACT_ADDNEW:
		/* Don't do anything. */
		break;
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		{
			const QString userName(m_accountInfo.userName());
			Q_ASSERT(!userName.isEmpty());
			/*
			 * This assignment represents hidden functionality.
			 * Either this must be clearly documented or it must
			 * be removed.
			 *
			 * FIXME -- Do something!
			 */
			AccountModel::globAccounts[userName] = m_accountInfo;
			/*
			 * Catching the following signal is required only when
			 * ACT_EDIT was enabled.
			 *
			 * The account model catches the signal.
			 */
			emit AccountModel::globAccounts.accountDataChanged(userName);
			/* TODO -- Save/update related account DB entry? */
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}

void DlgCreateAccount::initialiseDialogue(void)
{
	this->loginmethodComboBox->addItem(tr("Password"));
	this->loginmethodComboBox->addItem(tr("Certificate"));
	this->loginmethodComboBox->addItem(tr("Certificate + Password"));
	this->loginmethodComboBox->addItem(tr("Password + Secure code"));
	this->loginmethodComboBox->addItem(tr("Password + Secure SMS"));
	this->certificateLabel->setEnabled(false);
	this->accountButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->addCertificateButton->setEnabled(false);

	connect(this->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(activateContent(int)));
	connect(this->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFile()));
	connect(this->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(saveAccount(void)));
	connect(this->accountLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->usernameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->passwordLineEdit, SIGNAL(textChanged(QString)),
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
		windowTitle = tr("Update account") + " " +
		    acntData.accountName();
		break;
	case ACT_PWD:
		windowTitle = tr("Enter password for account") + " "
		    + acntData.accountName();
		this->infoLabel->setEnabled(false);
		this->accountLineEdit->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		break;
	case ACT_CERT:
		windowTitle = tr("Set certificate for account") + " "
		    + acntData.accountName();
		this->infoLabel->setEnabled(false);
		this->accountLineEdit->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		break;
	case ACT_CERTPWD:
		windowTitle = tr("Enter password/certificate for account")
		    + " " + acntData.accountName();
		this->infoLabel->setEnabled(false);
		this->accountLineEdit->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		break;
	case ACT_IDBOX:
		windowTitle = tr("Enter ID of your databox for account")
		    + " " + acntData.accountName();
		this->infoLabel->setEnabled(false);
		this->accountLineEdit->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->usernameLabel->setText(tr("Databox ID:"));
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	this->setWindowTitle(windowTitle);
	this->accountLineEdit->setText(acntData.accountName());
	this->usernameLineEdit->setText(acntData.userName());
	this->usernameLineEdit->setEnabled(false);
	this->testAccountCheckBox->setEnabled(false);

	int itemIdx;
	const QString loginMethod(acntData.loginMethod());
	if (LIM_USERNAME == loginMethod) {
		itemIdx = USER_NAME;
	} else if (LIM_CERT == loginMethod) {
		itemIdx = CERTIFICATE;
	} else if (LIM_USER_CERT == loginMethod) {
		itemIdx = USER_CERTIFICATE;
	} else if (LIM_HOTP == loginMethod) {
		itemIdx = HOTP;
	} else {
		itemIdx = TOTP;
	}

	this->loginmethodComboBox->setCurrentIndex(itemIdx);
	activateContent(itemIdx);

	this->passwordLineEdit->setText(acntData.password());
	this->testAccountCheckBox->setChecked(acntData.isTestAccount());
	this->rememberPswcheckBox->setChecked(acntData.rememberPwd());
	this->synchroCheckBox->setChecked(acntData.syncWithAll());

	if (!acntData.p12File().isEmpty()) {
		this->addCertificateButton->setText(QDir::
		    toNativeSeparators(acntData.p12File()));
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
		m_certPath = QDir::toNativeSeparators(acntData.p12File());
	}

	checkInputFields();
}

AcntSettings DlgCreateAccount::getContent(void) const
{
	AcntSettings newAccountSettings;

	/* set account index, itemTop and account settings for account */
	switch (m_action) {
	case ACT_ADDNEW:
		break;
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		{
			const QString userName(m_accountInfo.userName());
			Q_ASSERT(!userName.isEmpty());
			Q_ASSERT(userName == this->usernameLineEdit->text().trimmed());
			newAccountSettings = AccountModel::globAccounts[userName];
		}
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* set account items */
	newAccountSettings.setAccountName(this->accountLineEdit->text().trimmed());
	newAccountSettings.setUserName(this->usernameLineEdit->text().trimmed());
	newAccountSettings.setRememberPwd(this->rememberPswcheckBox->isChecked());
	newAccountSettings.setPassword(this->passwordLineEdit->text().trimmed());
	newAccountSettings.setTestAccount(this->testAccountCheckBox->isChecked());
	newAccountSettings.setSyncWithAll(this->synchroCheckBox->isChecked());

	if (this->loginmethodComboBox->currentIndex() == USER_NAME) {
		newAccountSettings.setLoginMethod(LIM_USERNAME);
		newAccountSettings.setP12File("");
	} else if (this->loginmethodComboBox->currentIndex() == CERTIFICATE) {
		newAccountSettings.setLoginMethod(LIM_CERT);
		newAccountSettings.setPassword("");
		newAccountSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
	} else if (this->loginmethodComboBox->currentIndex() ==
	           USER_CERTIFICATE) {
		newAccountSettings.setLoginMethod(LIM_USER_CERT);
		newAccountSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
	} else if (this->loginmethodComboBox->currentIndex() == HOTP) {
		newAccountSettings.setLoginMethod(LIM_HOTP);
		newAccountSettings.setP12File("");
	} else if (this->loginmethodComboBox->currentIndex() == TOTP) {
		newAccountSettings.setLoginMethod(LIM_TOTP);
		newAccountSettings.setP12File("");
	} else {
		Q_ASSERT(0);
	}

	/* Only for newly created account. */
	newAccountSettings._setCreatedFromScratch(ACT_ADDNEW == m_action);

	return newAccountSettings;
}
