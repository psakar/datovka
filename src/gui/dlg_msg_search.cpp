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

#include "src/gui/dlg_msg_search.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/views/table_home_end_filter.h"
#include "ui_dlg_msg_search.h"

#define COL_USER_NAME 0
#define COL_MESSAGE_ID 1
#define COL_DELIVERY_YEAR 2
#define COL_MESSAGE_TYPE 3
#define COL_ANNOTATION 4
#define COL_SENDER 5
#define COL_RECIPIENT 6

/* Tooltip is generated for every item in the search result table. */
#define ENABLE_TOOLTIP 1

DlgMsgSearch::DlgMsgSearch(
    const QList< QPair <QString, MessageDbSet *> > messageDbSetList,
    const QString &userName,
    QWidget *parent, Qt::WindowFlags f)
    : QDialog(parent, f),
    m_ui(new (std::nothrow) Ui::DlgMsgSearch),
    m_messageDbSetList(messageDbSetList)
{
	m_ui->setupUi(this);

	/* Set default line height for table views/widgets. */
	m_ui->resultsTableWidget->setNarrowedLineHeight();

	initSearchWindow(userName);
}

DlgMsgSearch::~DlgMsgSearch(void)
{
	delete m_ui;
}

void DlgMsgSearch::initSearchWindow(const QString &username)
{
	m_ui->infoTextLabel->setText(
	   tr("Here it is possible to search for messages according to "
	   "supplied criteria. You can search for messages in selected "
	   "account or in all accounts. Double clicking on a found message "
	   "will change focus to the selected message in the application "
	   "window. Note: You can view additional information when hovering "
	   "your mouse cursor over the message ID."));

	Q_ASSERT(!username.isEmpty());

	/* Set account name and user name to label. */
	m_ui->crntAcntNameLabel->setText(globAccounts[username].accountName() +
	    " (" + username + ")");

	/* Only one account available. */
	if (m_messageDbSetList.count() <= 1) {
		m_ui->searchAllAcntCheckBox->setEnabled(false);
	}

	m_ui->tooManyFields->setStyleSheet("QLabel { color: red }");
	m_ui->tooManyFields->hide();

	m_ui->resultsTableWidget->setColumnCount(7);
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_USER_NAME, new QTableWidgetItem(tr("Account")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_MESSAGE_ID, new QTableWidgetItem(tr("Message ID")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_ANNOTATION, new QTableWidgetItem(tr("Subject")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_SENDER, new QTableWidgetItem(tr("Sender")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_RECIPIENT, new QTableWidgetItem(tr("Recipient")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_DELIVERY_YEAR, new QTableWidgetItem(tr("Delivery Year")));
	m_ui->resultsTableWidget->setHorizontalHeaderItem(COL_MESSAGE_TYPE, new QTableWidgetItem(tr("Message Type")));

	/* Hide column with delivery time and message type. */
	m_ui->resultsTableWidget->setColumnHidden(COL_DELIVERY_YEAR, true);
	m_ui->resultsTableWidget->setColumnHidden(COL_MESSAGE_TYPE, true);

	connect(m_ui->searchRcvdMsgCheckBox, SIGNAL(clicked()),
	    this, SLOT(checkInputFields()));
	connect(m_ui->searchSntMsgCheckBox, SIGNAL(clicked()),
	    this, SLOT(checkInputFields()));

	connect(m_ui->msgIdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->subjectLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(m_ui->sndrBoxIdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->sndrNameLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->sndrRefNumLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->sndrFileMarkLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->rcpntBoxIdLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->rcpntNameLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->rcpntRefNumLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->rcpntFileMarkLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(m_ui->addressLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->toHandsLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(m_ui->tagLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(m_ui->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchMessages()));
	connect(m_ui->resultsTableWidget, SIGNAL(itemSelectionChanged()),
	    this, SLOT(setFirtsColumnActive()));
	connect(m_ui->resultsTableWidget, SIGNAL(cellDoubleClicked(int, int)),
	    this, SLOT(getSelectedMsg(int, int)));

	m_ui->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	m_ui->resultsTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
}


/* ========================================================================= */
/*
 * Check dialogue elements and set search button enable/disable
 */
void DlgMsgSearch::checkInputFields(void)
/* ========================================================================= */
{
	bool isAnyMsgTypeChecked = true;

	m_ui->searchPushButton->setEnabled(false);
	m_ui->tooManyFields->hide();

	/* is any message type checked? */
	if (!m_ui->searchRcvdMsgCheckBox->isChecked() &&
	    !m_ui->searchSntMsgCheckBox->isChecked()) {
		isAnyMsgTypeChecked = false;
	}

	/* search via message ID */
	if (!m_ui->msgIdLine->text().isEmpty()) {
		m_ui->subjectLine->setEnabled(false);
		m_ui->sndrBoxIdLine->setEnabled(false);
		m_ui->sndrNameLine->setEnabled(false);
		m_ui->sndrRefNumLine->setEnabled(false);
		m_ui->sndrFileMarkLine->setEnabled(false);
		m_ui->rcpntBoxIdLine->setEnabled(false);
		m_ui->rcpntNameLine->setEnabled(false);
		m_ui->rcpntRefNumLine->setEnabled(false);
		m_ui->rcpntFileMarkLine->setEnabled(false);
		m_ui->addressLine->setEnabled(false);
		m_ui->toHandsLine->setEnabled(false);
		m_ui->tagLine->setEnabled(false);
		m_ui->tagLine->clear();
		goto finish;
	} else {
		m_ui->subjectLine->setEnabled(true);
		m_ui->sndrBoxIdLine->setEnabled(true);
		m_ui->sndrNameLine->setEnabled(true);
		m_ui->sndrRefNumLine->setEnabled(true);
		m_ui->sndrFileMarkLine->setEnabled(true);
		m_ui->rcpntBoxIdLine->setEnabled(true);
		m_ui->rcpntNameLine->setEnabled(true);
		m_ui->rcpntRefNumLine->setEnabled(true);
		m_ui->rcpntFileMarkLine->setEnabled(true);
		m_ui->addressLine->setEnabled(true);
		m_ui->toHandsLine->setEnabled(true);
		m_ui->tagLine->setEnabled(true);
	}

	/* Use the sender data box ID for searching. */
	if (!m_ui->sndrBoxIdLine->text().isEmpty()) {
		m_ui->sndrBoxIdLine->setEnabled(true);
		m_ui->sndrNameLine->setEnabled(false);
	} else if (!m_ui->sndrNameLine->text().isEmpty()){
		m_ui->sndrBoxIdLine->setEnabled(false);
		m_ui->sndrNameLine->setEnabled(true);
	} else {
		m_ui->sndrBoxIdLine->setEnabled(true);
		m_ui->sndrNameLine->setEnabled(true);
	}

	/* Use the recipient data box ID for searching. */
	if (!m_ui->rcpntBoxIdLine->text().isEmpty()) {
		m_ui->rcpntBoxIdLine->setEnabled(true);
		m_ui->rcpntNameLine->setEnabled(false);
	} else if (!m_ui->rcpntNameLine->text().isEmpty()){
		m_ui->rcpntBoxIdLine->setEnabled(false);
		m_ui->rcpntNameLine->setEnabled(true);
	} else {
		m_ui->rcpntBoxIdLine->setEnabled(true);
		m_ui->rcpntNameLine->setEnabled(true);
	}

finish:
	/* search by message ID only */
	if (!m_ui->msgIdLine->text().isEmpty()) {
		/* test if message ID is number */
		QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
		/* test if message is fill and message ID > 3 chars */
		if (isAnyMsgTypeChecked &&
		    re.exactMatch(m_ui->msgIdLine->text()) &&
		    m_ui->msgIdLine->text().size() > 2) {
			m_ui->searchPushButton->setEnabled(true);
		} else {
			m_ui->searchPushButton->setEnabled(false);

		}
		return;
	}

	/* Search according to supplied tag text. */
	bool isTagCorrect = true;
	if (!m_ui->tagLine->text().isEmpty() &&
	    (m_ui->tagLine->text().length() <= 2)) {
		isTagCorrect = false;
	}

	bool isDbIdCorrect = true;
	/* Data-box ID must have 7 characters. */
	if (!m_ui->sndrBoxIdLine->text().isEmpty() &&
	    (m_ui->sndrBoxIdLine->text().size() != 7)) {
		isDbIdCorrect = false;
	}
	if (!m_ui->rcpntBoxIdLine->text().isEmpty() &&
	    (m_ui->rcpntBoxIdLine->text().size() != 7)) {
		isDbIdCorrect = false;
	}

	/* only 3 fields can be set together */
	bool isNotFillManyFileds = true;

	int itemsWithoutTag = howManyFieldsAreFilledWithoutTag();
	if (itemsWithoutTag > 3) {
		isNotFillManyFileds = false;
		m_ui->tooManyFields->show();
	} else if ((itemsWithoutTag < 1) && m_ui->tagLine->text().isEmpty()) {
		isNotFillManyFileds = false;
	}

	m_ui->searchPushButton->setEnabled(isAnyMsgTypeChecked &&
	    isDbIdCorrect && isTagCorrect && isNotFillManyFileds);
}


/* ========================================================================= */
/*
 * Detect, how many search fileds are filled
 */
int DlgMsgSearch::howManyFieldsAreFilledWithoutTag(void)
/* ========================================================================= */
{
	int cnt = 0;

	if (!m_ui->msgIdLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->subjectLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->sndrBoxIdLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->sndrNameLine->text().isEmpty())  {
		cnt++;
	}
	if (!m_ui->sndrRefNumLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->sndrFileMarkLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->rcpntBoxIdLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->rcpntNameLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->rcpntRefNumLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->rcpntFileMarkLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->addressLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->toHandsLine->text().isEmpty()) {
		cnt++;
	}
	if (!m_ui->tagLine->text().isEmpty()) {
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
	m_ui->resultsTableWidget->selectColumn(0);
	m_ui->resultsTableWidget->selectRow(
	    m_ui->resultsTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Search messages
 */
void DlgMsgSearch::searchMessages(void)
/* ========================================================================= */
{
	debugSlotCall();

	m_ui->resultsTableWidget->setRowCount(0);
	m_ui->resultsTableWidget->setEnabled(false);

	MessageDb::SoughtMsg msgData;

	/* holds search result data from message envelope table */
	QList<MessageDb::SoughtMsg> msgEnvlpResultList;

	/*
	 * holds all message ids of from message_tags table
	 * where input search text like with tag name
	 */
	QList<qint64> tagMsgIdList;

	/* holds messages (data) which will add to result widget */
	QList<MessageDb::SoughtMsg> msgListForTableView;

	/* message types where search process will be applied */
	enum MessageDirection msgType = MSG_ALL;
	if (m_ui->searchRcvdMsgCheckBox->isChecked() &&
	    m_ui->searchSntMsgCheckBox->isChecked()) {
		msgType = MSG_ALL;
	} else if (m_ui->searchRcvdMsgCheckBox->isChecked()) {
		msgType = MSG_RECEIVED;
	} else if (m_ui->searchSntMsgCheckBox->isChecked()) {
		msgType = MSG_SENT;
	}

	/*
	 * if tag input was filled, get message ids from message_tags table
	 * where input search text like with tag name
	*/
	bool applyTag = false;
	if (!m_ui->tagLine->text().isEmpty()) {
		tagMsgIdList = globTagDbPtr->getMsgIdsContainSearchTagText(
		    m_ui->tagLine->text());
		applyTag = true;
	}

	/* selected account or all accounts will be used for search request */
	int dbCount = 1;
	if (m_ui->searchAllAcntCheckBox->isChecked()) {
		dbCount = m_messageDbSetList.count();
	}

	/* how many fields without tag item are filled in the search dialog */
	int itemsWithoutTag = howManyFieldsAreFilledWithoutTag();

	/* over selected account or all accounts do */
	for (int i = 0; i < dbCount; ++i) {

		msgEnvlpResultList.clear();
		msgListForTableView.clear();

		/* when at least one field is filled (without tag) */
		if (itemsWithoutTag > 0) {
			/*
			 * get messages envelope data
			 * where search items are applied
			 */
			msgEnvlpResultList = m_messageDbSetList.at(i).second->
			    msgsAdvancedSearchMessageEnvelope(
			    m_ui->msgIdLine->text().isEmpty() ? -1 :
			        m_ui->msgIdLine->text().toLongLong(),
			    m_ui->subjectLine->text(),
			    m_ui->sndrBoxIdLine->text(),
			    m_ui->sndrNameLine->text(),
			    m_ui->addressLine->text(),
			    m_ui->rcpntBoxIdLine->text(),
			    m_ui->rcpntNameLine->text(),
			    m_ui->sndrRefNumLine->text(),
			    m_ui->sndrFileMarkLine->text(),
			    m_ui->rcpntRefNumLine->text(),
			    m_ui->rcpntFileMarkLine->text(),
			    m_ui->toHandsLine->text(),
			    QString(), QString(), msgType);
		}

		/* Results processing section - 4 scenarios */

		/*
		 * First scenario:
		 * tag input was filled and another envelope fileds were filled,
		 * tag list and msg envelope search list results are not empty,
		 * so we must penetration both list (prunik) and
		 * choose relevant records and show it (fill msgListForView).
		 */
		if (applyTag && (!tagMsgIdList.isEmpty()) &&
		    (!msgEnvlpResultList.isEmpty())) {

			foreach (const qint64 msgId, tagMsgIdList) {
				foreach (const MessageDb::SoughtMsg msg,
				    msgEnvlpResultList) {
					if (msg.mId.dmId == msgId) {
						msgData =
						    m_messageDbSetList.at(i).
						    second->msgsGetMsgDataFromId(msgId);
						if (msgData.mId.dmId != -1) {
							msgListForTableView.append(msgData);
						}
					}
				}
			}
			if (!msgListForTableView.isEmpty()) {
				appendMsgsToTable(m_messageDbSetList.at(i),
				    msgListForTableView);
			}

		/*
		 * Second scenario:
		 * tag input was filled and another envelope fileds were filled
		 * but msg envelope search result list is empty = no match,
		 * we show (do) nothing
		 */
		} else if (applyTag && msgEnvlpResultList.isEmpty() &&
		    (itemsWithoutTag > 0)) {

		/*
		 * Third scenario:
		 * tag input was not filled and msg envelope list is not empty,
		 * we show result for msg envelope list only
		  */
		} else if (!applyTag && !msgEnvlpResultList.isEmpty()) {
			appendMsgsToTable(m_messageDbSetList.at(i),
			    msgEnvlpResultList);

		/*
		 * Last scenario:
		 * only tag input was filled and tag list are not empty,
		 * we show result for tag results only (fill msgListForView).
		 */
		} else if (applyTag && (!tagMsgIdList.isEmpty())) {

			foreach (const qint64 msgId, tagMsgIdList) {

				msgData = m_messageDbSetList.at(i).second->
				    msgsGetMsgDataFromId(msgId);

				if (msgData.mId.dmId != -1) {
					msgListForTableView.append(msgData);
				}
			}

			if (!msgListForTableView.isEmpty()) {
				appendMsgsToTable(m_messageDbSetList.at(i),
				    msgListForTableView);
			}
		}
	}
}


/* ========================================================================= */
/*
 * Append message list to result tablewidget
 */
void DlgMsgSearch::appendMsgsToTable(
    const QPair<QString, MessageDbSet *> &usrNmAndMsgDbSet,
    const QList<MessageDb::SoughtMsg> &msgDataList)
/* ========================================================================= */
{
	m_ui->resultsTableWidget->setEnabled(true);

	foreach (const MessageDb::SoughtMsg &msgData, msgDataList) {
		int row = m_ui->resultsTableWidget->rowCount();
		m_ui->resultsTableWidget->insertRow(row);

		m_ui->resultsTableWidget->setItem(row, COL_USER_NAME,
		    new QTableWidgetItem(usrNmAndMsgDbSet.first));
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(QString::number(msgData.mId.dmId));
		if (ENABLE_TOOLTIP) {
			const MessageDb *messageDb =
			    usrNmAndMsgDbSet.second->constAccessMessageDb(
			        msgData.mId.deliveryTime);
			Q_ASSERT(0 != messageDb);

			item->setToolTip(messageDb->descriptionHtml(
			    msgData.mId.dmId, true, false, true));
		}
		m_ui->resultsTableWidget->setItem(row, COL_MESSAGE_ID, item);
		m_ui->resultsTableWidget->setItem(row, COL_DELIVERY_YEAR,
		    new QTableWidgetItem(
		        MessageDbSet::yearFromDateTime(
		            msgData.mId.deliveryTime)));
		m_ui->resultsTableWidget->setItem(row, COL_MESSAGE_TYPE,
		    new QTableWidgetItem(QString::number(msgData.type)));
		m_ui->resultsTableWidget->setItem(row, COL_ANNOTATION,
		    new QTableWidgetItem(msgData.dmAnnotation));
		m_ui->resultsTableWidget->setItem(row, COL_SENDER,
		    new QTableWidgetItem(msgData.dmSender));
		m_ui->resultsTableWidget->setItem(row, COL_RECIPIENT,
		    new QTableWidgetItem(msgData.dmRecipient));
	}

	m_ui->resultsTableWidget->resizeColumnsToContents();
	m_ui->resultsTableWidget->
	    horizontalHeader()->setStretchLastSection(true);
}


/* ========================================================================= */
/*
 * Get ID of selected message and set focus in MessageList Tableview
 */
void DlgMsgSearch::getSelectedMsg(int row, int column)
/* ========================================================================= */
{
	Q_UNUSED(column);
	emit focusSelectedMsg(
	    m_ui->resultsTableWidget->item(row, COL_USER_NAME)->text(),
	    m_ui->resultsTableWidget->item(row, COL_MESSAGE_ID)->text().toLongLong(),
	    m_ui->resultsTableWidget->item(row, COL_DELIVERY_YEAR)->text(),
	    m_ui->resultsTableWidget->item(row, COL_MESSAGE_TYPE)->text().toInt());
	//this->close();
}
