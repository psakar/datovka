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

#include <QFileDialog>

#include "dlg_login_mojeid.h"
#include "src/log/log.h"
#include "src/web/json.h"

DlgLoginToMojeId::DlgLoginToMojeId(const QString &userName,
    const QString &lastUrl, QWidget *parent)
    : QDialog(parent),
    m_loginmethod(0),
    m_certPath(""),
    m_userName(userName),
    m_lastUrl(lastUrl),
    m_token(QString())
{
	setupUi(this);
	initAccountDialog();
}


/* ========================================================================= */
/*
 * Init dialog
 */
void DlgLoginToMojeId::initAccountDialog(void)
/* ========================================================================= */
{
	this->loginmethodComboBox->addItem(tr("Password"));
	this->loginmethodComboBox->addItem(tr("Certificate"));
	this->loginmethodComboBox->addItem(tr("Password + Secure code"));
	this->certificateLabel->setEnabled(false);
	this->otpLineEdit->setEnabled(false);
	this->accountButtonBox->button(
	    QDialogButtonBox::Ok)->setEnabled(false);
	this->addCertificateButton->setEnabled(false);
	connect(this->loginmethodComboBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(setActiveButton(int)));
	connect(this->addCertificateButton, SIGNAL(clicked()), this,
	    SLOT(addCertificateFromFile()));
	connect(this->accountButtonBox, SIGNAL(accepted()), this,
	    SLOT(sendData(void)));
	connect(this->usernameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->passwordLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->otpLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	if (!m_userName.isEmpty()) {
		this->synchroCheckBox->setEnabled(false);
		this->setWindowTitle(tr("Login to account: %1").arg(m_userName));
	}

	/* USERNAME+PWD is set as default login method in the Webdatovka */
	jsonlayer.loginMethodChanged(USER_NAME, m_lastUrl, m_token);
}


/* ========================================================================= */
/*
 * Open load dialog and set certificate file path
 */
void DlgLoginToMojeId::addCertificateFromFile(void)
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
void DlgLoginToMojeId::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled;
	if (m_loginmethod == CERTIFICATE) {
		buttonEnabled = !m_certPath.isEmpty();
	} else if (m_loginmethod == USER_NAME) {
		buttonEnabled = !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty();
	} else {
		buttonEnabled = !this->usernameLineEdit->text().isEmpty()
		    && !this->passwordLineEdit->text().isEmpty()
		    && !this->otpLineEdit->text().isEmpty();
	}

	this->accountButtonBox->button(QDialogButtonBox::Ok)->
	    setEnabled(buttonEnabled);
}


/* ========================================================================= */
/*
 * Set active/inactive buttons based on login method
 */
void DlgLoginToMojeId::setActiveButton(int itemindex)
/* ========================================================================= */
{
	this->usernameLineEdit->setEnabled(true);

	if (itemindex == CERTIFICATE) {
		this->certificateLabel->setEnabled(true);
		this->addCertificateButton->setEnabled(true);
		this->passwordLabel->setEnabled(false);
		this->passwordLineEdit->setEnabled(false);
		this->otpLineEdit->setEnabled(false);
		this->otpLineEdit->clear();
		this->passwordLineEdit->clear();
	} else if (itemindex == USER_NAME) {
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->otpLineEdit->setEnabled(false);
		this->otpLineEdit->clear();
	} else {
		this->certificateLabel->setEnabled(false);
		this->addCertificateButton->setEnabled(false);
		this->passwordLabel->setEnabled((true));
		this->passwordLineEdit->setEnabled((true));
		this->otpLineEdit->setEnabled(true);
	}

	m_loginmethod = itemindex;

	jsonlayer.loginMethodChanged(m_loginmethod, m_lastUrl, m_token);

	checkInputFields();
}


/* ========================================================================= */
/*
 *  Login to mojeID.
 */
void DlgLoginToMojeId::sendData(void)
/* ========================================================================= */
{
	debugSlotCall();

	emit callMojeId(m_userName, m_lastUrl,
	    m_token, this->usernameLineEdit->text(),
	    this->passwordLineEdit->text(), this->otpLineEdit->text(),
	    this->synchroCheckBox->isChecked(), m_certPath);
}