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


#include "dlg_msg_search.h"

DlgMsgSearch::DlgMsgSearch(const QList<MessageDb*> messageDbList,
    const AccountModel::SettingsMap &accountInfo, QWidget *parent)
    : QDialog(parent),
    m_messageDbList(messageDbList),
    m_accountInfo(accountInfo)
{
	setupUi(this);
	initSearchWindow();
}


/* ========================================================================= */
/*
 * Init message search dialog
 */
void DlgMsgSearch::initSearchWindow(void)
/* ========================================================================= */
{
	/* set account name and user name to label */
	QString accountName = m_accountInfo.accountName()
	+ " ("+ m_accountInfo.userName() + ")";
	this->currentAccountName->setText(accountName);

	/* is only one account available */
	if (m_messageDbList.count() <= 1) {
		this->searchAllAcntCheckBox->setEnabled(false);
	}

	connect(this->searchReceivedMsgCheckBox, SIGNAL(clicked()),
	    this, SLOT(checkInputFields()));
	connect(this->searchSentMsgCheckBox, SIGNAL(clicked()),
	    this, SLOT(checkInputFields()));
	connect(this->messageIdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->senderDbIdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->senderNameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->recipientDbIdLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->recipientNameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchMessage()));

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}


/* ========================================================================= */
/*
 * Check dialogue elements and set search button enable/disable
 */
void DlgMsgSearch::checkInputFields(void)
/* ========================================================================= */
{
	this->searchPushButton->setEnabled(false);

	/* is any message type checked? */
	if (!this->searchReceivedMsgCheckBox->isChecked() &&
	    !this->searchSentMsgCheckBox->isChecked()) {
		return;
	}

	/* search via message ID */
	if (!this->messageIdLineEdit->text().isEmpty()) {
		this->subjectLineEdit->setEnabled(false);
		this->senderDbIdLineEdit->setEnabled(false);
		this->senderNameLineEdit->setEnabled(false);
		this->senderRefNumLineEdit->setEnabled(false);
		this->senderFileMarkLineEdit->setEnabled(false);
		this->recipientDbIdLineEdit->setEnabled(false);
		this->recipientNameLineEdit->setEnabled(false);
		this->recipientRefNumLineEdit->setEnabled(false);
		this->recipientFileMarkLineEdit->setEnabled(false);
		this->addressLineEdit->setEnabled(false);
		this->toHandsLineEdit->setEnabled(false);
		/* message ID > 3 chars */
		if (this->messageIdLineEdit->text().size() > 2) {
			this->searchPushButton->setEnabled(true);
		} else {
			this->searchPushButton->setEnabled(false);
		}
		return;
	} else {
		this->subjectLineEdit->setEnabled(true);
		this->senderDbIdLineEdit->setEnabled(true);
		this->senderNameLineEdit->setEnabled(true);
		this->senderRefNumLineEdit->setEnabled(true);
		this->senderFileMarkLineEdit->setEnabled(true);
		this->recipientDbIdLineEdit->setEnabled(true);
		this->recipientNameLineEdit->setEnabled(true);
		this->recipientRefNumLineEdit->setEnabled(true);
		this->recipientFileMarkLineEdit->setEnabled(true);
		this->addressLineEdit->setEnabled(true);
		this->toHandsLineEdit->setEnabled(true);
		this->searchPushButton->setEnabled(true);
		this->searchPushButton->setEnabled(false);
	}

	/* search via sender databox ID */
	if (!this->senderDbIdLineEdit->text().isEmpty()) {
		this->senderDbIdLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(false);
		this->senderNameLineEdit->setEnabled(false);
		/* databox ID == 7 chars */
		if (this->senderDbIdLineEdit->text().size() == 7) {
			this->searchPushButton->setEnabled(true);
		} else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->senderNameLineEdit->text().isEmpty()){
		this->senderNameLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(false);
		this->senderDbIdLineEdit->setEnabled(false);
		/* sender name > 3 chars */
		if (this->senderNameLineEdit->text().size() > 2) {
			this->searchPushButton->setEnabled(true);
		} else {
			this->searchPushButton->setEnabled(false);
		}
	} else {
		this->senderNameLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(true);
		this->senderDbIdLineEdit->setEnabled(true);
		this->searchPushButton->setEnabled(false);
	}


	/* search via recipient databox ID */
	if (!this->recipientDbIdLineEdit->text().isEmpty()) {
		this->recipientDbIdLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(false);
		this->recipientNameLineEdit->setEnabled(false);
		/* databox ID == 7 chars */
		if (this->recipientDbIdLineEdit->text().size() == 7) {
			this->searchPushButton->setEnabled(true);
		} else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->recipientNameLineEdit->text().isEmpty()){
		this->recipientNameLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(false);
		this->recipientDbIdLineEdit->setEnabled(false);
		/* recipient name > 3 chars */
		if (this->recipientNameLineEdit->text().size() > 2) {
			this->searchPushButton->setEnabled(true);
		} else {
			this->searchPushButton->setEnabled(false);
		}
	} else {
		this->recipientNameLineEdit->setEnabled(true);
		this->messageIdLineEdit->setEnabled(true);
		this->recipientDbIdLineEdit->setEnabled(true);
		this->searchPushButton->setEnabled(false);
	}
}


/* ========================================================================= */
/*
 * Search message
 */
void DlgMsgSearch::searchMessage(void)
/* ========================================================================= */
{
	qDebug() << "searchMessage";
}
