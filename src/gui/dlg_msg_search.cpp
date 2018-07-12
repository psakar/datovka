/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include "src/global.h"
#include "src/gui/dlg_msg_search.h"
#include "src/io/message_db.h"
#include "src/io/tag_db.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_tab_ignore_filter.h"
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

#define BOX_ID_LEN 7

DlgMsgSearch::DlgMsgSearch(
    const QList< QPair <QString, MessageDbSet *> > msgSetEntryList,
    const QString &userName, QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags),
    m_ui(new (std::nothrow) Ui::DlgMsgSearch),
    m_msgSetEntryList(msgSetEntryList)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	/* Set default line height for table views/widgets. */
	m_ui->resultsTableWidget->setNarrowedLineHeight();

	initSearchWindow(userName);
}

DlgMsgSearch::~DlgMsgSearch(void)
{
	delete m_ui;
}

void DlgMsgSearch::checkInputFields(void)
{
	bool isAnyMsgTypeChecked = true;

	m_ui->searchPushButton->setEnabled(false);
	m_ui->tooManyFields->hide();

	/* is any message type checked? */
	if (!m_ui->searchRcvdMsgCheckBox->isChecked() &&
	    !m_ui->searchSntMsgCheckBox->isChecked()) {
		isAnyMsgTypeChecked = false;
	}

	const bool msgIdMissing = m_ui->msgIdLine->text().isEmpty();

	m_ui->subjectLine->setEnabled(msgIdMissing);
	m_ui->sndrBoxIdLine->setEnabled(msgIdMissing);
	m_ui->sndrNameLine->setEnabled(msgIdMissing);
	m_ui->sndrRefNumLine->setEnabled(msgIdMissing);
	m_ui->sndrFileMarkLine->setEnabled(msgIdMissing);
	m_ui->rcpntBoxIdLine->setEnabled(msgIdMissing);
	m_ui->rcpntNameLine->setEnabled(msgIdMissing);
	m_ui->rcpntRefNumLine->setEnabled(msgIdMissing);
	m_ui->rcpntFileMarkLine->setEnabled(msgIdMissing);
	m_ui->addressLine->setEnabled(msgIdMissing);
	m_ui->toHandsLine->setEnabled(msgIdMissing);
	m_ui->tagLine->setEnabled(msgIdMissing);
	m_ui->fileNameLine->setEnabled(msgIdMissing);

	if (!msgIdMissing) {
		/* Search via message ID. */
		m_ui->tagLine->clear();
	} else {
		/* Use either sender box ID or sender name for searching. */
		m_ui->sndrBoxIdLine->setEnabled(m_ui->sndrNameLine->text().isEmpty());
		m_ui->sndrNameLine->setEnabled(m_ui->sndrBoxIdLine->text().isEmpty());

		/* Use either recipient box ID or recipient name for searching. */
		m_ui->rcpntBoxIdLine->setEnabled(m_ui->rcpntNameLine->text().isEmpty());
		m_ui->rcpntNameLine->setEnabled(m_ui->rcpntBoxIdLine->text().isEmpty());
	}

	/* Search using message ID only. */
	if (!msgIdMissing) {
		/* Test if message ID to be a number. */
		QRegExp re("\\d*"); /* A digit (\d), zero or more times (*). */
		/* Test whether message ID is at least 3 characters. */
		m_ui->searchPushButton->setEnabled(isAnyMsgTypeChecked &&
		    re.exactMatch(m_ui->msgIdLine->text()) &&
		    (m_ui->msgIdLine->text().size() > 2));
		return;
	}

	/* Search according to supplied tag text. */
	const bool tagCorrect = m_ui->tagLine->text().isEmpty() ||
	    (m_ui->tagLine->text().length() > 2);

	/* Data-box ID must have 7 characters. */
	const bool boxIdCorrect =
	    (m_ui->sndrBoxIdLine->text().isEmpty() ||
	     (m_ui->sndrBoxIdLine->text().size() == BOX_ID_LEN)) &&
	    (m_ui->rcpntBoxIdLine->text().isEmpty() ||
	     (m_ui->rcpntBoxIdLine->text().size() == BOX_ID_LEN));

	/* only 3 fields can be set together */
	bool isNotFillManyFileds = true;

	const int itemsWithoutTag = filledInExceptTags();
	if (itemsWithoutTag > 3) {
		isNotFillManyFileds = false;
		m_ui->tooManyFields->show();
	} else if ((itemsWithoutTag < 1) && m_ui->tagLine->text().isEmpty()) {
		isNotFillManyFileds = false;
	}

	m_ui->searchPushButton->setEnabled(isAnyMsgTypeChecked &&
	    boxIdCorrect && tagCorrect && isNotFillManyFileds);
}

void DlgMsgSearch::setFirtsColumnActive(void)
{
	m_ui->resultsTableWidget->selectColumn(0);
	m_ui->resultsTableWidget->selectRow(
	    m_ui->resultsTableWidget->currentRow());
}

void DlgMsgSearch::getSelectedMsg(int row, int col)
{
	Q_UNUSED(col);
	emit focusSelectedMsg(
	    m_ui->resultsTableWidget->item(row, COL_USER_NAME)->text(),
	    m_ui->resultsTableWidget->item(row, COL_MESSAGE_ID)->text().toLongLong(),
	    m_ui->resultsTableWidget->item(row, COL_DELIVERY_YEAR)->text(),
	    m_ui->resultsTableWidget->item(row, COL_MESSAGE_TYPE)->text().toInt());
	/* Don't close the dialogue. */
}

/*!
 * @brief Computes intersection between tag and envelope search result data.
 *
 * @param[in] envelData Envelope search result data.
 * @param[in] tagData Tag search data.
 * @param[in] msgSetEntry Username and database set pair.
 * @return Intersection data.
 */
static
QList<MessageDb::SoughtMsg> dataIntersect(
    const QList<MessageDb::SoughtMsg> &envelData, const QList<qint64> &tagData,
    const QPair<QString, MessageDbSet *> &msgSetEntry)
{
	QList<MessageDb::SoughtMsg> result;

	foreach (const qint64 msgId, tagData) {
		foreach (const MessageDb::SoughtMsg msg, envelData) {
			if (msg.mId.dmId == msgId) {
				MessageDb::SoughtMsg msgData(
				    msgSetEntry.second->msgsGetMsgDataFromId(msgId));
				if (msgData.mId.dmId != -1) {
					result.append(msgData);
				}
			}
		}
	}

	return result;
}

/*!
 * @brief Obtains data from databases that match found tag data.
 *
 * @param[in] tagData Tag search data.
 * @param[in] msgSetEntry Username and database set pair.
 * @return Message data.
 */
static
QList<MessageDb::SoughtMsg> displayableTagData(const QList<qint64> &tagData,
    const QPair<QString, MessageDbSet *> &msgSetEntry)
{
	QList<MessageDb::SoughtMsg> result;

	foreach (const qint64 msgId, tagData) {
		MessageDb::SoughtMsg msgData(
		    msgSetEntry.second->msgsGetMsgDataFromId(msgId));

		if (msgData.mId.dmId != -1) {
			result.append(msgData);
		}
	}

	return result;
}

void DlgMsgSearch::searchMessages(void)
{
	debugSlotCall();

	m_ui->resultsTableWidget->setRowCount(0);
	m_ui->resultsTableWidget->setEnabled(false);

	/* Message envelope search results. */
	QList<MessageDb::SoughtMsg> envelResults;

	/* Tag search result. */
	QList<qint64> tagResults;

	/* Displayed results. */
	QList<MessageDb::SoughtMsg> resultsToBeDisplayed;

	/* Types of messages to search for. */
	enum MessageDirection msgType = MSG_ALL;
	if (m_ui->searchRcvdMsgCheckBox->isChecked() &&
	    m_ui->searchSntMsgCheckBox->isChecked()) {
		msgType = MSG_ALL;
	} else if (m_ui->searchRcvdMsgCheckBox->isChecked()) {
		msgType = MSG_RECEIVED;
	} else if (m_ui->searchSntMsgCheckBox->isChecked()) {
		msgType = MSG_SENT;
	}

	/* If tag data were supplied, get message ids from tag table. */
	const bool searchTags = !m_ui->tagLine->text().isEmpty();
	if (searchTags) {
		tagResults =
		    GlobInstcs::tagDbPtr->getMsgIdsContainSearchTagText(
		        m_ui->tagLine->text());
	}

	/* Number of accounts in which to search for messages in. */
	const int dbCount = m_ui->searchAllAcntCheckBox->isChecked() ?
	    m_msgSetEntryList.count() : 1;

	/* How many envelope fields (without tags) are supplied. */
	const int envelopeItems = filledInExceptTags();

	/* Fill search envelope items. */
	Isds::Envelope searchEnvelope;
	searchEnvelope.setDmId(m_ui->msgIdLine->text().isEmpty() ? -1 :
	    m_ui->msgIdLine->text().toLongLong());
	searchEnvelope.setDmAnnotation(m_ui->subjectLine->text());
	searchEnvelope.setDbIDSender(m_ui->sndrBoxIdLine->text());
	searchEnvelope.setDmSender(m_ui->sndrNameLine->text());
	searchEnvelope.setDmSenderAddress(m_ui->addressLine->text());
	searchEnvelope.setDbIDRecipient(m_ui->rcpntBoxIdLine->text());
	searchEnvelope.setDmRecipient(m_ui->rcpntNameLine->text());
	searchEnvelope.setDmSenderRefNumber(m_ui->sndrRefNumLine->text());
	searchEnvelope.setDmSenderIdent(m_ui->sndrFileMarkLine->text());
	searchEnvelope.setDmRecipientRefNumber(m_ui->rcpntRefNumLine->text());
	searchEnvelope.setDmRecipientIdent(m_ui->rcpntFileMarkLine->text());
	searchEnvelope.setDmToHands(m_ui->toHandsLine->text());

	/* Search in accounts. */
	for (int i = 0; i < dbCount; ++i) {
		const QPair<QString, MessageDbSet *> &msgSetEntry(
		    m_msgSetEntryList.at(i));

		envelResults.clear();
		resultsToBeDisplayed.clear();

		if (envelopeItems > 0) {
			/* Search in envelope envelope data. */
			envelResults =
			    msgSetEntry.second->msgsAdvancedSearchMessageEnvelope(
			    searchEnvelope, msgType, m_ui->fileNameLine->text());
		}

		if (searchTags && !tagResults.isEmpty() && !envelResults.isEmpty()) {
			/*
			 * Tag data were supplied and some envelope data also.
			 * Intersection of tag and envelope search results is
			 * needed.
			 */
			resultsToBeDisplayed = dataIntersect(envelResults,
			    tagResults, msgSetEntry);
		} else if (!searchTags && !envelResults.isEmpty()) {
			/*
			 * No tag data were supplied. Envelope were supplied.
			 * Use envelope search results only.
			 */
			resultsToBeDisplayed = envelResults;
		} else if (searchTags && !tagResults.isEmpty()) {
			/*
			 * Only tag tag data were supplied.
			 * Convert tag results into displayable form.
			 */
			resultsToBeDisplayed = displayableTagData(tagResults,
			    msgSetEntry);
		}

		if (!resultsToBeDisplayed.isEmpty()) {
			appendMsgsToTable(msgSetEntry, resultsToBeDisplayed);
		}
	}
}

void DlgMsgSearch::initSearchWindow(const QString &username)
{
	m_ui->infoTextLabel->setText(tr(
	    "Here it is possible to search for messages according to supplied criteria. "
	    "You can search for messages in the selected account or in all accounts. "
	    "Double clicking on a found message will change focus of the selected message in the main application window. "
	    "Note: You can view additional information when hovering the mouse cursor over the message ID."));

	Q_ASSERT(!username.isEmpty());

	/* Set account name and user name to label. */
	m_ui->crntAcntNameLabel->setText(
	    (*GlobInstcs::acntMapPtr)[username].accountName() +
	    " (" + username + ")");

	/* Only one account available. */
	if (m_msgSetEntryList.count() <= 1) {
		m_ui->searchAllAcntCheckBox->setEnabled(false);
	}

	m_ui->tooManyFields->setStyleSheet("QLabel { color: red }");
	m_ui->tooManyFields->hide();

	{
		QIcon ico;
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		m_ui->searchPushButton->setIcon(ico);
	}

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

	connect(m_ui->fileNameLine, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(m_ui->resultsTableWidget, SIGNAL(itemSelectionChanged()),
	    this, SLOT(setFirtsColumnActive()));
	connect(m_ui->resultsTableWidget, SIGNAL(cellDoubleClicked(int, int)),
	    this, SLOT(getSelectedMsg(int, int)));

	connect(m_ui->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchMessages()));

	m_ui->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	m_ui->resultsTableWidget->installEventFilter(
	    new TableHomeEndFilter(m_ui->resultsTableWidget));
	m_ui->resultsTableWidget->installEventFilter(
	    new TableTabIgnoreFilter(m_ui->resultsTableWidget));
}

int DlgMsgSearch::filledInExceptTags(void) const
{
	int cnt = 0;

	if (!m_ui->msgIdLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->subjectLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->sndrBoxIdLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->sndrNameLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->sndrRefNumLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->sndrFileMarkLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->rcpntBoxIdLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->rcpntNameLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->rcpntRefNumLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->rcpntFileMarkLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->addressLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->toHandsLine->text().isEmpty()) { ++cnt; }
	if (!m_ui->fileNameLine->text().isEmpty()) { ++cnt; }

	return cnt;
}

/*!
 * @brief Append message entry into message table widget.
 *
 * @param[in,out] tabWid Table widget to put message entries into.
 * @param[in]     msgSetEntry Username and pointer to database set.
 * @param[in]     msgData Message entry.
 */
static
void appendMsgToWidget(QTableWidget *tabWid,
    const QPair<QString, MessageDbSet *> &msgSetEntry,
    const MessageDb::SoughtMsg &msgData)
{
	if (Q_UNLIKELY(Q_NULLPTR == tabWid)) {
		Q_ASSERT(0);
		return;
	}

	const int row = tabWid->rowCount();
	tabWid->insertRow(row);

	tabWid->setItem(row, COL_USER_NAME,
	    new QTableWidgetItem(msgSetEntry.first));
	QTableWidgetItem *item = new QTableWidgetItem;
	item->setText(QString::number(msgData.mId.dmId));
	if (ENABLE_TOOLTIP) {
		const MessageDb *messageDb =
		    msgSetEntry.second->constAccessMessageDb(
		        msgData.mId.deliveryTime);
		if (Q_NULLPTR != messageDb) {
			item->setToolTip(messageDb->descriptionHtml(
			    msgData.mId.dmId, false));
		} else {
			Q_ASSERT(0);
		}
	}
	tabWid->setItem(row, COL_MESSAGE_ID, item);
	tabWid->setItem(row, COL_DELIVERY_YEAR,
	    new QTableWidgetItem(
	        MessageDbSet::yearFromDateTime(
	            msgData.mId.deliveryTime)));
	tabWid->setItem(row, COL_MESSAGE_TYPE,
	    new QTableWidgetItem(QString::number(msgData.type)));
	tabWid->setItem(row, COL_ANNOTATION,
	    new QTableWidgetItem(msgData.dmAnnotation));
	tabWid->setItem(row, COL_SENDER,
	    new QTableWidgetItem(msgData.dmSender));
	tabWid->setItem(row, COL_RECIPIENT,
	    new QTableWidgetItem(msgData.dmRecipient));
}

void DlgMsgSearch::appendMsgsToTable(
    const QPair<QString, MessageDbSet *> &msgSetEntry,
    const QList<MessageDb::SoughtMsg> &msgDataList)
{
	m_ui->resultsTableWidget->setEnabled(true);

	foreach (const MessageDb::SoughtMsg &msgData, msgDataList) {
		appendMsgToWidget(m_ui->resultsTableWidget, msgSetEntry,
		    msgData);
	}

	m_ui->resultsTableWidget->resizeColumnsToContents();
	m_ui->resultsTableWidget->
	    horizontalHeader()->setStretchLastSection(true);
}
