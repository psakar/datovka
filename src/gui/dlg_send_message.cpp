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

	QString dbOpenAddressing = "";

	if (!m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			dbOpenAddressing =
			    " - " + tr("commercial messages are enabled");
		} else {
			dbOpenAddressing =
			    " - " + tr("commercial messages are disabled");
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
	this->removeRecipient->setEnabled(true);
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
 * Add recipient from search dialog
 */
void DlgSendMessage::addRecipientData(void)
/* ========================================================================= */
{
	QDialog *dsSearch = new DlgDsSearch(DlgDsSearch::ACT_ADDNEW,
	    this->recipientTableWidget, m_dbType, m_dbEffectiveOVM,
	    m_dbOpenAddressing, this, m_userName);
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
 * Get attachment size when anz item was removed from tablewidget
 */
int DlgSendMessage::cmptAttachmentSize(void)
/* ========================================================================= */
{
	int attachSize = 0;

	for (int i = 0; i < this->attachmentTableWidget->rowCount(); i++) {
		attachSize += this->attachmentTableWidget->item(i,3)->text().toInt();
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
 * Find recipent in contact list
 */
void DlgSendMessage::findRecipientData(void)
/* ========================================================================= */
{
	QDialog *dlg_cont = new DlgContacts(m_messDb, m_dbId,
	    *(this->recipientTableWidget), m_dbType, m_dbEffectiveOVM,
	    m_dbOpenAddressing, this, m_userName);
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
 * Dialog informs user that message contains one or more PDZs.
 */
int DlgSendMessage::showInfoAboutPDZ(int pdzCnt)
/* ========================================================================= */
{
	QString title;
	QString info;

	if (pdzCnt > 1) {
		title = tr("Message contains Commercial messages (PDZ)");
		info = tr("Your message contains %1 non-OVM recipients "
		    "therefore these messages will be sent as a"
		    "Commercial messages (PDZ)").arg(pdzCnt);
		info += "\n\n";
		info += tr("Do you want to send all messages?");
	} else {
		title = tr("Message contains Commercial message (PDZ)");
		info = tr("Your message contains %1 non-OVM recipient "
		    "therefore this message will be sent as "
		    "Commercial message (PDZ)").arg(pdzCnt);
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

	int pdzCnt = 0;

	for (int i = 0; i < this->recipientTableWidget->rowCount(); i++) {
		if (this->recipientTableWidget->item(i,3)->text() ==tr("yes")) {
			pdzCnt++;
		}
	}

	if (pdzCnt > 0) {
		if (QMessageBox::No == showInfoAboutPDZ(pdzCnt)) {
			return;
		}
	}

	// init bool pointers
	_Bool *dmPersonalDelivery = NULL;
	_Bool *dmOVM = NULL;
	_Bool *dmPublishOwnID = NULL;
	_Bool *dmAllowSubstDelivery = NULL;
	QString dmType;

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
		goto finish;
	}
	memset(sent_envelope, 0, sizeof(struct isds_envelope));

	sent_message = (struct isds_message *)
	    malloc(sizeof(struct isds_message));

	if (sent_message == NULL) {
		free(sent_message);
		goto finish;
	}
	memset(sent_message, 0, sizeof(struct isds_message));

	// load attachments
	for (int i=0; i < this->attachmentTableWidget->rowCount(); i++) {

		struct isds_document *document = NULL;
		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		Q_ASSERT(0 != document);

		if (document == NULL) {
			goto finish;
		}
		memset(document, 0, sizeof(struct isds_document));

		// set document structure
		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;
		document->dmFileDescr = strdup(this->attachmentTableWidget->
		    item(i,0)->text().toStdString().c_str());

		// set document structure
		 // TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;

		document->dmFileDescr = strdup(this->attachmentTableWidget->
		    item(i,0)->text().toStdString().c_str());

		if (i == 0) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

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
	sent_envelope->dmID = NULL;
	sent_envelope->dmAnnotation =
	    strdup(this->subjectText->text().toStdString().c_str());
	sent_envelope->dbIDRecipient =
	    strdup(this->recipientTableWidget->item(0,0)->
	    text().toStdString().c_str());

	// set optional fields
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
	sent_envelope->dmToHands = !this->dmToHands->text().isEmpty() ?
	    strdup(this->dmToHands->text().toStdString().c_str()) : NULL;

	if (!this->dmLegalTitleLaw->text().isEmpty()) {
		long int *number = NULL;
		number = (long int *) malloc(sizeof(*number));
		*number = this->dmLegalTitleLaw->text().toLong();
		sent_envelope->dmLegalTitleLaw = number;
	} else {
		sent_envelope->dmLegalTitleLaw = NULL;
	}
	if (!this->dmLegalTitleYear->text().isEmpty()) {
		long int *number = NULL;
		number = (long int *) malloc(sizeof(*number));
		*number = this->dmLegalTitleYear->text().toLong();
		sent_envelope->dmLegalTitleYear = number;
	} else {
		sent_envelope->dmLegalTitleYear = NULL;
	}

	sent_envelope->dmLegalTitleSect =
	    !this->dmLegalTitleSect->text().isEmpty() ?
	    strdup(this->dmLegalTitleSect->text().toStdString().c_str()) :NULL;
	sent_envelope->dmLegalTitlePar =
	    !this->dmLegalTitlePar->text().isEmpty() ?
	    strdup(this->dmLegalTitlePar->text().toStdString().c_str()) :NULL;
	sent_envelope->dmLegalTitlePoint =
	    !this->dmLegalTitlePoint->text().isEmpty() ?
	    strdup(this->dmLegalTitlePoint->text().toStdString().c_str()):NULL;


	dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	*dmPersonalDelivery = this->dmPersonalDelivery->isChecked();
	sent_envelope->dmPersonalDelivery = dmPersonalDelivery;

	/* only OVM can changes */
	dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		*dmAllowSubstDelivery = true;
	} else {
		*dmAllowSubstDelivery = this->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == "I") {
		if (this->payRecipient->isChecked()) {
			dmType = "O";
		} else {
			dmType = "K";
		}
		sent_envelope->dmRecipientRefNumber =
		    !m_dmSenderRefNumber.isEmpty() ?
		    strdup(m_dmSenderRefNumber.toStdString().c_str()):NULL;
	} else {
		if (this->payReply->isChecked()) {
			dmType = "I";
		}
	}

	sent_envelope->dmType = !dmType.isNull() ?
	    strdup(dmType.toStdString().c_str()) : NULL;

	sent_envelope->dmAllowSubstDelivery = dmAllowSubstDelivery;
	dmOVM = (_Bool *) malloc(sizeof(_Bool));
	*dmOVM = m_dbEffectiveOVM;
	sent_envelope->dmOVM = dmOVM;
	dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	*dmPublishOwnID = this->dmPublishOwnID->isChecked();
	sent_envelope->dmPublishOwnID  = dmPublishOwnID;

	// set envelop and attachments to message structure
	sent_message->envelope = sent_envelope;
	sent_message->documents = documents;

	if (!isdsSessions.isConnectToIsds(m_accountInfo.userName())) {
		goto finish;
	}

	/* Sent message to 1 recipient */
	if (this->recipientTableWidget->rowCount() == 1) {

		qDebug() << "sending message form user name" << m_userName;
		status = isds_send_message(isdsSessions.session(m_userName),
		    sent_message);

	/* Sent message for multiple recipients */
	} else {
		struct isds_list *copies = NULL;
		struct isds_list *last = NULL;

		int i = 0;
		for (i = 0; i < this->recipientTableWidget->rowCount(); i++) {

			struct isds_message_copy *message_copy = NULL;
			message_copy = (struct isds_message_copy *)
			malloc(sizeof(struct isds_message_copy));
			Q_ASSERT(0 != message_copy);

			if (message_copy == NULL) {
				goto finish;
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

		qDebug() << "sending" << i << "messages from user name"
		    << m_userName;
		status = isds_send_message_to_multiple_recipients(
		    isdsSessions.session(m_userName), sent_message, copies);
	}

finish:
	if (status == IE_SUCCESS) {
		QMessageBox::information(this,
		    tr("Message was sent"),
		    tr("Messages was sent into ISDS successfully."),
		    QMessageBox::Ok);

		isds_message_free(&sent_message);
		this->close();
		return;
	} else {
		if (showErrorMessageBox((int)status) == QMessageBox::Yes) {
			isds_message_free(&sent_message);
			this->close();
			return;
		};
	}

	isds_message_free(&sent_message);
}


/* ========================================================================= */
/*
* This is call if an error of send message procedure was obtained
*/
int DlgSendMessage::showErrorMessageBox(int status)
/* ========================================================================= */
{
	QString msgBoxTitle = "";
	QString msgBoxContent = "";

	qDebug() << status << isds_strerror((isds_error)status);

	switch(status) {
	case IE_PARTIAL_SUCCESS:
		msgBoxTitle = tr("Send multiple message problem!");
		msgBoxContent =
		    tr("It was not possible to send message to all recipients.")
		    + "<br/><br/>" +
		    "<b>" + tr("Send multiple message finished with an problem!")
		    + "</b>" + "<br/><br/>" +
		    tr("It can be caused sending of the message to the "
		    "unactivated databox or some of recipient does not exist.")
		    + "<br/>" +
		    tr("Download list of sent messages and check which "
		    "recipients didn't receive your message.");
		QMessageBox::critical(this, msgBoxTitle, msgBoxContent,
		    QMessageBox::Ok);
		return QMessageBox::Yes;
		break;
	case IE_NOT_LOGGED_IN:
		msgBoxTitle = tr("Send message error!");
		    tr("It was not possible to send message to server "
		    "Datové schránky.") + "<br/><br/>" +
		    "<b>" + tr("Authorization failed!") + "</b>" + "<br/><br/>"
		     + tr("Please check your credentials including the test-"
		        "environment setting.") + "<br/>" +
		    tr("It is possible that your password has expired - "
		        "in this case, you need to use the official web "
		        "interface of Datové schránky to change it.");
		break;
	case IE_TIMED_OUT:
	case IE_CONNECTION_CLOSED:
		msgBoxTitle = tr("Send message error!");
		msgBoxContent =
		    tr("It was not possible to send message to server "
		    "Datové schránky.") + "<br/><br/>" +
		    "<b>" + tr("Connection to the server timed out!")
		    + "</b>" + "<br/><br/>" +
		    tr("It was not possible to establish a connection "
		    "within a set time.") + "<br/>" +
		    tr("Please check your internet connection and try again.");
		break;
	case IE_ISDS:
		msgBoxTitle = tr("Send message error!");
		msgBoxContent =
		    tr("Sent message was refused by server Datové schránky.")
		    + "<br/><br/>" +
		    "<b>" + tr("Problem with attachment of message!")
		    + "</b>" + "<br/><br/>" +
		    tr("Server did not accept message for this databox "
		    "and returned message back.") + " " +
		    tr("It can be caused by a wrong/unsupported MIME type of "
		    "some file in the attachment or if an attachment "
		    "contains an archive.");
		break;
/*
 *	TODO - add another dialogs for this reasults - libisds internal errors
	case IE_INVAL:
	case IE_ENUM:
	case IE_NOMEM:
	case IE_INVALID_CONTEXT:
	case IE_NOTSUP:
	case IE_HTTP:
	case IE_ERROR:
*/
	default:
		msgBoxTitle = tr("Send message error!");
		msgBoxContent =
		    tr("It was not possible to send message to the "
		    "server Datové schránky.") + "<br><br>" +
		    "<b>" + tr("Error: ") + isds_strerror((isds_error)status) +
		    + "</b>" + "<br><br>" +
		    tr("It was not possible create send message request "
		    "because an internal error has occurred.");
		break;
	}

	msgBoxContent += "<br/><br/><b>" +
	    tr("Do you want to abort sending of the message and "
	    "close the send message dialog?") + "</b>";

	return QMessageBox::critical(this, msgBoxTitle, msgBoxContent,
		QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
}
