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
#include <QMessageBox>
#include <QMimeDatabase>

#include "dlg_send_message.h"
#include "src/gui/dlg_ds_search.h"
#include "src/gui/dlg_contacts.h"
#include "src/models/accounts_model.h"
#include "src/io/message_db.h"
#include "ui_dlg_send_message.h"
#include "src/io/isds_sessions.h"
#include "src/io/dbs.h"


DlgSendMessage::DlgSendMessage(MessageDb &db, QString &dbId, Action action,
    QTreeView &accountList, QTableView &messageList,
    const AccountModel::SettingsMap &accountInfo,
    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
    QWidget *parent,
    const QString &reSubject, const QString &senderId, const QString &sender,
    const QString &senderAddress, const QString &dmType,
    const QString &dmSenderRefNumber
    )
    : QDialog(parent),
    m_accountList(accountList),
    m_messageList(messageList),
    m_dbId(dbId),
    m_action(action),
    m_accountInfo(accountInfo),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_reSubject(reSubject),
    m_senderId(senderId),
    m_sender(sender),
    m_senderAddress(senderAddress),
    m_dmType(dmType),
    m_dmSenderRefNumber(dmSenderRefNumber),
    m_userName(""),
    m_messDb(db)
{
	m_attachSize = 0;
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
 * Init dialog
 */
void DlgSendMessage::initNewMessageDialog(void)
/* ========================================================================= */
{
	QModelIndex index;
	const QStandardItem *item;

	this->recipientTableWidget->setColumnWidth(0,70);
	this->recipientTableWidget->setColumnWidth(1,180);
	this->recipientTableWidget->setColumnWidth(2,240);

	this->attachmentTableWidget->setColumnWidth(0,150);
	this->attachmentTableWidget->setColumnWidth(1,40);
	this->attachmentTableWidget->setColumnWidth(2,120);

	AccountModel *accountModel = dynamic_cast<AccountModel *>(
	    m_accountList.model());
	index = m_accountList.currentIndex();
	Q_ASSERT(index.isValid()); /* TODO -- Deal with invalid. */

	item = accountModel->itemFromIndex(index);
	Q_ASSERT(0 != item);
	const QStandardItem *accountItemTop = AccountModel::itemTop(item);
	const AccountModel::SettingsMap &itemSettings =
	    accountItemTop->data(ROLE_ACNT_CONF_SETTINGS).toMap();

	this->replyLabel->hide();
	this->replyLabel->setEnabled(false);

	QString dbOpenAddressing = "";

	if (!m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			dbOpenAddressing =
			    " - " + tr("sending of PDZ: enabled");
		} else {
			dbOpenAddressing =
			    " - " + tr("sending of PDZ: disabled");
		}
	}

	this->fromUser->setText("<strong>" + accountItemTop->text() +
	    "</strong>" + " (" + itemSettings[USER].toString() + ") - "
	    + m_dbType + dbOpenAddressing);

	index = m_messageList.currentIndex();
	m_userName = itemSettings[USER].toString();

	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	connect(this->payReply, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalFormAndSet(int)));

	if (ACT_REPLY == m_action) {
		this->subjectText->setText("Re: " + m_reSubject);
		this->dmRecipientRefNumber->setText(m_dmSenderRefNumber);
		int row = this->recipientTableWidget->rowCount();
		this->recipientTableWidget->insertRow(row);

		this->payRecipient->setEnabled(false);
		this->payRecipient->setChecked(false);
		this->payRecipient->hide();

		QString pdz;
		if (!m_dbEffectiveOVM) {
			pdz = getUserInfoFormIsds(m_senderId);
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

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(m_senderId);
		this->recipientTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(m_sender);
		this->recipientTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(m_senderAddress);
		this->recipientTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(pdz);

		item->setTextAlignment(Qt::AlignCenter);
		this->recipientTableWidget->setItem(row,3,item);
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
	}

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
	    SLOT(deleteAttachmentFile()));
	connect(this->openAttachment, SIGNAL(clicked()), this,
	    SLOT(openAttachmentFile()));

	connect(this->recipientTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem *)), this,
	    SLOT(recItemSelect()));

	connect(this->attachmentTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem *)), this,
	    SLOT(attItemSelect()));

	connect(this->subjectText, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));

	connect(this->attachmentTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(checkInputFields()));
	connect(this->attachmentTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	this->recipientTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->attachmentTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	connect(this->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	//connect(this->sendButton, SIGNAL(clicked(bool)), this, SLOT(accept()));

	pingTimer = new QTimer(this);
	pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(pingTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));

	this->attachmentWarning->setStyleSheet("QLabel { color: red }");
	this->attachmentWarning->hide();

	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		this->dmAllowSubstDelivery->setEnabled(false);
		this->dmAllowSubstDelivery->hide();
	}
}


/* ========================================================================= */
/*
 * return dbEffectiveOVM for recipient
 */
QString DlgSendMessage::getUserInfoFormIsds(QString idDbox)
/* ========================================================================= */
{
	QString str = tr("no");
	struct isds_list *box = NULL;
	struct isds_PersonName *personName = NULL;
	struct isds_Address *address = NULL;
	isds_DbType dbType = DBTYPE_FO;

	isds_DbOwnerInfo_search(&box, m_userName, idDbox, dbType, "",
	    personName, "", NULL, address, "", "", "", "", "", 0, false, false);

	if (0 != box) {
		isds_DbOwnerInfo *item = (isds_DbOwnerInfo *) box->data;
		Q_ASSERT(0 != item);
		str = *item->dbEffectiveOVM ? tr("no") : tr("yes");
	}

	isds_PersonName_free(&personName);
	isds_Address_free(&address);
	isds_list_free(&box);

	return str;
}


/* ========================================================================= */
/*
 * Ping isds server, test if connection on isds server is active
 */
void DlgSendMessage::pingIsdsServer(void)
/* ========================================================================= */
{
	if (isdsSessions.isConnectToIsds(m_accountInfo.userName())) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}


/* ========================================================================= */
/*
 * Add file to attachment table widget
 */
void DlgSendMessage::addAttachmentFile(void)
/* ========================================================================= */
{
	QFileDialog dialog(this);
	dialog.setDirectory(QDir::homePath());
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList fileNames;

	if (dialog.exec()) {
		fileNames = dialog.selectedFiles();
	}

	for (int i = 0; i < fileNames.count(); ++i) {

		int size = 0;
		QString filename = "";
		QFile attFile(fileNames[i]);
		size = attFile.size();
		m_attachSize += size;
		QFileInfo fileInfo(attFile.fileName());
		filename = fileInfo.fileName();
		QMimeDatabase db;
		QMimeType type = db.mimeTypeForFile(attFile);

		int row = this->attachmentTableWidget->rowCount();
		this->attachmentTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(filename);
		this->attachmentTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText("");
		this->attachmentTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(type.name());
		this->attachmentTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(QString::number(size));
		this->attachmentTableWidget->setItem(row,3,item);
		item = new QTableWidgetItem;
		item->setText(fileNames[i]);
		this->attachmentTableWidget->setItem(row,4,item);
	}
}


/* ========================================================================= */
/*
 * Selection of attachment item
 */
void DlgSendMessage::attItemSelect(void)
/* ========================================================================= */
{
	this->removeAttachment->setEnabled(true);
	this->openAttachment->setEnabled(true);
}


/* ========================================================================= */
/*
 * Enable/disable optional fields in dialog
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
 * Check all intputs when any item was changed in the tablewidget
 */
void DlgSendMessage::tableItemInsRem(void)
/* ========================================================================= */
{
	m_attachSize = cmptAttachmentSize();
	checkInputFields();
}


/* ========================================================================= */
/*
 * Show/hide optional fields in dialog
 */
void DlgSendMessage::showOptionalForm(int state)
/* ========================================================================= */
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


/* ========================================================================= */
/*
 * Show/hide optional fields in dialog and set any items
 */
void DlgSendMessage::showOptionalFormAndSet(int state)
/* ========================================================================= */
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);

	checkInputFields();

	if (Qt::Unchecked == state) {
		this->labeldmSenderRefNumber->setStyleSheet("QLabel { color: black }");
		this->labeldmSenderRefNumber->setText(tr("Your reference number:"));
		disconnect(this->dmSenderRefNumber, SIGNAL(textChanged(QString)),
		this, SLOT(checkInputFields()));
	} else {
		this->labeldmSenderRefNumber->setStyleSheet("QLabel { color: red }");
		this->labeldmSenderRefNumber->setText(tr("Enter reference number:"));
		this->dmSenderRefNumber->setFocus();

		connect(this->dmSenderRefNumber, SIGNAL(textChanged(QString)),
		this, SLOT(checkInputFields()));
	}
}



/* ========================================================================= */
/*
 * Add recipient from local contact list
 */
void DlgSendMessage::addRecipientFromLocalContact(void)
/* ========================================================================= */
{
	QDialog *dlg_cont = new DlgContacts(m_messDb, m_dbId,
	    *(this->recipientTableWidget), m_dbType, m_dbEffectiveOVM,
	    m_dbOpenAddressing, this, m_userName);
	dlg_cont->show();
}


/* ========================================================================= */
/*
 * Delete file (item) from attachment table widget
 */
void DlgSendMessage::deleteAttachmentFile(void)
/* ========================================================================= */
{
	int row = this->attachmentTableWidget->currentRow();
	if (row >= 0) {
		this->attachmentTableWidget->removeRow(row);
		this->removeAttachment->setEnabled(false);
		this->openAttachment->setEnabled(false);
	}
}


/* ========================================================================= */
/*
 * Get attachment size when anz item was removed from tablewidget
 */
int DlgSendMessage::cmptAttachmentSize(void)
/* ========================================================================= */
{
	int attachSize = 0;

	for (int i = 0; i < this->attachmentTableWidget->rowCount(); i++) {
		attachSize += this->attachmentTableWidget->item(i,3)->text().
		    toInt();
	}

	return attachSize;
}


/* ========================================================================= */
/*
 * Check non-empty mandatory items in send message dialog
 */
void DlgSendMessage::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled = !this->subjectText->text().isEmpty()
		    && (this->recipientTableWidget->rowCount() > 0)
		    && (this->attachmentTableWidget->rowCount() > 0);

	if (m_attachSize <= MAX_ATTACHMENT_SIZE) {
		this->attachmentWarning->hide();
	} else {
		this->attachmentWarning->show();
		buttonEnabled = false;
	}

	if (this->payReply->isChecked()) {
		if (this->dmSenderRefNumber->text().isEmpty()) {
			buttonEnabled = false;
		}
	}

	this->sendButton->setEnabled(buttonEnabled);
}


/* ========================================================================= */
/*
 * Delete recipient from table widget
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
 * Find recipent in the ISDS
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
	QModelIndex selectedIndex = this->attachmentTableWidget->currentIndex();

	Q_ASSERT(selectedIndex.isValid());
	if (!selectedIndex.isValid()) {
		return;
	}

	QModelIndex fileNameIndex =
	    selectedIndex.sibling(selectedIndex.row(), 4);
	Q_ASSERT(fileNameIndex.isValid());
	if(!fileNameIndex.isValid()) {
		return;
	}
	QString fileName = fileNameIndex.data().toString();
	Q_ASSERT(!fileName.isEmpty());

	if (fileName.isEmpty()) {
		return;
	}
	QDesktopServices::openUrl(QUrl("file:///" + fileName));
}


/* ========================================================================= */
/*
 * Free document list
 */
static
void isds_document_free_void(void **document)
/* ========================================================================= */
{
	isds_document_free((struct isds_document **) document);
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

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(title);
	msgBox.setInformativeText(info);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	return msgBox.exec();
}



/* ========================================================================= */
/*
 * Send message/multiple message
 */
void DlgSendMessage::sendMessage(void)
/* ========================================================================= */
{
	isds_error status = IE_ERROR;
	QString errorMsg;
	QString detailText = "";

	/* Sent message. */
	struct isds_message *sent_message = NULL;
	/* All remaining structures are created to be a part of the message. */
	struct isds_document *document = NULL; /* Attachment. */
	struct isds_list *documents = NULL; /* Attachment list. */
	struct isds_envelope *sent_envelope = NULL; /* Message envelope. */
	struct isds_list *last = NULL; /* No need to free it explicitly. */

	/* List of send message result */
	QList<sendMsgResultStruct> sendMsgResultList;
	sendMsgResultList.clear();

	int successSendCnt = 0;
	int pdzCnt = 0; /* Number of paid messages. */
	QString dmType;

	/* Compute number of messages which the sender has to pay for. */
	for (int i = 0; i < this->recipientTableWidget->rowCount(); ++i) {
		if (this->recipientTableWidget->item(i, 3)->text() ==
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

	sent_envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));
	if (sent_envelope == NULL) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	memset(sent_envelope, 0, sizeof(struct isds_envelope));

	sent_message = (struct isds_message *)
	    malloc(sizeof(struct isds_message));
	if (sent_message == NULL) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	memset(sent_message, 0, sizeof(struct isds_message));

	/* Load attachments. */
	for (int i = 0; i < this->attachmentTableWidget->rowCount(); ++i) {

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			errorMsg = "Out of memory.";
			goto finish;
		}
		memset(document, 0, sizeof(struct isds_document));

		/* Set document content. */
		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;
		document->dmFileDescr = strdup(this->attachmentTableWidget->
		    item(i, 0)->text().toStdString().c_str());
		if (NULL == document->dmFileDescr) {
			errorMsg = "Out of memory.";
			goto finish;
		}

		if (0 == i) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

		document->dmMimeType = strdup(this->attachmentTableWidget->
		    item(i, 2)->text().toStdString().c_str());
		if (NULL == document->dmMimeType) {
			errorMsg = "Out of memory.";
			goto finish;
		}

		QString filePath =
		    this->attachmentTableWidget->item(i, 4)->text();
		QFile file(filePath);
		if (file.exists()) {
			if (!file.open(QIODevice::ReadOnly)) {
				qDebug("Couldn't open the file");
				errorMsg = "Couldn't open file '" +
				    filePath + "'.";
				goto finish;
			}
			QByteArray bytes = file.readAll();
			document->data_length = bytes.size();
			document->data = malloc(bytes.size());
			if (NULL == document->data) {
				errorMsg = "Out of memory.";
				goto finish;
			}
			memcpy(document->data, bytes.data(),
			    document->data_length);
		}

		/* Add document on the list of document. */
		struct isds_list *newListItem = (struct isds_list *) malloc(
		    sizeof(struct isds_list));
		if (NULL == newListItem) {
			errorMsg = "Out of memory.";
			goto finish;
		}
		newListItem->data = document; document = NULL;
		newListItem->next = NULL;
		newListItem->destructor = isds_document_free_void;
		if (last == NULL) {
			documents = last = newListItem;
		} else {
			last->next = newListItem;
			last = newListItem;
		}
	}

	/* Set mandatory fields of envelope. */
	sent_envelope->dmID = NULL;
	sent_envelope->dmAnnotation =
	    strdup(this->subjectText->text().toStdString().c_str());
	if (NULL == sent_envelope->dmAnnotation) {
		errorMsg = "Out of memory.";
		goto finish;
	}

	/* Set optional fields. */
	if (!this->dmSenderIdent->text().isEmpty()) {
		sent_envelope->dmSenderIdent =
		    strdup(this->dmSenderIdent->text().toStdString().c_str());
		if (NULL == sent_envelope->dmSenderIdent) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmRecipientIdent->text().isEmpty()) {
		sent_envelope->dmRecipientIdent =
		    strdup(this->dmRecipientIdent->text().toStdString().c_str());
		if (NULL == sent_envelope->dmRecipientIdent) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmSenderRefNumber->text().isEmpty()) {
		sent_envelope->dmSenderRefNumber =
		    strdup(this->dmSenderRefNumber->text().toStdString().c_str());
		if (NULL == sent_envelope->dmSenderRefNumber) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmRecipientRefNumber->text().isEmpty()) {
		sent_envelope->dmRecipientRefNumber =
		    strdup(this->dmRecipientRefNumber->text().toStdString().c_str());
		if (NULL == sent_envelope->dmRecipientRefNumber) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmLegalTitleLaw->text().isEmpty()) {
		sent_envelope->dmLegalTitleLaw =
		    (long int *) malloc(sizeof(long int));
		if (NULL == sent_envelope->dmLegalTitleLaw) {
			errorMsg = "Out of memory.";
			goto finish; 
		}
		*sent_envelope->dmLegalTitleLaw =
		    this->dmLegalTitleLaw->text().toLong();
	} else {
		sent_envelope->dmLegalTitleLaw = NULL;
	}
	if (!this->dmLegalTitleYear->text().isEmpty()) {
		sent_envelope->dmLegalTitleYear =
		    (long int *) malloc(sizeof(long int));
		if (NULL == sent_envelope->dmLegalTitleYear) {
			errorMsg = "Out of memory.";
			goto finish;
		}
		*sent_envelope->dmLegalTitleYear =
		    this->dmLegalTitleYear->text().toLong();
	} else {
		sent_envelope->dmLegalTitleYear = NULL;
	}

	if (!this->dmLegalTitleSect->text().isEmpty()) {
		sent_envelope->dmLegalTitleSect =
		    strdup(this->dmLegalTitleSect->text().toStdString().c_str());
		if (NULL == sent_envelope->dmLegalTitleSect) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmLegalTitlePar->text().isEmpty()) {
		sent_envelope->dmLegalTitlePar =
		    strdup(this->dmLegalTitlePar->text().toStdString().c_str());
		if (NULL == sent_envelope->dmLegalTitlePar) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	if (!this->dmLegalTitlePoint->text().isEmpty()) {
		sent_envelope->dmLegalTitlePoint =
		    strdup(this->dmLegalTitlePoint->text().toStdString().c_str());
		if (NULL == sent_envelope->dmLegalTitlePoint) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}
	sent_envelope->dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmPersonalDelivery) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	*sent_envelope->dmPersonalDelivery = this->dmPersonalDelivery->isChecked();

	/* only OVM can changes */
	sent_envelope->dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmAllowSubstDelivery) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		*sent_envelope->dmAllowSubstDelivery = true;
	} else {
		*sent_envelope->dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == "I") {
		if (this->payRecipient->isChecked()) {
			dmType = "O";
		} else {
			dmType = "K";
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			sent_envelope->dmRecipientRefNumber =
			    strdup(m_dmSenderRefNumber.toStdString().c_str());
			if (NULL == sent_envelope->dmRecipientRefNumber) {
				errorMsg = "Out of memory.";
				goto finish;
			}
		}
	} else {
		if (this->payReply->isChecked()) {
			dmType = "I";
		}
	}

	if (!dmType.isEmpty()) {
		sent_envelope->dmType = strdup(dmType.toStdString().c_str());
		if (NULL == sent_envelope->dmType) {
			errorMsg = "Out of memory.";
			goto finish;
		}
	}

	sent_envelope->dmOVM = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmOVM) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	*sent_envelope->dmOVM = m_dbEffectiveOVM;

	sent_envelope->dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == sent_envelope->dmPublishOwnID) {
		errorMsg = "Out of memory.";
		goto finish;
	}
	*sent_envelope->dmPublishOwnID = this->dmPublishOwnID->isChecked();

	/* Set envelope and attachments to message structure. */
	sent_message->documents = documents; documents = NULL;
	sent_message->envelope = sent_envelope; sent_envelope = NULL;

	if (!isdsSessions.isConnectToIsds(m_accountInfo.userName())) {
		goto finish;
	}

	for (int i = 0; i < this->recipientTableWidget->rowCount(); ++i) {

		if (NULL != sent_message->envelope->dbIDRecipient) {
			free(sent_message->envelope->dbIDRecipient);
			sent_message->envelope->dbIDRecipient = NULL;
		}
		sent_message->envelope->dbIDRecipient =
		    strdup(this->recipientTableWidget->item(i,0)->
		    text().toStdString().c_str());
		if (NULL == sent_message->envelope->dbIDRecipient) {
			errorMsg = "Out of memory.";
			goto finish;
		}

		if (NULL != sent_message->envelope->dmToHands) {
			free(sent_message->envelope->dmToHands);
			sent_message->envelope->dmToHands = NULL;
		}
		if (!this->dmToHands->text().isEmpty()) {
			sent_message->envelope->dmToHands =
			    strdup(this->dmToHands->text().toStdString().c_str());
			if (NULL == sent_message->envelope->dmToHands) {
				errorMsg = "Out of memory.";
				goto finish;
			}
		}

		qDebug() << "sending message from user name" << m_userName;
		status = isds_send_message(isdsSessions.session(m_userName),
		    sent_message);

		sendMsgResultStruct sendMsgResults;
		sendMsgResults.dbID = this->recipientTableWidget->
		    item(i,0)->text();
		sendMsgResults.recipientName = this->recipientTableWidget->
		    item(i,1)->text();
		sendMsgResults.dmID = sent_message->envelope->dmID;
		sendMsgResults.isPDZ = (this->recipientTableWidget->
		    item(i, 3)->text() == tr("yes")) ? true : false;
		sendMsgResults.status = (int) status;
		sendMsgResults.errInfo = isds_long_message(isdsSessions.session(m_userName));
		sendMsgResultList.append(sendMsgResults);

		if (status == IE_SUCCESS) {
			successSendCnt++;
		}
	}

	for (int i = 0; i < sendMsgResultList.size(); ++i) {
		if (sendMsgResultList.at(i).status == IE_SUCCESS) {
			if (sendMsgResultList.at(i).isPDZ) {
				detailText += tr("Message was successfully "
				"sent to <i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(sendMsgResultList.at(i).recipientName).
				    arg(sendMsgResultList.at(i).dbID).
				    arg(sendMsgResultList.at(i).dmID) + "<br/>";
			} else {
				detailText += tr("Message was successfully "
				    "sent to <i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(sendMsgResultList.at(i).recipientName).
				    arg(sendMsgResultList.at(i).dbID).
				    arg(sendMsgResultList.at(i).dmID) + "<br/>";
			}
		} else {
			detailText += tr("Message was NOT successfully "
			"sent to <i>%1 (%2)</i>. Server says: %3").
			    arg(sendMsgResultList.at(i).recipientName).
			    arg(sendMsgResultList.at(i).dbID).
			    arg(sendMsgResultList.at(i).errInfo) + "<br/>";
		}
	}

	if (this->recipientTableWidget->rowCount() == successSendCnt) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setWindowTitle(tr("Message was sent"));
		msgBox.setText("<b>"+tr("Message was successfully sent to "
		    "all recipients.")+"</b>");
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.setDefaultButton(QMessageBox::Ok);
		msgBox.exec();

		isds_document_free(&document);
		isds_list_free(&documents);
		isds_envelope_free(&sent_envelope);
		isds_message_free(&sent_message);

		this->accept(); /* Set return code to accepted. */
		return;
	} else {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setWindowTitle(tr("Message was sent with error"));
		msgBox.setText("<b>"+tr("Message was NOT successfully sent "
		    "to all recipients.")+"</b>");
		detailText += "<br/><br/><b>" +
		    tr("Do you want to close the Send message form?") +"</b>";
		msgBox.setInformativeText(detailText);
		msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		if (msgBox.exec() == QMessageBox::Yes) {
			isds_document_free(&document);
			isds_list_free(&documents);
			isds_envelope_free(&sent_envelope);
			isds_message_free(&sent_message);
			this->accept(); /* Set return code to accepted. */
			return;
		} else {
			isds_document_free(&document);
			isds_list_free(&documents);
			isds_envelope_free(&sent_envelope);
			isds_message_free(&sent_message);
			return;
		}
	}

finish:
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(tr("Send message error"));
	msgBox.setText("<b>"+tr("It was not possible to send message "
	    "to the server Datové schránky.")+"</b>");
	detailText += tr("It can be caused by following") +  ":<br/><br/>";
	detailText += "1. " + tr("It was not possible to establish a "
	    "connection to the server.") + "<br/>";
	detailText += "2. " + tr("Authorization on the server "
	    "failed.") + "<br/>";
	detailText += "3. " + tr("Wrong/unsupported MIME type of some file in "
	    "the attachment.") + "<br/>";
	detailText += "4. " + tr("An internal error has occurred in "
	    "the application.") + "<br/>";
	detailText += "<br/><br/><b>" +
	    tr("Do you want to close the Send message form?") + "</b>";
	msgBox.setInformativeText(detailText);
	msgBox.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	if (msgBox.exec() == QMessageBox::No) {
		isds_document_free(&document);
		isds_list_free(&documents);
		isds_envelope_free(&sent_envelope);
		isds_message_free(&sent_message);
		this->close(); /* Set return code to accepted. */
		return;
	}

	/* Functions do nothing on NULL pointers. */
	isds_document_free(&document);
	isds_list_free(&documents);
	isds_envelope_free(&sent_envelope);
	isds_message_free(&sent_message);
}
