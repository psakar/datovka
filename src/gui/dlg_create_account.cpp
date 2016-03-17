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

#include "dlg_create_account.h"
#include "src/io/dbs.h"
#include "src/log/log.h"


DlgCreateAccount::DlgCreateAccount(const AcntSettings &accountInfo,
    Action action, QWidget *parent)
    : QDialog(parent),
    m_accountInfo(accountInfo),
    m_action(action),
    m_loginmethod(0),
    m_certPath("")
{
	setupUi(this);
	initAccountDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgCreateAccount::initAccountDialog(void)
/* ========================================================================= */
{
	this->loginmethodComboBox->addItem(tr("Password"));
	this->loginmethodComboBox->addItem(tr("Certificate"));
	this->loginmethodComboBox->addItem(tr("Certificate + Password"));
	this->loginmethodComboBox->addItem(tr("Password + Secure code"));
	this->loginmethodComboBox->addItem(tr("Password + Secure SMS"));
	this->certificateLabel->setEnabled(false);
	this->accountButtonBox->button(
	    QDialogButtonBox::Ok)->setEnabled(false);
	this->addCertificateButton->setEnabled(false);
	connect(this->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(setActiveButton(int)));
	connect(this->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFromFile()));
	connect(this->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(saveAccount(void)));
	connect(this->accountLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->usernameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->passwordLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	/* if account exists then we set all items */
	if (ACT_ADDNEW != m_action) {
		setCurrentAccountData();
	}
}


/* ========================================================================= */
/*
 * Set current account data from dsgui.conf (exist account edit)
 */
void DlgCreateAccount::setCurrentAccountData(void)
/* ========================================================================= */
{
	int itemindex;

	if (m_accountInfo.userName().isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	switch (m_action) {
	case ACT_EDIT:
		this->setWindowTitle(tr("Update account") + " "
		    + m_accountInfo.accountName());
		this->accountLineEdit->setText(m_accountInfo.accountName());
		this->usernameLineEdit->setText(m_accountInfo.userName());
		this->usernameLineEdit->setEnabled(false);
		this->testAccountCheckBox->setEnabled(false);
		break;
	case ACT_PWD:
		this->setWindowTitle(tr("Enter password for account") + " "
		    + m_accountInfo.accountName());
		this->accountLineEdit->setText(m_accountInfo.accountName());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(m_accountInfo.userName());
		this->usernameLineEdit->setEnabled(false);
		this->testAccountCheckBox->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		break;
	case ACT_CERT:
		this->setWindowTitle(tr("Set certificate for account") + " "
		    + m_accountInfo.accountName());
		this->accountLineEdit->setText(m_accountInfo.accountName());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(m_accountInfo.userName());
		this->usernameLineEdit->setEnabled(false);
		this->testAccountCheckBox->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		break;
	case ACT_CERTPWD:
		this->setWindowTitle(tr("Enter password/certificate for account")
		    + " " + m_accountInfo.accountName());
		this->accountLineEdit->setText(m_accountInfo.accountName());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(m_accountInfo.userName());
		this->usernameLineEdit->setEnabled(false);
		this->testAccountCheckBox->setEnabled(false);
		break;
	case ACT_IDBOX:
		this->setWindowTitle(tr("Enter ID of your databox for account")
		    + " " + m_accountInfo.accountName());
		this->accountLineEdit->setText(m_accountInfo.accountName());
		this->accountLineEdit->setEnabled(false);
		this->infoLabel->setEnabled(false);
		this->loginmethodComboBox->setEnabled(false);
		this->usernameLineEdit->setText(m_accountInfo.userName());
		this->testAccountCheckBox->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->usernameLabel->setText(tr("Databox ID:"));
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	const QString login_method = m_accountInfo.loginMethod();
	if (LIM_USERNAME == login_method) {
		itemindex = USER_NAME;
	} else if (LIM_CERT == login_method) {
		itemindex = CERTIFICATE;
	} else if (LIM_USER_CERT == login_method) {
		itemindex = USER_CERTIFICATE;
	} else if (LIM_HOTP == login_method) {
		itemindex = HOTP;
	} else {
		itemindex = TOTP;
	}

	this->loginmethodComboBox->setCurrentIndex(itemindex);
	setActiveButton(itemindex);

	this->passwordLineEdit->setText(m_accountInfo.password());
	this->testAccountCheckBox->setChecked(m_accountInfo.isTestAccount());
	this->rememberPswcheckBox->setChecked(m_accountInfo.rememberPwd());
	this->synchroCheckBox->setChecked(m_accountInfo.syncWithAll());

	if (!m_accountInfo.p12File().isEmpty()) {
		this->addCertificateButton->setText(QDir::
		    toNativeSeparators(m_accountInfo.p12File()));
		this->addCertificateButton->setIcon(QIcon(ICON_3PARTY_PATH +
		QString("key_16.png")));
		m_certPath = QDir::toNativeSeparators(m_accountInfo.p12File());
	}

	checkInputFields();
}


/* ========================================================================= */
/*
 * Open load dialog and set certificate file path
 */
void DlgCreateAccount::addCertificateFromFile(void)
/* ========================================================================= */
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


/* ========================================================================= */
/*
 *  Check content of mandatory items in dialog and activate save button
 */
void DlgCreateAccount::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled;
	if (m_loginmethod == CERTIFICATE) {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
	} else if (m_loginmethod == USER_CERTIFICATE) {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty()
		    && !m_certPath.isEmpty();
	} else {
		buttonEnabled = !this->accountLineEdit->text().isEmpty()
		    && !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty();
	}
	this->accountButtonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}


/* ========================================================================= */
/*
 * Set active/inactive buttons based on login method
 */
void DlgCreateAccount::setActiveButton(int itemindex)
/* ========================================================================= */
{
	if (itemindex == CERTIFICATE) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->rememberPswcheckBox->setEnabled(false);
	} else if (itemindex == USER_CERTIFICATE) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(true);
		this->passwordLineEdit->setEnabled(true);
		this->rememberPswcheckBox->setEnabled(true);
	} else {
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->rememberPswcheckBox->setEnabled(true);
	}
	this->usernameLineEdit->setEnabled(m_action == ACT_ADDNEW);
	this->testAccountCheckBox->setEnabled(m_action == ACT_ADDNEW);

	m_loginmethod = itemindex;
	checkInputFields();
}


/* ========================================================================= */
/*
 *  Create new or save account into dsgui.conf
 */
void DlgCreateAccount::saveAccount(void)
/* ========================================================================= */
{
	debugSlotCall();

	AcntSettings itemSettings;
	const QString userName(m_accountInfo.userName());

	/* set account index, itemTop and map itemSettings for account */
	switch (m_action) {
	case ACT_ADDNEW:
		break;
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		Q_ASSERT(!userName.isEmpty());
		Q_ASSERT(userName == this->usernameLineEdit->text());
		itemSettings = AccountModel::globAccounts[userName];
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	/* set account items */
	itemSettings.setAccountName(this->accountLineEdit->text());
	itemSettings.setUserName(this->usernameLineEdit->text());
	itemSettings.setRememberPwd(this->rememberPswcheckBox->isChecked());
	itemSettings.setPassword(this->passwordLineEdit->text());
	itemSettings.setTestAccount(this->testAccountCheckBox->isChecked());
	itemSettings.setSyncWithAll(this->synchroCheckBox->isChecked());

	if (this->loginmethodComboBox->currentIndex() == USER_NAME) {
		itemSettings.setLoginMethod(LIM_USERNAME);
		itemSettings.setP12File("");
	} else if (this->loginmethodComboBox->currentIndex() == CERTIFICATE) {
		itemSettings.setLoginMethod(LIM_CERT);
		itemSettings.setPassword("");
		itemSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
	} else if (this->loginmethodComboBox->currentIndex() ==
	           USER_CERTIFICATE) {
		itemSettings.setLoginMethod(LIM_USER_CERT);
		itemSettings.setP12File(
		    QDir::fromNativeSeparators(m_certPath));
	} else if (this->loginmethodComboBox->currentIndex() == HOTP) {
		itemSettings.setLoginMethod(LIM_HOTP);
		itemSettings.setP12File("");
	} else if (this->loginmethodComboBox->currentIndex() == TOTP) {
		itemSettings.setLoginMethod(LIM_TOTP);
		itemSettings.setP12File("");
	} else {
		Q_ASSERT(0);
	}

	/* Only for newly created account. */
	itemSettings._setCreatedFromScratch(ACT_ADDNEW == m_action);

	/* create new account / save current account */
	switch (m_action) {
	case ACT_EDIT:
	case ACT_PWD:
	case ACT_CERT:
	case ACT_CERTPWD:
	case ACT_IDBOX:
		AccountModel::globAccounts[userName] = itemSettings;
		/*
		 * Catching the following signal is required only when
		 * ACT_EDIT was enabled.
		 *
		 * The account model catches the signal.
		 */
		emit AccountModel::globAccounts.accountDataChanged(userName);
		/* TODO -- Save/update related account DB entry? */
		break;
	case ACT_ADDNEW:
		emit getAccountUserDataboxInfo(itemSettings);
		break;
	default:
		Q_ASSERT(0);
		break;
	}
}
