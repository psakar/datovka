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

	this->tooMuchFields->setStyleSheet("QLabel { color: red }");
	this->tooMuchFields->hide();

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
	connect(this->subjectLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->toHandsLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->addressLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->senderRefNumLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->senderFileMarkLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->recipientRefNumLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->recipientFileMarkLineEdit, SIGNAL(textChanged(QString)),
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
	//qDebug() << "checkInputFields";

	bool messageType = true;

	this->searchPushButton->setEnabled(false);
	this->tooMuchFields->hide();

	/* is any message type checked? */
	if (!this->searchReceivedMsgCheckBox->isChecked() &&
	    !this->searchSentMsgCheckBox->isChecked()) {
		messageType = false;
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
		goto finish;
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
	}

	/* search via sender databox ID */
	if (!this->senderDbIdLineEdit->text().isEmpty()) {
		this->senderDbIdLineEdit->setEnabled(true);
		this->senderNameLineEdit->setEnabled(false);
	} else if (!this->senderNameLineEdit->text().isEmpty()){
		this->senderDbIdLineEdit->setEnabled(false);
		this->senderNameLineEdit->setEnabled(true);
	} else {
		this->senderNameLineEdit->setEnabled(true);
		this->senderDbIdLineEdit->setEnabled(true);
	}

	/* search via recipient databox ID */
	if (!this->recipientDbIdLineEdit->text().isEmpty()) {
		this->recipientDbIdLineEdit->setEnabled(true);
		this->recipientNameLineEdit->setEnabled(false);
	} else if (!this->recipientNameLineEdit->text().isEmpty()){
		this->recipientDbIdLineEdit->setEnabled(false);
		this->recipientNameLineEdit->setEnabled(true);
	} else {
		this->recipientNameLineEdit->setEnabled(true);
		this->recipientDbIdLineEdit->setEnabled(true);
	}

finish:
	/* search by message ID */
	if (!this->messageIdLineEdit->text().isEmpty()) {
		/* test if message ID is number */
		QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
		/* test if message is fill and message ID > 3 chars */
		if (messageType &&
		    (re.exactMatch(this->messageIdLineEdit->text())) &&
		    this->messageIdLineEdit->text().size() > 2) {
			this->searchPushButton->setEnabled(true);
			return;
		} else {
			this->searchPushButton->setEnabled(false);
			return;
		}
	}

	bool isDbIdCorrect = false;
	if (!this->senderDbIdLineEdit->text().isEmpty() &&
	    !this->recipientDbIdLineEdit->text().isEmpty()) {
		if (this->senderDbIdLineEdit->text().size()==7 &&
		    this->recipientDbIdLineEdit->text().size() == 7) {
			isDbIdCorrect = true;
		}
	} else if (!this->senderDbIdLineEdit->text().isEmpty()) {
		/* databox ID must have 7 chars */
		if (this->senderDbIdLineEdit->text().size() == 7) {
			isDbIdCorrect = true;
		}
	} else if (!this->recipientDbIdLineEdit->text().isEmpty()) {
		/* databox ID must have 7 chars */
		if (messageType &&
		    this->recipientDbIdLineEdit->text().size() == 7) {
			isDbIdCorrect = true;
		}
	} else {
		if (messageType) {
			int fields = howManyFieldsAreFill();
			if ((fields > 0) && (fields < 4)) {
				this->searchPushButton->setEnabled(true);
			} else {
				this->searchPushButton->setEnabled(false);
				if (fields != 0) {
					this->tooMuchFields->show();
				}
			}
		} else {
			this->searchPushButton->setEnabled(false);
		}
		return;
	}

	if (messageType && isDbIdCorrect) {
		this->searchPushButton->setEnabled(true);
		return;
	} else {
		this->searchPushButton->setEnabled(false);
		return;
	}
}


/* ========================================================================= */
/*
 * Detect, how many search fileds are filled
 */
int DlgMsgSearch::howManyFieldsAreFill(void)
/* ========================================================================= */
{
	int cnt = 0;

	if (!this->subjectLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->senderDbIdLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->senderNameLineEdit->text().isEmpty())  {
		cnt++;
	}
	if (!this->addressLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->recipientDbIdLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->recipientNameLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->addressLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->senderRefNumLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->senderFileMarkLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->recipientRefNumLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->recipientFileMarkLineEdit->text().isEmpty()) {
		cnt++;
	}
	if (!this->toHandsLineEdit->text().isEmpty()) {
		cnt++;
	}

	return cnt;
}

/* ========================================================================= */
/*
 * Search message
 */
void DlgMsgSearch::searchMessage(void)
/* ========================================================================= */
{
	qDebug() << "searchMessage";

	int msgType = 0;

	if (this->searchReceivedMsgCheckBox->isChecked() &&
	    this->searchSentMsgCheckBox->isChecked()) {
		msgType = 3;
	} else if (this->searchReceivedMsgCheckBox->isChecked()) {
		msgType = 1;
	} else if (this->searchSentMsgCheckBox->isChecked()) {
		msgType = 2;
	}

	QStringList dmIDList;
	dmIDList = m_messageDbList.at(0)->msgsAdvanceSearchMessageEnvelope(
	    this->messageIdLineEdit->text(),
	    this->subjectLineEdit->text(),
	    this->senderDbIdLineEdit->text(),
	    this->senderNameLineEdit->text(),
	    this->addressLineEdit->text(),
	    this->recipientDbIdLineEdit->text(),
	    this->recipientNameLineEdit->text(),
	    this->addressLineEdit->text(),
	    this->senderRefNumLineEdit->text(),
	    this->senderFileMarkLineEdit->text(),
	    this->recipientRefNumLineEdit->text(),
	    this->recipientFileMarkLineEdit->text(),
	    this->toHandsLineEdit->text(),
	    QString(), QString(), msgType);

	for (int i = 0; i < dmIDList.size(); i++) {
		qDebug() << dmIDList.at(i);
	}
}
