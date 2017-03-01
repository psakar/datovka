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

#include <algorithm> /* std::sort */
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMimeDatabase>

#include "dlg_send_message.h"
#include "src/gui/datovka.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/model_interaction/attachment_interaction.h"
#include "src/gui/dlg_search_mojeid.h"
#include "src/models/accounts_model.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/wd_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/views/attachment_table_view.h"
#include "src/views/table_home_end_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_download_credit_info.h"
#include "src/worker/task_keep_alive.h"
#include "src/worker/task_send_message.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_search_owner_fulltext.h"
#include "src/worker/task_send_message_mojeid.h"
#include "src/worker/task_search_owner.h"
#include "ui_dlg_send_message.h"

const QString &dzPrefix(MessageDb *messageDb, qint64 dmId)
{
	const static QString nothing;
	const static QString received(QLatin1String("D"));
	const static QString sent(QLatin1String("O"));

	if (Q_NULLPTR == messageDb || dmId < 0) {
		return nothing;
	}

	switch (messageDb->msgMessageType(dmId)) {
	case MessageDb::TYPE_RECEIVED:
		return received;
		break;
	case MessageDb::TYPE_SENT:
		return sent;
		break;
	default:
		return nothing;
		break;
	}
}

DlgSendMessage::DlgSendMessage(
    const QList<Task::AccountDescr> &messageDbSetList,
    Action action, const QList<MessageDb::MsgId> &msgIds,
    const QString &userName, class MainWindow *mv, QWidget *parent)
    : QDialog(parent),
    m_keepAliveTimer(),
    m_messageDbSetList(messageDbSetList),
    m_msgIds(msgIds),
    m_dbId(),
    m_senderName(),
    m_action(action),
    m_userName(userName),
    m_dbType(),
    m_dbEffectiveOVM(false),
    m_dbOpenAddressing(false),
    m_lastAttAddPath(""),
    m_pdzCredit("0"),
    m_dmType(),
    m_dmSenderRefNumber(),
    m_mv(mv),
    m_dbSet(Q_NULLPTR),
    m_isLogged(false),
    m_attachmentModel(),
    m_isWebDatovkaAccount(false),
    m_recipientTableModel(this),
    m_transactIds(),
    m_sentMsgResultList()
{
	setupUi(this);

	/* Set default line height for table views/widgets. */
	recipientTableView->setNarrowedLineHeight();
	recipientTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	attachmentTableView->setNarrowedLineHeight();
	attachmentTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	initContent();

	Q_ASSERT(Q_NULLPTR != m_dbSet);
}

void DlgSendMessage::checkInputFields(void)
{
	bool buttonEnabled = calculateAndShowTotalAttachSize() &&
	    !this->subjectText->text().isEmpty() &&
	    (m_recipientTableModel.rowCount() > 0) &&
	    (m_attachmentModel.rowCount() > 0);

	if (this->payReplyCheckBox->isChecked()) {
		if (this->dmSenderRefNumber->text().isEmpty()) {
			buttonEnabled = false;
		}
	}

	if (m_isLogged) {
		this->sendButton->setEnabled(buttonEnabled);
	} else {
		this->sendButton->setEnabled(false);
	}
}

void DlgSendMessage::addRecipientFromLocalContact(void)
{
	QStringList dbIDs;
	QDialog *dlgCont = new DlgContacts(*m_dbSet, m_dbId, &dbIDs, this);
	dlgCont->exec();
	dlgCont->deleteLater();
	insertDataboxesToRecipientList(dbIDs);
}

void DlgSendMessage::addRecipientFromISDSSearch(void)
{
	QStringList dbIDs;
	QDialog *dsSearch = Q_NULLPTR;
	if (!m_isWebDatovkaAccount) {
		dsSearch = new DlgDsSearch(m_userName, m_dbType,
		    m_dbEffectiveOVM, m_dbOpenAddressing, &dbIDs, this);
	} else {
#if 0 /* TODO -- MojeID functionality disabled, deal with it later. */
		dsSearch = new DlgDsSearchMojeId(DlgDsSearchMojeId::ACT_ADDNEW,
		    this->recipientTableWidget, m_dbType, m_dbEffectiveOVM,
		    this, m_userName);
#else
		return;
#endif
	}
	dsSearch->exec();
	dsSearch->deleteLater();
	insertDataboxesToRecipientList(dbIDs);
}

void DlgSendMessage::addRecipientManually(void)
{
	bool ok = false;

	QString dbID = QInputDialog::getText(this, tr("Databox ID"),
	    tr("Enter Databox ID (7 characters):"), QLineEdit::Normal,
	    NULL, &ok, Qt::WindowStaysOnTopHint);

	if (!ok) {
		return;
	}

	if (dbID.isEmpty() || dbID.length() != 7) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle(tr("Wrong data box ID"));
		msgBox.setText(tr("Wrong data box ID '%1'!").arg(dbID));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		return;
	}

	insertDataboxesToRecipientList(QStringList(dbID));
}

void DlgSendMessage::recipientSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	/*
	 * 'I' means contractual PDZ that initiates a reply (prepaid?) PDZ.
	 * It should not be possible to delete recipients for those messages.
	 */
	removeRecipient->setEnabled((m_dmType != QStringLiteral("I")) &&
	    recipientTableView->selectionModel()->selectedRows(0).size() > 0);
}

/*!
 * @brief Used for sorting lists of integers.
 */
class RowLess {
public:
	bool operator()(int a, int b) const
	{
		return a < b;
	}
};

/*!
 * @brief Remove selected lines from model.
 *
 * @param[in]     view Table view to use for determining the selected lines.
 * @param[in,out] model Model to delete the rows from.
 */
static
void removeSelectedEntries(const QTableView *view, QAbstractItemModel *model)
{
	if ((view == Q_NULLPTR) || (model == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	QList<int> rows;
	{
		foreach (const QModelIndex &idx,
		         view->selectionModel()->selectedRows(0)) {
			rows.append(idx.row());
		}

		::std::sort(rows.begin(), rows.end(), RowLess());
	}

	/* In reverse order. */
	for (int i = rows.size() - 1; i >= 0; --i) {
		model->removeRow(rows.at(i));
	}
}

void DlgSendMessage::deleteRecipientEntries(void)
{
	removeSelectedEntries(recipientTableView, &m_recipientTableModel);
}

void DlgSendMessage::showOptionalForm(void)
{
	this->optionalWidget->setHidden(
	    (this->optionalFieldCheckBox->checkState() == Qt::Unchecked) &&
	    (this->payReplyCheckBox->checkState() == Qt::Unchecked));

	checkInputFields();

	if (this->payReplyCheckBox->checkState() == Qt::Unchecked) {
		this->labeldmSenderRefNumber->setStyleSheet(
		    "QLabel { color: black }");
		this->labeldmSenderRefNumber->setText(
		    tr("Our reference number:"));
		disconnect(this->dmSenderRefNumber,SIGNAL(textChanged(QString)),
		    this, SLOT(checkInputFields()));
	} else {
		this->labeldmSenderRefNumber->setStyleSheet(
		    "QLabel { color: red }");
		this->labeldmSenderRefNumber->setText(
		    tr("Enter reference number:"));
		this->dmSenderRefNumber->setFocus();
		connect(this->dmSenderRefNumber, SIGNAL(textChanged(QString)),
		    this, SLOT(checkInputFields()));
	}
}

void DlgSendMessage::addAttachmentFile(void)
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastAttAddPath);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList fileNames;

	if (dialog.exec()) {
		fileNames = dialog.selectedFiles();
		if (!globPref.use_global_paths) {
			m_lastAttAddPath = dialog.directory().absolutePath();
			emit doActionAfterSentMsgSignal(m_userName,
			    m_lastAttAddPath);
		}
	}

	foreach (const QString &fileName, fileNames) {
		int fileSize = m_attachmentModel.insertAttachmentFile(fileName,
		    m_attachmentModel.rowCount());
		if (fileSize <= 0) {
			/* TODO -- Generate some warning message. */
			continue;
		}
	}
}

void DlgSendMessage::attachmentSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	int selectionSize =
	    attachmentTableView->selectionModel()->selectedRows(0).size();

	removeAttachment->setEnabled(selectionSize > 0);
	openAttachment->setEnabled(selectionSize == 1);
}

void DlgSendMessage::deleteSelectedAttachmentFiles(void)
{
	removeSelectedEntries(attachmentTableView, &m_attachmentModel);
}

void DlgSendMessage::openSelectedAttachment(const QModelIndex &index)
{
	debugSlotCall();

	AttachmentInteraction::openAttachment(this, *this->attachmentTableView,
	    index);
}

void DlgSendMessage::pingIsdsServer(void) const
{
	TaskKeepAlive *task = new (std::nothrow) TaskKeepAlive(m_userName);
	task->setAutoDelete(true);
	globWorkPool.assignHi(task);
}

void DlgSendMessage::initContent(void)
{
	if (isWebDatovkaAccount(m_userName)) {
		m_isWebDatovkaAccount = true;
	}

	m_recipientTableModel.setHeader();
	this->recipientTableView->setModel(&m_recipientTableModel);

	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	this->recipientTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	this->recipientTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	this->recipientTableView->setColumnHidden(BoxContactsModel::CHECKBOX_COL, true);
	this->recipientTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL, true);

	m_attachmentModel.setHeader();
	this->attachmentTableView->setModel(&m_attachmentModel);

	this->attachmentTableView->setColumnWidth(DbFlsTblModel::FNAME_COL, 150);
	this->attachmentTableView->setColumnWidth(DbFlsTblModel::MIME_COL, 120);

	this->attachmentTableView->setColumnHidden(DbFlsTblModel::ATTACHID_COL, true);
	this->attachmentTableView->setColumnHidden(DbFlsTblModel::MSGID_COL, true);
	this->attachmentTableView->setColumnHidden(DbFlsTblModel::CONTENT_COL, true);

	/* Enable drag and drop on attachment table. */
	this->attachmentTableView->setAcceptDrops(true);
	this->attachmentTableView->setDragEnabled(true);
	this->attachmentTableView->setDragDropOverwriteMode(false);
	this->attachmentTableView->setDropIndicatorShown(true);
	this->attachmentTableView->setDragDropMode(QAbstractItemView::DragDrop);
	this->attachmentTableView->setDefaultDropAction(Qt::CopyAction);

	this->replyLabel->hide();
	this->replyLabel->setEnabled(false);

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		const QString accountName =
		    AccountModel::globAccounts[acnt.userName].accountName() +
		    " (" + acnt.userName + ")";
		this->fromComboBox->addItem(accountName, QVariant(acnt.userName));
		if (m_userName == acnt.userName) {
			int i = this->fromComboBox->count() - 1;
			Q_ASSERT(0 <= i);
			this->fromComboBox->setCurrentIndex(i);
			setAccountInfo(i);
		}
	}

	connect(this->fromComboBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(setAccountInfo(int)));

	connect(&m_recipientTableModel,
	    SIGNAL(rowsInserted(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));
	connect(&m_recipientTableModel,
	    SIGNAL(rowsRemoved(QModelIndex, int, int)),
	    this, SLOT(checkInputFields()));

	this->optionalWidget->setHidden(true);

	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalForm()));
	connect(this->payReplyCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(showOptionalForm()));

	connect(this->addRecipient, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromLocalContact()));
	connect(this->removeRecipient, SIGNAL(clicked()),
	    this, SLOT(deleteRecipientEntries()));
	connect(this->findRecipient, SIGNAL(clicked()),
	    this, SLOT(addRecipientFromISDSSearch()));
	connect(this->enterDbIdpushButton, SIGNAL(clicked()),
	    this, SLOT(addRecipientManually()));

	connect(this->addAttachment, SIGNAL(clicked()), this,
	    SLOT(addAttachmentFile()));
	connect(this->removeAttachment, SIGNAL(clicked()), this,
	    SLOT(deleteSelectedAttachmentFiles()));
	connect(this->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openSelectedAttachment()));

	connect(this->recipientTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(recipientSelectionChanged(QItemSelection, QItemSelection)));

	connect(this->attachmentTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(openSelectedAttachment(QModelIndex)));

	connect(this->subjectText, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(&m_attachmentModel,
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(&m_attachmentModel,
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(&m_attachmentModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this,
	    SLOT(attachmentDataChanged(QModelIndex, QModelIndex, QVector<int>)));
	connect(this->attachmentTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentSelectionChanged(QItemSelection, QItemSelection)));

	this->recipientTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->attachmentTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->recipientTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	this->attachmentTableView->installEventFilter(
	    new TableHomeEndFilter(this));

	connect(this->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(this->cancelButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64)));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageMojeIdFinished(QString, QStringList, QString)), this,
	    SLOT(sendMessageMojeIdAction(QString, QStringList,  QString)));

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	if (!m_isWebDatovkaAccount) {
		connect(&m_keepAliveTimer, SIGNAL(timeout()), this,
		    SLOT(pingIsdsServer()));
	}

	this->attachmentSizeInfo->setText(
	    tr("Total size of attachments is %1 B").arg(0));

	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		this->dmAllowSubstDelivery->setEnabled(false);
		this->dmAllowSubstDelivery->hide();
	}

	if (ACT_REPLY == m_action) {
		fillDlgAsReply();
	} else {
		if (m_dbOpenAddressing) {
			this->payReplyCheckBox->setEnabled(true);
			this->payReplyCheckBox->show();
		} else {
			this->payReplyCheckBox->setEnabled(false);
			this->payReplyCheckBox->hide();
		}

		this->payRecipient->setEnabled(false);
		this->payRecipient->hide();
		if (ACT_NEW_FROM_TMP == m_action) {
			fillDlgFromTmpMsg();
		} else if (ACT_FORWARD == m_action) {
			fillDlgAsForward();
		}
	}

	this->adjustSize();
}


/* ========================================================================= */
/*
 * Slot: Set info data and database for selected account
 */
void DlgSendMessage::setAccountInfo(int item)
/* ========================================================================= */
{
	debugSlotCall();

	/* Get user name for selected account. */
	const QString userName = this->fromComboBox->itemData(item).toString();

	if (!userName.isEmpty()) {
		/* If account was changed, remove all recipients. */
		m_recipientTableModel.removeRows(0,
		    m_recipientTableModel.rowCount());
		m_userName = userName;
	}

	if (isWebDatovkaAccount(m_userName)) {
		m_isWebDatovkaAccount = true;
	}

	if (!m_isWebDatovkaAccount) {

		struct isds_ctx *session = NULL;
		m_isLogged = true;
		m_keepAliveTimer.stop();
		{
			TaskKeepAlive *task;
			task = new (std::nothrow) TaskKeepAlive(m_userName);
			task->setAutoDelete(false);
			globWorkPool.runSingle(task);

			m_isLogged = task->m_isAlive;

			delete task;
		}
		if (!m_isLogged) {
			if (m_mv) {
				m_isLogged = m_mv->connectToIsds(m_userName);
			}
		}
		m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

		session = globIsdsSessions.session(m_userName);
		if (NULL == session) {
			logErrorNL("%s", "Missing ISDS session.");
			m_isLogged = false;
		}
	} else {
		if (!wdSessions.isConnectedToWebdatovka(m_userName)) {
			m_mv->loginToMojeId(m_userName);
		}
		m_isLogged = true;
	}

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		if (acnt.userName == m_userName) {
			m_dbSet = acnt.messageDbSet;
			break;
		}
	}

	const AcntSettings &accountInfo(AccountModel::globAccounts[m_userName]);
	m_dbId = globAccountDbPtr->dbId(m_userName + "___True");
	Q_ASSERT(!m_dbId.isEmpty());
	m_senderName =
	    globAccountDbPtr->senderNameGuess(m_userName + "___True");
	QList<QString> accountData =
	    globAccountDbPtr->getUserDataboxInfo(m_userName + "___True");
	if (!accountData.isEmpty()) {
		m_dbType = accountData.at(0);
		m_dbEffectiveOVM = (accountData.at(1) == "1");
		m_dbOpenAddressing = (accountData.at(2) == "1");
	}
	if (globPref.use_global_paths) {
		m_lastAttAddPath = globPref.add_file_to_attachments_path;
	} else {
		m_lastAttAddPath = accountInfo.lastAttachAddPath();
	}

	if (!m_isWebDatovkaAccount) {
		if (m_dbOpenAddressing) {
			m_pdzCredit = getPDZCreditFromISDS(m_userName, m_dbId);
		}
	}

	QString dbOpenAddressingText = "";
	if (!m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			dbOpenAddressingText =
			    " - " + tr("sending of PDZ: enabled") + "; " +
			    tr("remaining credit: ") + m_pdzCredit + " Kč";
		} else {
			dbOpenAddressingText =
			    " - " + tr("sending of PDZ: disabled");
		}
	}

	this->fromUser->setText("<strong>" +
	    AccountModel::globAccounts[m_userName].accountName() +
	    "</strong>" + " (" + m_userName + ") - " + m_dbType +
	    dbOpenAddressingText);
}


/* ========================================================================= */
/*
 * Whenever any data in attachment table change.
 */
void DlgSendMessage::attachmentDataChanged(const QModelIndex &topLeft,
    const QModelIndex &bottomRight, const QVector<int> &roles)
/* ========================================================================= */
{
	Q_UNUSED(topLeft);
	Q_UNUSED(bottomRight);
	Q_UNUSED(roles);

	checkInputFields();
}


/* ========================================================================= */
/*
 * Func: Fill Send Message Dialog as reply.
 */
void DlgSendMessage::fillDlgAsReply(void)
/* ========================================================================= */
{
	debugFuncCall();

	if (m_msgIds.size() != 1) {
		logWarningNL("%s",
		    "Expected one message to generate reply from.");
		return;
	}
	const MessageDb::MsgId &msgId(m_msgIds.first());

	bool hideOptionalWidget = true;

	this->fromComboBox->setEnabled(false);

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	this->subjectText->setText("Re: " + envData.dmAnnotation);

	if (!envData.dmSenderRefNumber.isEmpty()) {
		this->dmRecipientRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		this->dmRecipientIdent->setText(envData.dmSenderIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		this->dmSenderRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		this->dmSenderIdent->setText(envData.dmRecipientIdent);
		hideOptionalWidget = false;
	}

	this->optionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);
	this->payRecipient->setEnabled(false);
	this->payRecipient->setChecked(false);
	this->payRecipient->hide();

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDSender);
		this->payReplyCheckBox->show();
		this->payReplyCheckBox->setEnabled(true);
	} else {
		this->payReplyCheckBox->setEnabled(false);
		this->payReplyCheckBox->hide();
		pdz = false;
	}

	if (m_dmType == "I") {
		this->addRecipient->setEnabled(false);
		this->removeRecipient->setEnabled(false);
		this->findRecipient->setEnabled(false);
		this->replyLabel->show();
		this->replyLabel->setEnabled(true);
		this->payReplyCheckBox->hide();
		this->payReplyCheckBox->setEnabled(false);
		this->payRecipient->setEnabled(true);
		this->payRecipient->setChecked(true);
		this->payRecipient->show();
		pdz = true;
	}

	m_recipientTableModel.appendData(envData.dbIDSender, -1,
	    envData.dmSender, envData.dmSenderAddress, QString(), pdz);
}

void DlgSendMessage::fillDlgAsForward(void)
{
	debugFuncCall();

	if (m_msgIds.size() == 0) {
		logWarningNL("%s",
		    "Expected at lease one message to generate reply from.");
		return;
	}

	/* Fill attachments with messages. */
	foreach (const MessageDb::MsgId &msgId, m_msgIds) {
		MessageDb *messageDb =
		    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
		if (Q_NULLPTR == messageDb) {
			Q_ASSERT(0);
			continue;
		}

		/* If only a single message if forwarded. */
		if (m_msgIds.size() == 1) {
			MessageDb::PartialEnvelopeData envData(
			    messageDb->msgsReplyData(msgId.dmId));

			this->subjectText->setText("Fwd: " + envData.dmAnnotation);
		}

		QByteArray msgBase64(messageDb->msgsMessageBase64(msgId.dmId));
		if (msgBase64.isEmpty()) {
			continue;
		}

		m_attachmentModel.appendAttachmentEntry(msgBase64,
		    dzPrefix(messageDb, msgId.dmId) + QString("DZ_%1.zfo").arg(msgId.dmId));
	}
}

/* ========================================================================= */
/*
 * Func: Fill Send Message Dialog from template message.
 */
void DlgSendMessage::fillDlgFromTmpMsg(void)
/* ========================================================================= */
{
	debugFuncCall();

	if (m_msgIds.size() != 1) {
		logWarningNL("%s",
		    "Expected one message to generate reply from.");
		return;
	}
	const MessageDb::MsgId &msgId(m_msgIds.first());

	bool hideOptionalWidget = true;

	MessageDb *messageDb =
	    m_dbSet->accessMessageDb(msgId.deliveryTime, false);
	Q_ASSERT(Q_NULLPTR != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(msgId.dmId);
	m_dmType = envData.dmType;
	m_dmSenderRefNumber = envData.dmRecipientRefNumber;

	this->subjectText->setText(envData.dmAnnotation);

	/* Fill in optional fields.  */
	if (!envData.dmSenderRefNumber.isEmpty()) {
		this->dmSenderRefNumber->setText(envData.dmSenderRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmSenderIdent.isEmpty()) {
		this->dmSenderIdent->setText(envData.dmSenderIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientRefNumber.isEmpty()) {
		this->dmRecipientRefNumber->setText(envData.dmRecipientRefNumber);
		hideOptionalWidget = false;
	}
	if (!envData.dmRecipientIdent.isEmpty()) {
		this->dmRecipientIdent->setText(envData.dmRecipientIdent);
		hideOptionalWidget = false;
	}
	if (!envData.dmToHands.isEmpty()) {
		this->dmToHands->setText(envData.dmToHands);
		hideOptionalWidget = false;
	}
	/* set check boxes */
	this->dmPersonalDelivery->setChecked(envData.dmPersonalDelivery);
	this->dmAllowSubstDelivery->setChecked(envData.dmAllowSubstDelivery);
	/* fill optional LegalTitle - Law, year, ... */
	if (!envData.dmLegalTitleLaw.isEmpty()) {
		this->dmLegalTitleLaw->setText(envData.dmLegalTitleLaw);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitleYear.isEmpty()) {
		this->dmLegalTitleYear->setText(envData.dmLegalTitleYear);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitleSect.isEmpty()) {
		this->dmLegalTitleSect->setText(envData.dmLegalTitleSect);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitlePar.isEmpty()) {
		this->dmLegalTitlePar->setText(envData.dmLegalTitlePar);
		hideOptionalWidget = false;
	}
	if (!envData.dmLegalTitlePoint.isEmpty()) {
		this->dmLegalTitlePoint->setText(envData.dmLegalTitlePoint);
		hideOptionalWidget = false;
	}

	this->optionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);

	bool pdz;
	if (!m_dbEffectiveOVM) {
		pdz = !queryISDSBoxEOVM(m_userName, envData.dbIDRecipient);
		this->payReplyCheckBox->show();
		this->payReplyCheckBox->setEnabled(true);
	} else {
		this->payReplyCheckBox->setEnabled(false);
		this->payReplyCheckBox->hide();
		pdz = false;
	}

	/* message is received -> recipient == sender */
	if (m_dbId != envData.dbIDRecipient) {
		m_recipientTableModel.appendData(envData.dbIDRecipient, -1,
		    envData.dmRecipient, envData.dmRecipientAddress, QString(),
		    pdz);
	}

	/* fill attachments from template message */
	QList<MessageDb::FileData> msgFileList =
	    messageDb->getFilesFromMessage(msgId.dmId);

	foreach (const MessageDb::FileData &fileData, msgFileList) {
		m_attachmentModel.appendAttachmentEntry(
		    fileData.dmEncodedContent, fileData.dmFileDescr);
	}
}


/* ========================================================================= */
/*
 * Calculate total attachment size when an item was added/removed in the table.
 */
bool DlgSendMessage::calculateAndShowTotalAttachSize(void)
/* ========================================================================= */
{
	int aSize = 0;

	for (int i = 0; i < m_attachmentModel.rowCount(); ++i) {
		QModelIndex idx(
		    m_attachmentModel.index(i, DbFlsTblModel::FSIZE_COL));

		if (idx.isValid()) {
			aSize += idx.data().toInt();
		}
	}

	this->attachmentSizeInfo->setStyleSheet("QLabel { color: black }");

	if (m_attachmentModel.rowCount() > MAX_ATTACHMENT_FILES) {
		this->attachmentSizeInfo->
		     setStyleSheet("QLabel { color: red }");
		this->attachmentSizeInfo->setText(tr(
		    "Warning: The permitted amount (%1) of attachments has been exceeded.")
		        .arg(QString::number(MAX_ATTACHMENT_FILES)));
		return false;
	}


	if (aSize > 0) {
		if (aSize >= 1024) {
			this->attachmentSizeInfo->setText(
			    tr("Total size of attachments is ~%1 KB").
			    arg(aSize/1024));
			if (aSize >= MAX_ATTACHMENT_SIZE_BYTES) {
				this->attachmentSizeInfo->
				     setStyleSheet("QLabel { color: red }");
				this->attachmentSizeInfo->setText(
				    tr("Warning: Total size of attachments is larger than %1 MB!")
				    .arg(QString::number(
				        MAX_ATTACHMENT_SIZE_MB)));
				return false;
			}
		} else {
			this->attachmentSizeInfo->setText(tr(
			    "Total size of attachments is ~%1 B").arg(aSize));
		}
	} else {
		this->attachmentSizeInfo->setText(
		    tr("Total size of attachments is %1 B").arg(aSize));
	}

	return true;
}


/* ========================================================================= */
/*
 * Dialog informs user that message contains one or more PDZs.
 */
int DlgSendMessage::showInfoAboutPDZ(int pdzCnt)
/* ========================================================================= */
{
	QString title;
	QString info;

	if (pdzCnt > 1) {
		title = tr("Message contains non-OVM recipients.");
		info = tr("Your message contains %1 non-OVM recipients "
		    "therefore this message will be sent as a "
		    "commercial messages (PDZ) for these recipients.").
		    arg(pdzCnt);
		info += "\n\n";
		info += tr("Do you want to send all messages?");
	} else {
		title = tr("Message contains non-OVM recipient.");
		info = tr("Your message contains non-OVM recipient "
		    "therefore this message will be sent as a "
		    "commercial message (PDZ) for this recipient.");
		info += "\n\n";
		info += tr("Do you want to send message?");
	}

	info += "\n\n" + tr("Your remaining credit is ") + m_pdzCredit + " Kč";

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(title);
	msgBox.setInformativeText(info);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	return msgBox.exec();
}

bool DlgSendMessage::buildDocuments(QList<IsdsDocument> &documents) const
{
	/* Load attachments. */
	for (int row = 0; row < m_attachmentModel.rowCount(); ++row) {
		IsdsDocument document;
		QModelIndex index;

		document.isXml = false;

		index = m_attachmentModel.index(row, DbFlsTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.dmFileDescr = index.data().toString();

		/*
		 * First document must have dmFileMetaType set to
		 * FILEMETATYPE_MAIN. Remaining documents have
		 * FILEMETATYPE_ENCLOSURE.
		 */

		/*
		 * Since 2011 Mime Type can be empty and MIME type will
		 * be filled up on the ISDS server. It allows sending files
		 * with special mime types without recognition by application.
		 */
		document.dmMimeType = QStringLiteral("");

		index =
		    m_attachmentModel.index(row, DbFlsTblModel::CONTENT_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}
		document.data = QByteArray::fromBase64(
		    index.data(Qt::DisplayRole).toByteArray());

		documents.append(document);
	}

	return true;
}

bool DlgSendMessage::buildEnvelope(IsdsEnvelope &envelope) const
{
	QString dmType;

	/* Set mandatory fields of envelope. */
	envelope.dmID.clear();
	envelope.dmAnnotation = this->subjectText->text();

	/* Set optional fields. */
	envelope.dmSenderIdent = this->dmSenderIdent->text();
	envelope.dmRecipientIdent = this->dmRecipientIdent->text();
	envelope.dmSenderRefNumber = this->dmSenderRefNumber->text();
	envelope.dmRecipientRefNumber = this->dmRecipientRefNumber->text();
	envelope._using_dmLegalTitleLaw =
	    !this->dmLegalTitleLaw->text().isEmpty();
	if (envelope._using_dmLegalTitleLaw) {
		envelope.dmLegalTitleLaw =
		    this->dmLegalTitleLaw->text().toLong();
	}
	envelope._using_dmLegalTitleYear =
	    !this->dmLegalTitleYear->text().isEmpty();
	if (envelope._using_dmLegalTitleYear) {
		envelope.dmLegalTitleYear =
		    this->dmLegalTitleYear->text().toLong();
	}
	envelope.dmLegalTitleSect = this->dmLegalTitleSect->text();
	envelope.dmLegalTitlePar = this->dmLegalTitlePar->text();
	envelope.dmLegalTitlePoint = this->dmLegalTitlePoint->text();
	envelope.dmPersonalDelivery = this->dmPersonalDelivery->isChecked();

	/* Only OVM can change. */
	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		envelope.dmAllowSubstDelivery = true;
	} else {
		envelope.dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == "I") {
		if (this->payRecipient->isChecked()) {
			dmType = "O";
		} else {
			dmType = "K";
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			envelope.dmRecipientRefNumber = m_dmSenderRefNumber;
		}
	} else {
		if (this->payReplyCheckBox->isChecked()) {
			dmType = "I";
		}
	}

	envelope.dmType = dmType;


	envelope.dmOVM = m_dbEffectiveOVM;

	envelope.dmPublishOwnID = this->dmPublishOwnID->isChecked();

	return true;
}


bool DlgSendMessage::buildEnvelopeWebDatovka(JsonLayer::Envelope &envelope) const
{
	/* Set mandatory fields of envelope. */
	envelope.dmAnnotation = this->subjectText->text();

	/* Set optional fields. */
	envelope.dmSenderIdent = this->dmSenderIdent->text();
	envelope.dmRecipientIdent = this->dmRecipientIdent->text();
	envelope.dmSenderRefNumber = this->dmSenderRefNumber->text();
	envelope.dmRecipientRefNumber = this->dmRecipientRefNumber->text();
	envelope.dmLegalTitleLaw = this->dmLegalTitleLaw->text();
	envelope.dmLegalTitleYear = this->dmLegalTitleYear->text();
	envelope.dmLegalTitleSect = this->dmLegalTitleSect->text();
	envelope.dmLegalTitlePar = this->dmLegalTitlePar->text();
	envelope.dmLegalTitlePoint = this->dmLegalTitlePoint->text();
	envelope.dmPersonalDelivery = this->dmPersonalDelivery->isChecked();
	envelope.dmPublishOwnID = this->dmPublishOwnID->isChecked();
	envelope.dmOVM = m_dbEffectiveOVM;

	/* Only OVM can change. */
	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		envelope.dmAllowSubstDelivery = true;
	} else {
		envelope.dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}

	return true;
}


/* ========================================================================= */
/*
 * Load attachments into json for sending via webdatovka.
 */
bool DlgSendMessage::buildFileListWebDatovka(QList<JsonLayer::File> &fileList)
   const
/* ========================================================================= */
{
	QModelIndex index;

	for (int row = 0; row < m_attachmentModel.rowCount(); ++row) {

		index = m_attachmentModel.index(row, DbFlsTblModel::FNAME_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}

		JsonLayer::File file;
		file.fName = index.data().toString();

		index =
		    m_attachmentModel.index(row, DbFlsTblModel::CONTENT_COL);
		if (!index.isValid()) {
			Q_ASSERT(0);
			continue;
		}

		file.fContent = index.data(Qt::DisplayRole).toByteArray();
		fileList.append(file);
	}

	return true;
}


/* ========================================================================= */
/*
 * Send message/multiple message.
 */
void DlgSendMessage::sendMessage(void)
/* ========================================================================= */
{
	debugSlotCall();

	const QList<BoxContactsModel::PartialEntry> recipEntries(
	    m_recipientTableModel.partialBoxEntries(BoxContactsModel::ANY));

	if (m_isWebDatovkaAccount) {

		/* Get account ID */
		int accountID = getWebDatovkaAccountId(m_userName);

		/* Create recipient list. */
		JsonLayer::Recipient recipient;
		QList<JsonLayer::Recipient> recipientList;
		foreach (const BoxContactsModel::PartialEntry &e,
		         recipEntries) {
			recipient.recipientDbId = e.id;
			recipient.toHands = this->dmToHands->text();
			recipient.recipientName = e.name;
			recipient.recipientAddress = e.address;
			recipientList.append(recipient);
		}

		JsonLayer::Envelope envelope;
		buildEnvelopeWebDatovka(envelope);
		QList<JsonLayer::File> fileList;
		buildFileListWebDatovka(fileList);

		TaskSendMessageMojeId *task;
		task = new (std::nothrow) TaskSendMessageMojeId(m_userName, accountID,
		    recipientList, envelope, fileList);
		task->setAutoDelete(true);
		globWorkPool.assignHi(task);

		return;
	}

	QString detailText;

	/* List of unique identifiers. */
	QList<QString> taskIdentifiers;
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());

	int pdzCnt = 0; /* Number of paid messages. */

	/* Compute number of messages which the sender has to pay for. */
	foreach (const BoxContactsModel::PartialEntry &e, recipEntries) {
		if (e.pdz) {
			++pdzCnt;
		}
	}

	if (pdzCnt > 0) {
		if (m_dmType == "I") {
			if (!this->payRecipient->isChecked()) {
				if (QMessageBox::No == showInfoAboutPDZ(pdzCnt)) {
					return;
				}
			}
		} else {
			if (QMessageBox::No == showInfoAboutPDZ(pdzCnt)) {
				return;
			}
		}
	}

	IsdsMessage message;

	/* Attach envelope and attachment files to message structure. */
	if (!buildDocuments(message.documents)) {
		detailText = tr("An error occurred while loading attachments into message.");
		goto finish;
	}
	if (!buildEnvelope(message.envelope)) {
		detailText = tr("An error occurred during message envelope creation.");
		goto finish;
	}

	this->setCursor(Qt::WaitCursor);
	this->setEnabled(false);

	/*
	 * Generate unique identifiers.
	 * These must be complete before creating first task.
	 */
	foreach (const BoxContactsModel::PartialEntry &e, recipEntries) {
		QString taskIdentifier(m_userName + "_" + e.id + "_" +
		    currentTime.toString() + "_" +
		    DlgChangePwd::generateRandomString(6));

		taskIdentifiers.append(taskIdentifier);
	}
	m_transactIds = taskIdentifiers.toSet();
	m_sentMsgResultList.clear();

	/* Send message to all recipients. */
	for (int i = 0; i < recipEntries.size(); ++i) {
		const BoxContactsModel::PartialEntry &e(recipEntries.at(i));

		/* Clear fields. */
		message.envelope.dmID.clear();

		/* Set new recipient. */
		message.envelope.dbIDRecipient = e.id;
		message.envelope.dmToHands = this->dmToHands->text();

		TaskSendMessage *task;

		task = new (std::nothrow) TaskSendMessage(m_userName, m_dbSet,
		    taskIdentifiers.at(i), message, e.name, e.address, e.pdz);
		task->setAutoDelete(true);
		globWorkPool.assignHi(task);
	}

	return;

finish:
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Send message error"));
	msgBox.setText(tr("It has not been possible to send a message "
	    "to the server Datové schránky."));
	detailText += "\n\n" + tr("The message will be discarded.");
	msgBox.setInformativeText(detailText);
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
	this->close();
}

void DlgSendMessage::collectSendMessageStatus(const QString &userName,
    const QString &transactId, int result, const QString &resultDesc,
    const QString &dbIDRecipient, const QString &recipientName,
    bool isPDZ, qint64 dmId)
{
	debugSlotCall();

	Q_UNUSED(userName);

	if (m_transactIds.end() == m_transactIds.find(transactId)) {
		/* Nothing found. */
		return;
	}

	m_sentMsgResultList.append(TaskSendMessage::ResultData(
	    (enum TaskSendMessage::Result) result, resultDesc,
	    dbIDRecipient, recipientName, isPDZ, dmId));

	if (!m_transactIds.remove(transactId)) {
		logErrorNL("%s", "Could not be able to remove a transaction "
		    "identifier from list of unfinished transactions.");
	}

	if (!m_transactIds.isEmpty()) {
		/* Still has some pending transactions. */
		return;
	}

	/* All transactions finished. */

	this->setCursor(Qt::ArrowCursor);
	this->setEnabled(true);

	int successfullySentCnt = 0;
	QString detailText;

	foreach (const TaskSendMessage::ResultData &resultData,
	         m_sentMsgResultList) {
		if (TaskSendMessage::SM_SUCCESS == resultData.result) {
			++successfullySentCnt;

			if (resultData.isPDZ) {
				detailText += tr(
				    "Message has successfully been sent to "
				    "<i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			} else {
				detailText += tr(
				    "Message has successfully been sent to "
				    "<i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			}
		} else {
			detailText += tr("Message has NOT been sent to "
			    "<i>%1 (%2)</i>. Server says: %3").
			    arg(resultData.recipientName).
			    arg(resultData.dbIDRecipient).
			    arg(resultData.errInfo) + "<br/>";
		}
	}
	m_sentMsgResultList.clear();

	if (m_recipientTableModel.rowCount() == successfullySentCnt) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message sent"));
		msgBox.setText("<b>" +
		    tr("Message has successfully been sent to all recipients.") +
		    "</b>");
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		this->accept(); /* Set return code to accepted. */
	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message sending error"));
		msgBox.setText("<b>" +
		    tr("Message has NOT been sent to all recipients.") +
		    "</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
		}
	}

	emit doActionAfterSentMsgSignal(m_userName, m_lastAttAddPath);
}

/* ========================================================================= */
/*
 * Insert list of databoxes into recipient list.
 */
void DlgSendMessage::insertDataboxesToRecipientList(const QStringList &dbIDs)
/* ========================================================================= */
{
	foreach (const QString &dbID, dbIDs) {

		/* Ignore existent entries. */
		if (m_recipientTableModel.containsBoxId(dbID)) {
			continue;
		}

		// search dbID only
		TaskSearchOwnerFulltext *task =
		    new (std::nothrow) TaskSearchOwnerFulltext(m_userName, dbID,
		        TaskSearchOwnerFulltext::FT_BOX_ID,
		        TaskSearchOwnerFulltext::BT_ALL);
		task->setAutoDelete(false);
		globWorkPool.runSingle(task);

		QList<TaskSearchOwnerFulltext::BoxEntry> foundBoxes(
		    task->m_foundBoxes);
		delete task; task = Q_NULLPTR;

		QString name = tr("Unknown");
		QString address = tr("Unknown");
		QVariant pdz;
		int boxType = -1;

		if (foundBoxes.size() == 1) {
			const TaskSearchOwnerFulltext::BoxEntry &entry(
			    foundBoxes.first());

			name = entry.name;
			address = entry.address;
			boxType = entry.type;
			if (!entry.active) {
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Warning);
				msgBox.setWindowTitle(tr("Data box is not active"));
				msgBox.setText(tr("Recipient with data box ID '%1' "
				    "does not have active data box.").arg(dbID));
				msgBox.setInformativeText(tr("The message can "
				    "not be delivered."));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				msgBox.exec();
				continue;
			}
			if (entry.publicSending) {
				pdz = false;
			} else if (entry.commercialSending) {
				pdz = true;
			} else if (entry.effectiveOVM) {
				pdz = false;
			} else {
				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Critical);
				msgBox.setWindowTitle(tr("Cannot send to data box"));
				msgBox.setText(tr("Cannot send message to recipient "
				    "with data box ID '%1'.").arg(dbID));
				msgBox.setInformativeText(tr("You won't be able as "
				    "user '%1' to send messages into data "
				    "box '%2'.").arg(m_userName).arg(dbID));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				msgBox.exec();
				continue;
			}
		} else if (foundBoxes.isEmpty()) {
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setWindowTitle(tr("Wrong recipient"));
			msgBox.setText(tr("Recipient with data box ID '%1' "
			    "does not exist.").arg(dbID));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
			continue;
		} else {
			Q_ASSERT(0);
			continue;
		}

		m_recipientTableModel.appendData(dbID, boxType, name, address,
		    QString(), pdz);
	}
}


/* ========================================================================= */
/*
 * Slot: Performs action depending on webdatovka message send outcome.
 */
void DlgSendMessage::sendMessageMojeIdAction(const QString &userName,
    const QStringList &result, const QString &error)
/* ========================================================================= */
{
	debugSlotCall();

	Q_UNUSED(error);

	QString detailText;

	if (result.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message sent"));
		msgBox.setText("<b>" +
		    tr("Message has successfully been sent to all recipients.") +
		    "</b>");
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		this->accept(); /* Set return code to accepted. */
		emit doActionAfterSentMsgSignal(userName, m_lastAttAddPath);
	} else {
		for (int i = 0; i < result.count(); ++i) {
			QString msg = result.at(i);
			detailText += msg.replace("§", ": ");
		}
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message sending error"));
		msgBox.setText("<b>" +
		    tr("Message has NOT been sent to all recipients.") +
		    "</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
			emit doActionAfterSentMsgSignal(userName,
			    m_lastAttAddPath);
		}
	}
}

QString DlgSendMessage::getPDZCreditFromISDS(const QString &userName,
    const QString &dbId)
{
	debugFuncCall();

	TaskDownloadCreditInfo *task =
	    new (std::nothrow) TaskDownloadCreditInfo(userName, dbId);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	qint64 credit = task->m_heller;
	delete task; task = Q_NULLPTR;

	if (credit <= 0) {
		return "0";
	}

	return programLocale.toString((float)credit / 100, 'f', 2);
}

bool DlgSendMessage::queryISDSBoxEOVM(const QString &userName,
    const QString &boxId)
{
	bool ret = false;

	TaskSearchOwner::SoughtOwnerInfo soughtInfo(
	    boxId, TaskSearchOwner::BT_FO, QString(), QString(), QString(),
	    QString(), QString());

	TaskSearchOwner *task =
	    new (std::nothrow) TaskSearchOwner(userName, soughtInfo);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	QList<TaskSearchOwner::BoxEntry> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	if (foundBoxes.size() == 1) {
		const TaskSearchOwner::BoxEntry &entry(foundBoxes.first());

		ret = entry.effectiveOVM;
	}

	return ret;
}
