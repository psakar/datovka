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


DlgSendMessage::DlgSendMessage(MessageDb &db, Action action,
    QTreeView &accountList, QTableView &messageList,
    const AccountModel::SettingsMap &accountInfo,
    QWidget *parent,
    const QString &reSubject, const QString &senderId, const QString &sender,
    const QString &senderAddress)
    : QDialog(parent),
    m_accountList(accountList),
    m_messageList(messageList),
    m_action(action),
    m_accountInfo(accountInfo),
    m_reSubject(reSubject),
    m_senderId(senderId),
    m_sender(sender),
    m_senderAddress(senderAddress),
    m_userName(""),
    m_messDb(db)
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
 * Init dialog
 */
void DlgSendMessage::initNewMessageDialog(void)
/* ========================================================================= */
{
	QModelIndex index;
	const QStandardItem *item;

	this->recipientTableWidget->setColumnWidth(0,60);
	this->recipientTableWidget->setColumnWidth(1,140);
	this->recipientTableWidget->setColumnWidth(2,150);

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

	this->fromUser->setText("<strong>" + accountItemTop->text() +
	    "</strong>" + " (" + itemSettings[USER].toString() + ")");

	index = m_messageList.currentIndex();
	m_userName = itemSettings[USER].toString();

	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	if (ACT_REPLY == m_action) {
		this->subjectText->setText("Re: " + m_reSubject);

		int row = this->recipientTableWidget->rowCount();
		this->recipientTableWidget->insertRow(row);

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(m_senderId);
		this->recipientTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(m_sender);
		this->recipientTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(m_senderAddress);
		this->recipientTableWidget->setItem(row,2,item);
	}

	this->OptionalWidget->setHidden(true);
	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalForm(int)));

	connect(this->addRecipient, SIGNAL(clicked()), this,
	    SLOT(addRecipientData()));
	connect(this->removeRecipient, SIGNAL(clicked()), this,
	    SLOT(deleteRecipientData()));
	connect(this->findRecipient, SIGNAL(clicked()), this,
	    SLOT(findRecipientData()));

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
	    SLOT(tableItemInsRem()));
	connect(this->attachmentTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	this->recipientTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
	this->attachmentTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	connect(this->sendButton, SIGNAL(clicked()), this, SLOT(sendMessage()));
	connect(this->sendButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
}

/* ========================================================================= */
/*
 * Add file to attachment table widget
 */
void DlgSendMessage::addAttachmentFile(void)
/* ========================================================================= */
{
	QString attachFileName = QFileDialog::getOpenFileName(this,
	    tr("Add file"), "", tr("All files (*.*)"));

	if (!attachFileName.isNull()) {

		int size = 0;
		QString filename = "";
		QFile attFile(attachFileName);
		size = attFile.size();
		QFileInfo fileInfo(attFile.fileName());
		filename = fileInfo.fileName();
		QMimeDatabase db;
		QMimeType type = db.mimeTypeForFile(attachFileName);

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
		item->setText(attachFileName);
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
	this->removeRecipient->setEnabled(true);
}

/* ========================================================================= */
/*
 * Check all intputs when any item was changed
 */
void DlgSendMessage::tableItemInsRem(void)
/* ========================================================================= */
{
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
 * Add recipient from search dialog
 */
void DlgSendMessage::addRecipientData(void)
/* ========================================================================= */
{
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_ADDNEW,
	    this->recipientTableWidget, m_accountInfo, this, m_userName);
	dsSearch->show();
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
 * Check non-empty mandatory items in send message dialog
 */
void DlgSendMessage::checkInputFields(void)
/* ========================================================================= */
{
	bool buttonEnabled = !this->subjectText->text().isEmpty()
		    && (this->recipientTableWidget->rowCount() > 0)
		    && (this->attachmentTableWidget->rowCount() > 0);
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
 * Free document list
 */
void DlgSendMessage::findRecipientData(void)
/* ========================================================================= */
{
	QDialog *dlg_cont = new DlgContacts(m_messDb,
	    *(this->recipientTableWidget), this);
	dlg_cont->show();
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
	QDesktopServices::openUrl(QUrl("file://" + fileName));
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
 * Free message_copy list
 */
static
void isds_message_copy_free_void(void **message_copy)
/* ========================================================================= */
{
	isds_message_copy_free((struct isds_message_copy **) message_copy);
}


/* ========================================================================= */
/*
 * Send message/multiple message
 */
void DlgSendMessage::sendMessage(void)
/* ========================================================================= */
{
	if (!isdsSessions.isConnectToIsds(m_accountInfo.userName())) {
		isdsSessions.connectToIsds(m_accountInfo);
	}

	isds_error status;

	// init bool pointers
	_Bool *dmPersonalDelivery = NULL;
	_Bool *dmOVM = NULL;
	_Bool *dmPublishOwnID = NULL;
	_Bool *dmAllowSubstDelivery = NULL;

	// message and envleople structures
	struct isds_message *sent_message = NULL;
	struct isds_envelope *sent_envelope = NULL;

	// attachments
	struct isds_list *documents = NULL;
	struct isds_list *last = NULL;

	sent_envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));

	if (sent_envelope == NULL) {
		free(sent_envelope);
		goto fail;
	}
	memset(sent_envelope, 0, sizeof(struct isds_envelope));

	sent_message = (struct isds_message *)
	    malloc(sizeof(struct isds_message));

	if (sent_message == NULL) {
		free(sent_message);
		goto fail;
	}
	memset(sent_message, 0, sizeof(struct isds_message));

	// load attachments
	for (int i=0; i < this->attachmentTableWidget->rowCount(); i++) {

		struct isds_document *document = NULL;
		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		Q_ASSERT(0 != document);

		if (document == NULL) {
			goto fail;
		}
		memset(document, 0, sizeof(struct isds_document));

		// set document structure
		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;
		document->dmFileDescr = strdup(this->attachmentTableWidget->
		    item(i,0)->text().toStdString().c_str());
		// TODO - set dmFileMetaType based on isds_FileMetaType
		document->dmFileMetaType = FILEMETATYPE_MAIN;
		document->dmMimeType = strdup(this->attachmentTableWidget->
		    item(i,2)->text().toStdString().c_str());
		QString filePath =
		    this->attachmentTableWidget->item(i,4)->text();
		QFile file(filePath);

		if (file.exists()) {
			if (!file.open(QIODevice::ReadOnly))
			{
				qDebug("Couldn't open the file");
				return;
			}
			QByteArray bytes = file.readAll();
			document->data_length = bytes.size();
			document->data = malloc(bytes.size());
			memcpy(document->data, bytes.data(),
			    document->data_length);
		}

		// add document on the list of document
		struct isds_list *newListItem = (struct isds_list *) malloc(
		    sizeof(struct isds_list));
		Q_ASSERT(0 != newListItem);
		newListItem->data = (struct isds_document *)document;
		newListItem->next = NULL;
		newListItem->destructor = isds_document_free_void;
		if (last == NULL) {
			documents = last = newListItem;
		} else {
			last->next = newListItem;
			last = newListItem;
		}
	}

	// set mandatory fields of envelope
	sent_envelope->dmID = NULL; // must be NULL;
	sent_envelope->dmAnnotation =
	    strdup(this->subjectText->text().toStdString().c_str());
	sent_envelope->dbIDRecipient =
	    strdup(this->recipientTableWidget->item(0,0)->
	    text().toStdString().c_str());

	// set optional fields
	sent_envelope->dmLegalTitlePoint =
	    !this->dmLegalTitlePoint->text().isEmpty() ?
	    strdup(this->dmLegalTitlePoint->text().toStdString().c_str()):NULL;
	sent_envelope->dmLegalTitlePar =
	    !this->dmLegalTitlePar->text().isEmpty() ?
	    strdup(this->dmLegalTitlePar->text().toStdString().c_str()) :NULL;
	sent_envelope->dmLegalTitleSect =
	    !this->dmLegalTitleSect->text().isEmpty() ?
	    strdup(this->dmLegalTitleSect->text().toStdString().c_str()) :NULL;
	sent_envelope->dmSenderIdent = !this->dmSenderIdent->text().isEmpty() ?
	    strdup(this->dmSenderIdent->text().toStdString().c_str()) : NULL;
	sent_envelope->dmRecipientIdent =
	    !this->dmRecipientIdent->text().isEmpty() ?
	    strdup(this->dmRecipientIdent->text().toStdString().c_str()) :NULL;
	sent_envelope->dmSenderRefNumber =
	    !this->dmSenderRefNumber->text().isEmpty() ?
	    strdup(this->dmSenderRefNumber->text().toStdString().c_str()):NULL;
	sent_envelope->dmRecipientRefNumber =
	    !this->dmRecipientRefNumber->text().isEmpty() ?
	  strdup(this->dmRecipientRefNumber->text().toStdString().c_str()):NULL;
	sent_envelope->dmLegalTitleYear =
	    !this->dmLegalTitleYear->text().isEmpty() ?
	    (long int *)this->dmLegalTitleYear->text().toLong() : NULL;
	sent_envelope->dmLegalTitleLaw =
	    !this->dmLegalTitleLaw->text().isEmpty() ?
	    (long int *)this->dmLegalTitleLaw->text().toLong() : NULL;
	sent_envelope->dmToHands = !this->dmToHands->text().isEmpty() ?
	    strdup(this->dmToHands->text().toStdString().c_str()) : NULL;
	sent_envelope->dmType = NULL;

	// set bool pointers
	dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	*dmPersonalDelivery = this->dmPersonalDelivery->isChecked();
	sent_envelope->dmPersonalDelivery = dmPersonalDelivery;
	/* TODO - set dmAllowSubstDelivery from dialog */
	dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	*dmAllowSubstDelivery = true;
	sent_envelope->dmAllowSubstDelivery = dmAllowSubstDelivery;
	/* TODO - set dmOVM from dialog */
	dmOVM = (_Bool *) malloc(sizeof(_Bool));
	*dmOVM = true;
	sent_envelope->dmOVM = dmOVM;
	dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	*dmPublishOwnID = this->dmPublishOwnID->isChecked();
	sent_envelope->dmPublishOwnID  = dmPublishOwnID;

	// set envelop and attachments to message structure
	sent_message->envelope = sent_envelope;
	sent_message->documents = documents;

//************
/* Temporary alert for testing mode */
QMessageBox::StandardButton reply;
reply = QMessageBox::question(this,
    tr("Sent message to ISDS?"),
    tr("You are in the test mode of QDatovka now.") + "\n" +
    tr("Do you want to send message to ISDS?"),
    QMessageBox::Yes | QMessageBox::No);
if (reply == QMessageBox::Yes) {
//*********

	// only one recipient was chossen
	if (this->recipientTableWidget->rowCount() == 1) {
		status = isds_send_message(isdsSessions.session(m_userName),
		    sent_message);

		if (status == IE_SUCCESS) {
			QMessageBox::information(this,
			    tr("Message was sent"),
			    tr("Message was sent into ISDS successfully."),
			    QMessageBox::Ok);
		} else {
			QMessageBox::warning(this, tr("Error occurred"),
			    tr("An error occurred while message was sent")
			    + "\n" + tr("ErrorType: ") + isds_strerror(status),
			    QMessageBox::Ok);
		}

		isds_message_free(&sent_message);
		this->close();


	// message for multiple recipients
	} else {
		struct isds_list *copies = NULL;
		struct isds_list *last = NULL;

		for (int i=0; i < this->recipientTableWidget->rowCount(); i++) {

			struct isds_message_copy *message_copy = NULL;
			message_copy = (struct isds_message_copy *)
			malloc(sizeof(struct isds_message_copy));
			Q_ASSERT(0 != message_copy);

			if (message_copy == NULL) {
				goto fail;
			}
			memset(message_copy,0,sizeof(struct isds_message_copy));

			message_copy->dbIDRecipient =
			    strdup(this->recipientTableWidget->item(i,0)->
			    text().toStdString().c_str());
			message_copy->dmToHands =
			    !this->dmToHands->text().isEmpty() ?
			    strdup(this->dmToHands->
			    text().toStdString().c_str()) : NULL;

			// add message_copy to the list (copies)
			struct isds_list *newListItem = (struct isds_list *)
			    malloc(sizeof(struct isds_list));
			Q_ASSERT(0 != newListItem);
			newListItem->data = (struct isds_message_copy *)copies;
			newListItem->next = NULL;
			newListItem->destructor = isds_message_copy_free_void;
			if (last == NULL) {
				copies = last = newListItem;
			} else {
				last->next = newListItem;
				last = newListItem;
			}
		}
		status = isds_send_message_to_multiple_recipients(
		    isdsSessions.session(m_userName), sent_message, copies);


		if (status == IE_SUCCESS) {
			QMessageBox::information(this,
			    tr("Messages were sent"),
			    tr("Messages were sent into ISDS successfully."),
			    QMessageBox::Ok);
		} else {
			QMessageBox::warning(this, tr("Error occurred"),
			    tr("An error occurred while messages were sent")
			    + "\n" + tr("ErrorType: ") + isds_strerror(status),
			    QMessageBox::Ok);
		}

		isds_message_free(&sent_message);
		this->close();
	}

//***************
} else {
	this->close();
}
//***************
	return;

fail:
	/* TODO - free all structure */
	qDebug() << "An error was occurre";
	this->close();
}
