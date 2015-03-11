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

DlgMsgSearch::DlgMsgSearch(
    const QList< QPair <QString,MessageDb*> > messageDbList,
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
	this->infoTextLabel->setText(
	   tr("Here it is possible to search for messages according to "
	   "supplied criteria. You can search for messages in selected "
	   "account or in all accounts. Double clicking on a found message "
	   "will change focus to the selected message in the application "
	   "window. Note: You can view additional information when hovering "
	   "your mouse cursor over the message ID."));

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

	/* hide first column of resultWidget (userName string) */
	//this->resultsTableWidget->setColumnHidden(0,true);

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
	    SLOT(searchMessages()));
	connect(this->resultsTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));
	connect(this->resultsTableWidget,
	    SIGNAL(cellDoubleClicked(int,int)), this,
	    SLOT(getSelectedMsg(int, int)));

	this->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
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
 * Set first column with checkbox active if item was changed
 */
void DlgMsgSearch::setFirtsColumnActive(void)
/* ========================================================================= */
{
	this->resultsTableWidget->selectColumn(0);
	this->resultsTableWidget->selectRow(
	    this->resultsTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Search message
 */
void DlgMsgSearch::searchMessages(void)
/* ========================================================================= */
{
	qDebug() << "searchMessage";

	enum MessageDirection msgType = MSG_ALL;
	QList <QStringList> msgList;

	this->resultsTableWidget->setRowCount(0);
	this->resultsTableWidget->setEnabled(false);

	if (this->searchReceivedMsgCheckBox->isChecked() &&
	    this->searchSentMsgCheckBox->isChecked()) {
		msgType = MSG_ALL;
	} else if (this->searchReceivedMsgCheckBox->isChecked()) {
		msgType = MSG_RECEIVED;
	} else if (this->searchSentMsgCheckBox->isChecked()) {
		msgType = MSG_SENT;
	}

	if (!this->searchAllAcntCheckBox->isChecked()) {
		msgList.clear();
		msgList = m_messageDbList.at(0).second->
		    msgsAdvancedSearchMessageEnvelope(
		    this->messageIdLineEdit->text().isEmpty() ? -1 :
		        this->messageIdLineEdit->text().toLongLong(),
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
		if (!msgList.isEmpty()) {
			appendMsgsToTable(m_messageDbList.at(0), msgList);
		}
	} else {
		for (int i = 0; i < m_messageDbList.count(); ++i) {
			msgList.clear();
			msgList = m_messageDbList.at(i).second->
			    msgsAdvancedSearchMessageEnvelope(
			    this->messageIdLineEdit->text().isEmpty() ? -1 :
			        this->messageIdLineEdit->text().toLongLong(),
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
			if (!msgList.isEmpty()) {
				appendMsgsToTable(m_messageDbList.at(i),
				    msgList);
			}
		}
	}
}


/* ========================================================================= */
/*
 * Append message list to result tablewidget
 */
void DlgMsgSearch::appendMsgsToTable(
    QPair <QString,MessageDb*> usrNmAndMsgDb,
    QList <QStringList> msgList)
/* ========================================================================= */
{
	this->resultsTableWidget->setEnabled(true);

	for (int j = 0; j < msgList.count(); ++j) {
		int row = this->resultsTableWidget->rowCount();
		this->resultsTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(usrNmAndMsgDb.first);
		this->resultsTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(msgList.at(j).at(0));
		if (ENABLE_TOOLTIP) {
			item->setToolTip(usrNmAndMsgDb.second->descriptionHtml(
			    msgList.at(j).at(0).toInt(), 0));
		}
		this->resultsTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(msgList.at(j).at(1));
		this->resultsTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(msgList.at(j).at(2));
		this->resultsTableWidget->setItem(row,3,item);
		item = new QTableWidgetItem;
		item->setText(msgList.at(j).at(3));
		this->resultsTableWidget->setItem(row,4,item);
	}

	this->resultsTableWidget->resizeColumnsToContents();
	this->resultsTableWidget->
	    horizontalHeader()->setStretchLastSection(true);
}


/* ========================================================================= */
/*
 * Get ID of selected message and set focus in MessageList Tableview
 */
void DlgMsgSearch::getSelectedMsg(int row, int column)
/* ========================================================================= */
{
	(void) column;
	emit focusSelectedMsg(this->resultsTableWidget->item(row, 0)->text(),
	    this->resultsTableWidget->item(row, 1)->text().toLongLong());
	//this->close();
}
