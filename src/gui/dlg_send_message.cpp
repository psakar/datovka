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


#include <QDesktopServices>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QMimeDatabase>

#include "dlg_send_message.h"
#include "src/gui/dlg_change_pwd.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/models/accounts_model.h"
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/filesystem.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"
#include "src/views/attachment_table_widget.h"
#include "src/views/table_home_end_filter.h"
#include "src/worker/message_emitter.h"
#include "src/worker/pool.h"
#include "src/worker/task_download_credit_info.h"
#include "src/worker/task_send_message.h"
#include "ui_dlg_send_message.h"


/*
 * Column indexes into recipient table widget.
 */
#define RTW_ID 0
#define RTW_NAME 1
#define RTW_ADDR 2
#define RTW_PDZ 3


DlgSendMessage::DlgSendMessage(
    const QList<Task::AccountDescr> &messageDbSetList,
    Action action, qint64 msgId, const QDateTime &deliveryTime,
    const QString &userName, MainWindow *mv, QWidget *parent)
    : QDialog(parent),
    m_messageDbSetList(messageDbSetList),
    m_msgID(msgId),
    m_deliveryTime(deliveryTime),
    m_dbId(""),
    m_senderName(""),
    m_action(action),
    m_userName(userName),
    m_dbType(""),
    m_dbEffectiveOVM(false),
    m_dbOpenAddressing(false),
    m_lastAttAddPath(""),
    m_pdzCredit("0"),
    m_dmType(""),
    m_dmSenderRefNumber(""),
    m_mv(mv),
    m_isLogged(false),
    m_transactIds(),
    m_sentMsgResultList()
{
	setupUi(this);
	initNewMessageDialog();
}

/* ========================================================================= */
void DlgSendMessage::on_cancelButton_clicked(void)
/* ========================================================================= */
{
	this->close();
}


/* ========================================================================= */
/*
 * Init send message dialogue.
 */
void DlgSendMessage::initNewMessageDialog(void)
/* ========================================================================= */
{
	this->recipientTableWidget->setColumnWidth(0,70);
	this->recipientTableWidget->setColumnWidth(1,180);
	this->recipientTableWidget->setColumnWidth(2,240);

	this->attachmentTableWidget->setColumnWidth(0,150);
	this->attachmentTableWidget->setColumnWidth(1,40);
	this->attachmentTableWidget->setColumnWidth(2,120);

	this->attachmentTableWidget->setColumnHidden(5, true);

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

	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));

	connect(this->payReply, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalFormAndSet(int)));

	this->OptionalWidget->setHidden(true);

	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalForm(int)));

	connect(this->addRecipient, SIGNAL(clicked()), this,
	    SLOT(addRecipientFromLocalContact()));
	connect(this->removeRecipient, SIGNAL(clicked()), this,
	    SLOT(deleteRecipientData()));
	connect(this->findRecipient, SIGNAL(clicked()), this,
	    SLOT(findAndAddRecipient()));

	connect(this->addAttachment, SIGNAL(clicked()), this,
	    SLOT(addAttachmentFile()));
	connect(this->removeAttachment, SIGNAL(clicked()), this,
	    SLOT(deleteSelectedAttachmentFiles()));
	connect(this->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openAttachmentFile()));

	connect(this->recipientTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem *)), this,
	    SLOT(recItemSelect()));

	connect(this->attachmentTableWidget,
	    SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	    this, SLOT(tableItemDoubleClicked(QTableWidgetItem *)));

	connect(this->enterDbIdpushButton, SIGNAL(clicked()), this,
	    SLOT(addDbIdToRecipientList()));

	connect(this->subjectText, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(this->attachmentTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(this->attachmentTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(this->attachmentTableWidget->model(),
	    SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this,
	    SLOT(attachmentDataChanged(QModelIndex, QModelIndex, QVector<int>)));
	connect(this->attachmentTableWidget->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(attachmentSelectionChanged(QItemSelection, QItemSelection)));

	this->recipientTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->attachmentTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->recipientTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
	this->attachmentTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));

	connect(this->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(&globMsgProcEmitter,
	    SIGNAL(sendMessageFinished(QString, QString, int, QString,
	        QString, QString, bool, qint64)), this,
	    SLOT(collectSendMessageStatus(QString, QString, int, QString,
	        QString, QString, bool, qint64)));

	pingTimer = new QTimer(this);
	pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(pingTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));

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
			this->payReply->setEnabled(true);
			this->payReply->show();
		} else {
			this->payReply->setEnabled(false);
			this->payReply->hide();
		}

		this->payRecipient->setEnabled(false);
		this->payRecipient->hide();
		if (ACT_NEW_FROM_TMP == m_action) {
			fillDlgFromTmpMsg();
		}
	}
}


/* ========================================================================= */
/*
 * Slot: Set info data and database for selected account
 */
void DlgSendMessage::setAccountInfo(int item)
/* ========================================================================= */
{
	debugSlotCall();

	/* get username for selectet account */
	const QString userName = this->fromComboBox->itemData(item).toString();

	if (!userName.isEmpty()) {
		/* if account was changed, remove all recipients */
		if (m_userName != userName) {
			for (int i = this->recipientTableWidget->rowCount() - 1;
			    i >= 0; --i) {
				this->recipientTableWidget->removeRow(i);
			}
		}
		m_userName = userName;
	}

	struct isds_ctx *session = NULL;

	m_isLogged = true;

	if (!isdsSessions.isConnectedToIsds(m_userName)) {
		if (!m_mv->connectToIsds(m_userName, m_mv)) {
			m_isLogged = false;
		}
	}
	session = isdsSessions.session(m_userName);
	if (NULL == session) {
		logErrorNL("%s", "Missing ISDS session.");
		m_isLogged = false;
	}

	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		if (acnt.userName == m_userName) {
			m_dbSet = acnt.messageDbSet;
			break;
		}
	}

	const AccountModel::SettingsMap &accountInfo =
	    AccountModel::globAccounts[m_userName];
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
	if (m_dbOpenAddressing) {
		m_pdzCredit = getPDZCreditFromISDS(m_userName, m_dbId);
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
 * Func: Return remaining PDZ credit.
 */
QString DlgSendMessage::getPDZCreditFromISDS(const QString &userName,
    const QString &dbId)
/* ========================================================================= */
{
	debugFuncCall();

	TaskDownloadCreditInfo *task;

	task = new (std::nothrow) TaskDownloadCreditInfo(userName, dbId);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	long int credit = task->m_heller;
	delete task;

	if (credit <= 0) {
		return "0";
	}

	return programLocale.toString((float)credit / 100, 'f', 2);
}


/* ========================================================================= */
/*
 * Slot is fired when user double clicked on attachment item - open file.
 */
void DlgSendMessage::tableItemDoubleClicked(QTableWidgetItem *item)
/* ========================================================================= */
{
	debugSlotCall();

	/* Unused. */
	(void) item;

	openAttachmentFile();
}


/* ========================================================================= */
/*
 * Whenever any data in attachment table change.
 */
void DlgSendMessage::attachmentDataChanged(const QModelIndex &topLeft,
    const QModelIndex &bottomRight, const QVector<int> &roles)
/* ========================================================================= */
{
	/* Unused. */
	(void) topLeft;
	(void) bottomRight;
	(void) roles;

	debugSlotCall();

	calculateAndShowTotalAttachSize();
}


/* ========================================================================= */
/*
 * Whenever attachment selection changes.
 */
void DlgSendMessage::attachmentSelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
/* ========================================================================= */
{
	debugSlotCall();

	/* Unused. */
	(void) selected;
	(void) deselected;

	QModelIndexList selectedIndexes;
	{
		QItemSelectionModel *selectionModel =
		    this->attachmentTableWidget->selectionModel();
		if (0 == selectionModel) {
			Q_ASSERT(0);
			return;
		}
		selectedIndexes = selectionModel->selectedRows(0);
	}

	this->removeAttachment->setEnabled(selectedIndexes.size() > 0);
	this->openAttachment->setEnabled(1 == selectedIndexes.size());
}


/* ========================================================================= */
/*
 * Func: Fill Send Message Dialog as reply.
 */
void DlgSendMessage::fillDlgAsReply(void)
/* ========================================================================= */
{
	debugFuncCall();

	bool hideOptionalWidget = true;

	this->fromComboBox->setEnabled(false);

	MessageDb *messageDb = m_dbSet->accessMessageDb(m_deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(m_msgID);
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

	this->OptionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);
	this->payRecipient->setEnabled(false);
	this->payRecipient->setChecked(false);
	this->payRecipient->hide();

	QString pdz;
	if (!m_dbEffectiveOVM) {
		pdz = DlgContacts::getUserInfoFromIsds(m_userName,
		    envData.dbIDSender);
		this->payReply->show();
		this->payReply->setEnabled(true);
	} else {
		this->payReply->setEnabled(false);
		this->payReply->hide();
		pdz = tr("no");
	}

	if (m_dmType == "I") {
		this->addRecipient->setEnabled(false);
		this->removeRecipient->setEnabled(false);
		this->findRecipient->setEnabled(false);
		this->replyLabel->show();
		this->replyLabel->setEnabled(true);
		this->payReply->hide();
		this->payReply->setEnabled(false);
		this->payRecipient->setEnabled(true);
		this->payRecipient->setChecked(true);
		this->payRecipient->show();
		pdz = tr("yes");
	}

	int row = this->recipientTableWidget->rowCount();
	this->recipientTableWidget->insertRow(row);
	QTableWidgetItem *item = new QTableWidgetItem;
	item->setText(envData.dbIDSender);
	this->recipientTableWidget->setItem(row, RTW_ID, item);
	item = new QTableWidgetItem;
	item->setText(envData.dmSender);
	this->recipientTableWidget->setItem(row, RTW_NAME, item);
	item = new QTableWidgetItem;
	item->setText(envData.dmSenderAddress);
	this->recipientTableWidget->setItem(row, RTW_ADDR, item);
	item = new QTableWidgetItem;
	item->setText(pdz);
	item->setTextAlignment(Qt::AlignCenter);
	this->recipientTableWidget->setItem(row, RTW_PDZ, item);
}


/* ========================================================================= */
/*
 * Func: Fill Send Message Dialog from template message.
 */
void DlgSendMessage::fillDlgFromTmpMsg(void)
/* ========================================================================= */
{
	debugFuncCall();

	bool hideOptionalWidget = true;

	MessageDb *messageDb = m_dbSet->accessMessageDb(m_deliveryTime, false);
	Q_ASSERT(0 != messageDb);

	MessageDb::PartialEnvelopeData envData =
	    messageDb->msgsReplyData(m_msgID);
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

	this->OptionalWidget->setHidden(hideOptionalWidget);
	this->optionalFieldCheckBox->setChecked(!hideOptionalWidget);

	QString pdz;
	if (!m_dbEffectiveOVM) {
		pdz = DlgContacts::getUserInfoFromIsds(m_userName,
		    envData.dbIDRecipient);
		this->payReply->show();
		this->payReply->setEnabled(true);
	} else {
		this->payReply->setEnabled(false);
		this->payReply->hide();
		pdz = tr("no");
	}

	/* message is received -> recipient == sender */
	if (m_dbId != envData.dbIDRecipient) {
		int row = this->recipientTableWidget->rowCount();
		this->recipientTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(envData.dbIDRecipient);
		this->recipientTableWidget->setItem(row, RTW_ID, item);
		item = new QTableWidgetItem;
		item->setText(envData.dmRecipient);
		this->recipientTableWidget->setItem(row, RTW_NAME, item);
		item = new QTableWidgetItem;
		item->setText(envData.dmRecipientAddress);
		this->recipientTableWidget->setItem(row, RTW_ADDR, item);
		item = new QTableWidgetItem;
		item->setText(pdz);
		item->setTextAlignment(Qt::AlignCenter);
		this->recipientTableWidget->setItem(row, RTW_PDZ, item);
	}

	/* fill attachments from template message */
	QList<MessageDb::FileData> msgFileList =
	    messageDb->getFilesFromMessage(m_msgID);

	foreach (const MessageDb::FileData &fileData, msgFileList) {
		int row = this->attachmentTableWidget->rowCount();
		this->attachmentTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(fileData.dmFileDescr);
		this->attachmentTableWidget->setItem(row, ATW_FILE, item);
		item = new QTableWidgetItem;
		item->setText("");
		this->attachmentTableWidget->setItem(row, ATW_TYPE, item);
		item = new QTableWidgetItem;
		item->setText(tr("unknown"));
		this->attachmentTableWidget->setItem(row, ATW_MIME, item);
		item = new QTableWidgetItem;
		item->setText(QString::number(
		    base64RealSize(fileData.dmEncodedContent)));
		this->attachmentTableWidget->setItem(row, ATW_SIZE, item);
		item = new QTableWidgetItem;
		item->setText(tr("local database"));
		this->attachmentTableWidget->setItem(row, ATW_PATH, item);
		item = new QTableWidgetItem;
		item->setData(Qt::DisplayRole, fileData.dmEncodedContent);
		this->attachmentTableWidget->setItem(row, ATW_DATA, item);
	}
}


/* ========================================================================= */
/*
 * Ping isds server, test if connection on isds server is active.
 */
void DlgSendMessage::pingIsdsServer(void)
/* ========================================================================= */
{
	if (isdsSessions.isConnectedToIsds(m_userName)) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}


/* ========================================================================= */
/*
 * Add file to attachment table widget.
 */
void DlgSendMessage::addAttachmentFile(void)
/* ========================================================================= */
{
	QFileDialog dialog(this);
	dialog.setDirectory(m_lastAttAddPath);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList fileNames;

	if (dialog.exec()) {
		fileNames = dialog.selectedFiles();
		if (!globPref.use_global_paths) {
			m_lastAttAddPath = dialog.directory().absolutePath();
		}
	}

	foreach (const QString &fileName, fileNames) {

//		int fileSize = QFile(fileName).size();
//		if (fileSize > MAX_ATTACHMENT_SIZE) {
//			QMessageBox::warning(this, tr("Wrong file size"),
//			    tr("File '%1' could not be added into attachment "
//			    "because its size is bigger than 20MB.").
//			    arg(fileName),
//			    QMessageBox::Ok);
//			continue;
//		}

		int fileSize = this->attachmentTableWidget->addFile(fileName);
		if (fileSize <= 0) {
			continue;
		}
	}
}


/* ========================================================================= */
/*
 * Enable/disable optional fields in dialog.
 */
void DlgSendMessage::recItemSelect(void)
/* ========================================================================= */
{
	if (m_dmType == "I") {
		this->removeRecipient->setEnabled(false);
	} else {
		this->removeRecipient->setEnabled(true);
	}
}


/* ========================================================================= */
/*
 * Show/hide optional fields in dialog.
 */
void DlgSendMessage::showOptionalForm(int state)
/* ========================================================================= */
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


/* ========================================================================= */
/*
 * Show/hide optional fields in dialog and set any items.
 */
void DlgSendMessage::showOptionalFormAndSet(int state)
/* ========================================================================= */
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);

	checkInputFields();

	if (Qt::Unchecked == state) {
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


/* ========================================================================= */
/*
 * Add recipient from local contact list.
 */
void DlgSendMessage::addRecipientFromLocalContact(void)
/* ========================================================================= */
{
	QDialog *dlg_cont = new DlgContacts(*m_dbSet, m_dbId,
	    *(this->recipientTableWidget), m_dbType, m_dbEffectiveOVM,
	    m_dbOpenAddressing, this, m_userName);
	dlg_cont->show();
}


/* ========================================================================= */
/*
 * Remove file (item) from attachment table widget.
 */
void DlgSendMessage::deleteSelectedAttachmentFiles(void)
/* ========================================================================= */
{
	debugSlotCall();

	QModelIndexList firstMsgColumnIdxs =
	   this->attachmentTableWidget->selectionModel()->selectedRows(0);

	for (int i = firstMsgColumnIdxs.size() - 1; i >= 0; --i) {
		/*
		 * Delete rows in reverse order so that we don't mess with
		 * indexes.
		 */
		int row = firstMsgColumnIdxs.at(i).row();
		this->attachmentTableWidget->removeRow(row);
	}

	calculateAndShowTotalAttachSize();
}


/* ========================================================================= */
/*
 * Calculate total attachment size when an item was added/removed in the table.
 */
void DlgSendMessage::calculateAndShowTotalAttachSize(void)
/* ========================================================================= */
{
	int aSize = 0;

	for (int i = 0; i < this->attachmentTableWidget->rowCount(); i++) {
		QTableWidgetItem *item =
		    this->attachmentTableWidget->item(i, ATW_SIZE);
		if (0 != item) {
			aSize += item->text().toInt();
		}
	}

	this->attachmentSizeInfo->setStyleSheet("QLabel { color: black }");

	if (aSize > 0) {
		if (aSize >= 1024) {
			this->attachmentSizeInfo->setText(
			    tr("Total size of attachments is ~%1 KB").
			    arg(aSize/1024));
			if (aSize >= MAX_ATTACHMENT_SIZE) {
				this->attachmentSizeInfo->
				     setStyleSheet("QLabel { color: red }");
				this->attachmentSizeInfo->setText(
				    tr("Warning: Total size of attachments "
				    "is larger than 10 MB!"));
			}
		} else {
			this->attachmentSizeInfo->setText(
			   tr("Total size of attachments is ~%1 B").arg(aSize));
		}
	} else {
		this->attachmentSizeInfo->setText(
		    tr("Total size of attachments is %1 B").arg(aSize));
	}
}


/* ========================================================================= */
/*
 * Check non-empty mandatory items in send message dialog.
 */
void DlgSendMessage::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled = !this->subjectText->text().isEmpty()
		    && (this->recipientTableWidget->rowCount() > 0)
		    && (this->attachmentTableWidget->rowCount() > 0);

	if (this->payReply->isChecked()) {
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


/* ========================================================================= */
/*
 * Delete recipient from table widget.
 */
void DlgSendMessage::deleteRecipientData(void)
/* ========================================================================= */
{
	int row = this->recipientTableWidget->currentRow();
	if (row >= 0) {
		this->recipientTableWidget->removeRow(row);
		this->removeRecipient->setEnabled(false);
	}
}


/* ========================================================================= */
/*
 * Find recipent in the ISDS.
 */
void DlgSendMessage::findAndAddRecipient(void)
/* ========================================================================= */
{
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_ADDNEW,
	    this->recipientTableWidget, m_dbType, m_dbEffectiveOVM,
	    m_dbOpenAddressing, this, m_userName);
	dsSearch->show();
}


/* ========================================================================= */
/*
 * Open attachment in default application.
 */
void DlgSendMessage::openAttachmentFile(void)
/* ========================================================================= */
{
	QModelIndex selectedIndex =
	    this->attachmentTableWidget->currentIndex();

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	QModelIndex fileNameIndex =
	    selectedIndex.sibling(selectedIndex.row(), ATW_FILE);
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		return;
	}
	QString attachName = fileNameIndex.data().toString();
	Q_ASSERT(!attachName.isEmpty());
	if (attachName.isEmpty()) {
		return;
	}
	attachName.replace(QRegExp("\\s"), "_").replace(
	    QRegExp("[^a-zA-Z\\d\\.\\-_]"), "x");
	/* TODO -- Add message id into file name? */
	QString fileName = TMP_ATTACHMENT_PREFIX + attachName;

	/* Get data from base64. */
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(),
	    ATW_DATA);
	Q_ASSERT(dataIndex.isValid());
	if (!dataIndex.isValid()) {
		return;
	}

	QByteArray data =
	    QByteArray::fromBase64(dataIndex.data().toByteArray());

	fileName = writeTemporaryFile(fileName, data);
	if (!fileName.isEmpty()) {
		QDesktopServices::openUrl(QUrl("file:///" + fileName));
		/* TODO -- Handle openUrl() return value. */
	} else {
		QMessageBox::warning(this,
		    tr("Error opening attachment."),
		    tr("Cannot write file '%1'.").arg(fileName),
		    QMessageBox::Ok);
	}
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
	for (int i = 0; i < this->attachmentTableWidget->rowCount(); ++i) {
		IsdsDocument document;

		document.isXml = false;

		document.dmFileDescr = this->attachmentTableWidget->
		    item(i, ATW_FILE)->text();

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
		document.dmMimeType = "";

		document.data = QByteArray::fromBase64(
		    this->attachmentTableWidget->item(i, ATW_DATA)->
		         data(Qt::DisplayRole).toByteArray());

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
	envelope.dmSenderIdent = this->subjectText->text();
	envelope.dmRecipientIdent = this->dmSenderIdent->text();
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
		if (this->payReply->isChecked()) {
			dmType = "I";
		}
	}

	envelope.dmType = dmType;


	envelope.dmOVM = m_dbEffectiveOVM;

	envelope.dmPublishOwnID = this->dmPublishOwnID->isChecked();

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

	QString detailText;

	/* List of unique identifiers. */
	QList<QString> taskIdentifiers;
	const QDateTime currentTime(QDateTime::currentDateTimeUtc());

	int pdzCnt = 0; /* Number of paid messages. */

	/* Compute number of messages which the sender has to pay for. */
	for (int i = 0; i < this->recipientTableWidget->rowCount(); ++i) {
		if (this->recipientTableWidget->item(i, RTW_PDZ)->text() ==
		    tr("yes")) {
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

	/*
	 * Generate unique identifiers.
	 * These must be complete before creating first task.
	 */
	for (int row = 0; row < this->recipientTableWidget->rowCount(); ++row) {
		QString taskIdentifier =
		    m_userName + "_" +
		    this->recipientTableWidget->item(row, RTW_ID)->text() +
		    "_" + currentTime.toString() + "_" +
		    DlgChangePwd::generateRandomString(6);

		taskIdentifiers.append(taskIdentifier);
	}
	m_transactIds = taskIdentifiers.toSet();
	m_sentMsgResultList.clear();

	/* Send message to all recipients. */
	for (int row = 0; row < this->recipientTableWidget->rowCount(); ++row) {
		/* Clear fields. */
		message.envelope.dmID.clear();

		/* Set new recipient. */
		message.envelope.dbIDRecipient = this->recipientTableWidget->item(row, RTW_ID)->text();
		message.envelope.dmToHands = this->dmToHands->text();

		TaskSendMessage *task;

		task = new (std::nothrow) TaskSendMessage(
		    m_userName, m_dbSet, taskIdentifiers.at(row), message,
		    this->recipientTableWidget->item(row, RTW_NAME)->text(),
		    this->recipientTableWidget->item(row, RTW_ADDR)->text(),
		    this->recipientTableWidget->item(row, RTW_PDZ)->text() == tr("yes"));
		task->setAutoDelete(true);
		globWorkPool.assignHi(task);
	}

	return;

finish:
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Send message error"));
	msgBox.setText(tr("It was not possible to send message "
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

	/* Unused. */
	(void) userName;

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

	int successfullySentCnt = 0;
	QString detailText;

	foreach (const TaskSendMessage::ResultData &resultData,
	         m_sentMsgResultList) {
		if (TaskSendMessage::SM_SUCCESS == resultData.result) {
			++successfullySentCnt;

			if (resultData.isPDZ) {
				detailText += tr("Message was successfully "
				    "sent to <i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			} else {
				detailText += tr("Message was successfully "
				    "sent to <i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(resultData.recipientName).
				    arg(resultData.dbIDRecipient).
				    arg(resultData.dmId) + "<br/>";
			}
		} else {
			detailText += tr("Message was NOT successfully "
			    "sent to <i>%1 (%2)</i>. Server says: %3").
			    arg(resultData.recipientName).
			    arg(resultData.dbIDRecipient).
			    arg(resultData.errInfo) + "<br/>";
		}
	}
	m_sentMsgResultList.clear();

	if (this->recipientTableWidget->rowCount() == successfullySentCnt) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message was sent"));
		msgBox.setText("<b>" +
		    tr("Message was successfully sent to all recipients.") +
		    "</b>");
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();
		this->accept(); /* Set return code to accepted. */
		emit doActionAfterSentMsgSignal(m_userName, m_lastAttAddPath);
	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message was sent with error"));
		msgBox.setText("<b>" +
		    tr("Message was NOT successfully sent to all recipients.") +
		    "</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") + "</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			this->close(); /* Set return code to closed. */
			emit doActionAfterSentMsgSignal(m_userName,
			    m_lastAttAddPath);
		}
	}
}

/* ========================================================================= */
/*
 * Enter DB ID manually.
 */
void DlgSendMessage::addDbIdToRecipientList(void)
/* ========================================================================= */
{
	bool ok = false;

	QString dbID = QInputDialog::getText(this, tr("Databox ID"),
	    tr("Enter Databox ID (7 characters):"), QLineEdit::Normal,
	    NULL, &ok, Qt::WindowStaysOnTopHint);

	if (ok) {
		if (dbID.isEmpty() || dbID.length() != 7) {
			return;
		}

		int row = this->recipientTableWidget->rowCount();

		/* exists dbID in the recipientTableWidget? */
		for (int i = 0; i < row; ++i) {
			if (this->recipientTableWidget->item(i, RTW_ID)->text() ==
			    dbID) {
				return;
			}
		}

		this->recipientTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(dbID);
		this->recipientTableWidget->setItem(row, RTW_ID, item);
		item = new QTableWidgetItem;
		item->setText("Unknown");
		this->recipientTableWidget->setItem(row, RTW_NAME, item);
		item = new QTableWidgetItem;
		item->setText("Unknown");
		this->recipientTableWidget->setItem(row, RTW_ADDR, item);
		item = new QTableWidgetItem;
		item->setText("n/a");
		this->recipientTableWidget->setItem(row, RTW_PDZ, item);
	}
}
