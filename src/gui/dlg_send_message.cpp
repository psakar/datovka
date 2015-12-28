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
#include "src/common.h"
#include "src/gui/dlg_contacts.h"
#include "src/gui/dlg_ds_search.h"
#include "src/models/accounts_model.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/message_db.h"
#include "src/log/log.h"
#include "src/thread/worker.h"
#include "src/views/table_home_end_filter.h"
#include "ui_dlg_send_message.h"


/*
 * Column indexes into recipient table widget.
 */
#define RTW_ID 0
#define RTW_NAME 1
#define RTW_ADDR 2
#define RTW_PDZ 3

/*
 * Column indexes into attachment table widget.
 */
#define ATW_FILE 0
#define ATW_TYPE 1
#define ATW_MIME 2
#define ATW_SIZE 3
#define ATW_PATH 4
#define ATW_DATA 5 /* Base64 encoded and hidden. */


DlgSendMessage::DlgSendMessage(MessageDbSet &dbSet, const QString &dbId,
    const QString &senderName, Action action, qint64 msgId,
    const QDateTime &deliveryTime,
    const QString &userName, const QString &dbType,
    bool dbEffectiveOVM, bool dbOpenAddressing,
    QString &lastAttAddPath, const QString &pdzCredit, QWidget *parent)
    : QDialog(parent),
    m_msgID(msgId),
    m_deliveryTime(deliveryTime),
    m_dbId(dbId),
    m_senderName(senderName),
    m_action(action),
    m_userName(userName),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_lastAttAddPath(lastAttAddPath),
    m_pdzCredit(pdzCredit),
    m_dbSet(dbSet),
    m_dmType(""),
    m_dmSenderRefNumber("")
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
	this->recipientTableWidget->setColumnWidth(0,70);
	this->recipientTableWidget->setColumnWidth(1,180);
	this->recipientTableWidget->setColumnWidth(2,240);

	this->attachmentTableWidget->setColumnWidth(0,150);
	this->attachmentTableWidget->setColumnWidth(1,40);
	this->attachmentTableWidget->setColumnWidth(2,120);

	this->attachmentTableWidget->setColumnHidden(5, true);

	this->replyLabel->hide();
	this->replyLabel->setEnabled(false);

	QString dbOpenAddressing = "";

	if (!m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			dbOpenAddressing =
			    " - " + tr("sending of PDZ: enabled") + "; " +
			    tr("remaining credit: ") + m_pdzCredit + " Kč";
		} else {
			dbOpenAddressing =
			    " - " + tr("sending of PDZ: disabled");
		}
	}

	Q_ASSERT(!m_userName.isEmpty());

	this->fromUser->setText("<strong>" +
	    AccountModel::globAccounts[m_userName].accountName() +
	    "</strong>" + " (" + m_userName + ") - " + m_dbType +
	    dbOpenAddressing);

	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	connect(this->payReply, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalFormAndSet(int)));

	this->OptionalWidget->setHidden(true);

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
	    SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	    this, SLOT(tableItemDoubleClicked(QTableWidgetItem *)));

	connect(this->attachmentTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem *)), this,
	    SLOT(attItemSelect()));

	connect(this->enterDbIdpushButton, SIGNAL(clicked()), this,
	    SLOT(addDbIdToRecipientList()));

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

	this->recipientTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
	this->attachmentTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));

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
 * Slot is fired when user double clicked on attachment item - open file
 */
void DlgSendMessage::tableItemDoubleClicked(QTableWidgetItem *item)
/* ========================================================================= */
{
	qDebug() << "tableItemDoubleClicked(" << item << ")";

	openAttachmentFile();
}


/* ========================================================================= */
/*
 * fill Send Message Dialog as reply
 */
void DlgSendMessage::fillDlgAsReply(void)
/* ========================================================================= */
{
	bool hideOptionalWidget = true;

	MessageDb *messageDb = m_dbSet.accessMessageDb(m_deliveryTime, false);
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
		pdz = getUserInfoFormIsds(envData.dbIDSender);
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
 * fill Send Message Dialog from template message
 */
void DlgSendMessage::fillDlgFromTmpMsg(void)
/* ========================================================================= */
{
	bool hideOptionalWidget = true;

	MessageDb *messageDb = m_dbSet.accessMessageDb(m_deliveryTime, false);
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
		pdz = getUserInfoFormIsds(envData.dbIDRecipient);
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
 * return dbEffectiveOVM for recipient
 */
QString DlgSendMessage::getUserInfoFormIsds(QString idDbox)
/* ========================================================================= */
{
	QString str = tr("no");
	struct isds_DbOwnerInfo *doi = NULL;
	struct isds_list *box = NULL;
	isds_DbType dbType = DBTYPE_FO;

	doi = isds_DbOwnerInfo_createConsume(idDbox, dbType, QString(),
	    NULL, QString(), NULL, NULL, QString(), QString(), QString(),
	    QString(), QString(), 0, false, false);
	if (NULL == doi) {
		return str;
	}

	isdsSearch(&box, m_userName, doi);
	isds_DbOwnerInfo_free(&doi);

	if (NULL != box) {
		const struct isds_DbOwnerInfo *item = (isds_DbOwnerInfo *)
		    box->data;
		Q_ASSERT(NULL != item);
		str = *item->dbEffectiveOVM ? tr("no") : tr("yes");
	}

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
	if (isdsSessions.isConnectedToIsds(m_userName)) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}


/* ========================================================================= */
/*
 * Return file content as Base64 string
 */
QByteArray DlgSendMessage::getFileBase64(const QString &filePath)
/* ========================================================================= */
 {
	QFile file(filePath);
	if (file.exists()) {
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Couldn't open the file" << filePath;
			goto fail;
		}
		return file.readAll().toBase64();
	}
fail:
	return QByteArray();
}


/* ========================================================================= */
/*
 * Add file to attachment table widget
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

		int fileSize = 0;
		QString filename = "";
		QFile attFile(fileName);
		fileSize = attFile.size();
		if (fileSize > MAX_ATTACHMENT_SIZE) {
			QMessageBox::warning(this, tr("Wrong file size"),
			    tr("File '%1' could not be added into attachment "
			    "because its size is bigger than 20MB.").
			    arg(fileName),
			    QMessageBox::Ok);
			continue;
		}
		m_attachSize += fileSize;
		QFileInfo fileInfo(attFile.fileName());
		filename = fileInfo.fileName();
		QMimeDatabase db;
		QMimeType type = db.mimeTypeForFile(attFile);

		int row = this->attachmentTableWidget->rowCount();
		bool isInTable = false;

		for (int j = 0; j < row; ++j) {
			if (this->attachmentTableWidget->item(j, ATW_FILE)->text() ==
			    filename) {
				isInTable = true;
				break;
			}
		}

		if (isInTable) {
			continue;
		}

		this->attachmentTableWidget->insertRow(row);

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(filename);
		this->attachmentTableWidget->setItem(row, ATW_FILE, item);
		item = new QTableWidgetItem;
		item->setText("");
		this->attachmentTableWidget->setItem(row, ATW_TYPE, item);
		item = new QTableWidgetItem;
		item->setText(type.name());
		this->attachmentTableWidget->setItem(row, ATW_MIME, item);
		item = new QTableWidgetItem;
		item->setText(QString::number(fileSize));
		this->attachmentTableWidget->setItem(row, ATW_SIZE, item);
		item = new QTableWidgetItem;
		item->setText(fileName);
		this->attachmentTableWidget->setItem(row, ATW_PATH, item);
		item = new QTableWidgetItem;
		item->setData(Qt::DisplayRole, getFileBase64(fileName));
		this->attachmentTableWidget->setItem(row, ATW_DATA, item);
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
 * Add recipient from local contact list
 */
void DlgSendMessage::addRecipientFromLocalContact(void)
/* ========================================================================= */
{
	QDialog *dlg_cont = new DlgContacts(m_dbSet, m_dbId,
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
 * Get attachment size when any item was removed from tablewidget
 */
int DlgSendMessage::cmptAttachmentSize(void)
/* ========================================================================= */
{
	int attachSize = 0;

	for (int i = 0; i < this->attachmentTableWidget->rowCount(); i++) {
		attachSize += this->attachmentTableWidget->item(i, ATW_SIZE)->text().
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
	QModelIndex dataIndex = selectedIndex.sibling(selectedIndex.row(), ATW_DATA);
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

	info += "\n\n" + tr("Your remaining credit is ") + m_pdzCredit + " Kč";

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(title);
	msgBox.setInformativeText(info);
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	return msgBox.exec();
}


/* ========================================================================= */
struct isds_list *DlgSendMessage::buildDocuments(void) const
/* ========================================================================= */
{
	struct isds_document *document = NULL; /* Attachment. */
	struct isds_list *documents = NULL; /* Attachment list (entry). */
	struct isds_list *last = NULL; /* No need to free it explicitly. */

	/* Load attachments. */
	for (int i = 0; i < this->attachmentTableWidget->rowCount(); ++i) {

		document = (struct isds_document *)
		    malloc(sizeof(struct isds_document));
		if (NULL == document) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		memset(document, 0, sizeof(struct isds_document));

		/* Set document content. */
		// TODO - document is binary document only -> is_xml = false;
		document->is_xml = false;
		document->dmFileDescr = strdup(this->attachmentTableWidget->
		    item(i, ATW_FILE)->text().toUtf8().constData());
		if (NULL == document->dmFileDescr) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}

		if (0 == i) {
			document->dmFileMetaType = FILEMETATYPE_MAIN;
		} else {
			document->dmFileMetaType = FILEMETATYPE_ENCLOSURE;
		}

		/* Since 2011 Mime Type can be empty and MIME type will
		 * be filled up on the ISDS server. It allows send files
		 * with special mime type without recognition by application.
		*/
		//document->dmMimeType = strdup(this->attachmentTableWidget->
		//    item(i, ATW_MIME)->text().toUtf8().constData());
		document->dmMimeType = strdup("");
		if (NULL == document->dmMimeType) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}

		QByteArray fileData(QByteArray::fromBase64(
		    this->attachmentTableWidget->item(i, ATW_DATA)->data(Qt::DisplayRole).toByteArray()));
		document->data_length = fileData.size();
		document->data = malloc(fileData.size());
		if (NULL == document->data) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		memcpy(document->data, fileData.data(), document->data_length);

		/* Add document on the list of document. */
		struct isds_list *newListItem = (struct isds_list *)
		    malloc(sizeof(struct isds_list));
		if (NULL == newListItem) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
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

	return documents;

fail:
	isds_document_free(&document);
	isds_list_free(&documents);
	return NULL;
}


/* ========================================================================= */
struct isds_envelope *DlgSendMessage::buildEnvelope(void) const
/* ========================================================================= */
{
	struct isds_envelope *envelope = NULL;
	QString dmType;

	envelope = (struct isds_envelope *)
	    malloc(sizeof(struct isds_envelope));
	if (envelope == NULL) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	memset(envelope, 0, sizeof(struct isds_envelope));

	/* Set mandatory fields of envelope. */
	envelope->dmID = NULL;
	envelope->dmAnnotation = strdup(
	    this->subjectText->text().toUtf8().constData());
	if (NULL == envelope->dmAnnotation) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}

	/* Set optional fields. */
	if (!this->dmSenderIdent->text().isEmpty()) {
		envelope->dmSenderIdent = strdup(
		    this->dmSenderIdent->text().toUtf8().constData());
		if (NULL == envelope->dmSenderIdent) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmRecipientIdent->text().isEmpty()) {
		envelope->dmRecipientIdent = strdup(
		    this->dmRecipientIdent->text().toUtf8().constData());
		if (NULL == envelope->dmRecipientIdent) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmSenderRefNumber->text().isEmpty()) {
		envelope->dmSenderRefNumber = strdup(
		    this->dmSenderRefNumber->text().toUtf8().constData());
		if (NULL == envelope->dmSenderRefNumber) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmRecipientRefNumber->text().isEmpty()) {
		envelope->dmRecipientRefNumber = strdup(
		    this->dmRecipientRefNumber->text().toUtf8().constData());
		if (NULL == envelope->dmRecipientRefNumber) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmLegalTitleLaw->text().isEmpty()) {
		envelope->dmLegalTitleLaw =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleLaw) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleLaw =
		    this->dmLegalTitleLaw->text().toLong();
	} else {
		envelope->dmLegalTitleLaw = NULL;
	}
	if (!this->dmLegalTitleYear->text().isEmpty()) {
		envelope->dmLegalTitleYear =
		    (long int *) malloc(sizeof(long int));
		if (NULL == envelope->dmLegalTitleYear) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
		*envelope->dmLegalTitleYear =
		    this->dmLegalTitleYear->text().toLong();
	} else {
		envelope->dmLegalTitleYear = NULL;
	}

	if (!this->dmLegalTitleSect->text().isEmpty()) {
		envelope->dmLegalTitleSect = strdup(
		    this->dmLegalTitleSect->text().toUtf8().constData());
		if (NULL == envelope->dmLegalTitleSect) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmLegalTitlePar->text().isEmpty()) {
		envelope->dmLegalTitlePar = strdup(
		    this->dmLegalTitlePar->text().toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePar) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	if (!this->dmLegalTitlePoint->text().isEmpty()) {
		envelope->dmLegalTitlePoint = strdup(
		    this->dmLegalTitlePoint->text().toUtf8().constData());
		if (NULL == envelope->dmLegalTitlePoint) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}
	envelope->dmPersonalDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPersonalDelivery) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmPersonalDelivery = this->dmPersonalDelivery->isChecked();

	/* only OVM can change */
	envelope->dmAllowSubstDelivery = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmAllowSubstDelivery) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	if (convertDbTypeToInt(m_dbType) > DBTYPE_OVM_REQ) {
		*envelope->dmAllowSubstDelivery = true;
	} else {
		*envelope->dmAllowSubstDelivery =
		    this->dmAllowSubstDelivery->isChecked();
	}

	if (m_dmType == "I") {
		if (this->payRecipient->isChecked()) {
			dmType = "O";
		} else {
			dmType = "K";
		}
		if (!m_dmSenderRefNumber.isEmpty()) {
			envelope->dmRecipientRefNumber = strdup(
			    m_dmSenderRefNumber.toUtf8().constData());
			if (NULL == envelope->dmRecipientRefNumber) {
				logErrorNL("%s", "Memory allocation failed.");
				goto fail;
			}
		}
	} else {
		if (this->payReply->isChecked()) {
			dmType = "I";
		}
	}

	if (!dmType.isEmpty()) {
		envelope->dmType = strdup(dmType.toUtf8().constData());
		if (NULL == envelope->dmType) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}

	envelope->dmOVM = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmOVM) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmOVM = m_dbEffectiveOVM;

	envelope->dmPublishOwnID = (_Bool *) malloc(sizeof(_Bool));
	if (NULL == envelope->dmPublishOwnID) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}
	*envelope->dmPublishOwnID = this->dmPublishOwnID->isChecked();

	return envelope;

fail:
	isds_envelope_free(&envelope);
	return NULL;
}


/* ========================================================================= */
DlgSendMessage::MsgSendingResult DlgSendMessage::sendSingleMessage(
    struct isds_message *message, int row) const
/* ========================================================================= */
{
	Q_ASSERT(NULL != message);
	Q_ASSERT(NULL != message->envelope);

	MsgSendingResult sendMsgResult;
	struct isds_envelope *envelope = message->envelope;
	struct isds_ctx *session = NULL;

	/* Clear fields. */
	if (NULL != envelope->dbIDRecipient) {
		free(envelope->dbIDRecipient);
		envelope->dbIDRecipient = NULL;
	}
	if (NULL != envelope->dmToHands) {
		free(envelope->dmToHands);
		envelope->dmToHands = NULL;
	}
	if (NULL != envelope->dmID) {
		free(envelope->dmID);
		envelope->dmID = NULL;
	}

	/* Set new recipient. */
	envelope->dbIDRecipient = strdup(
	    this->recipientTableWidget->item(row, RTW_ID)->text().toUtf8().constData());
	if (NULL == envelope->dbIDRecipient) {
		logErrorNL("%s", "Memory allocation failed.");
		goto fail;
	}

	if (!this->dmToHands->text().isEmpty()) {
		envelope->dmToHands =
		    strdup(this->dmToHands->text().toUtf8().constData());
		if (NULL == envelope->dmToHands) {
			logErrorNL("%s", "Memory allocation failed.");
			goto fail;
		}
	}

	session = isdsSessions.session(m_userName);
	if (NULL == session) {
		Q_ASSERT(0);
		logErrorNL("%s", "Missing ISDS session.");
		goto fail;
	}

	logInfo("Sending message from user '%s'.\n",
	    m_userName.toUtf8().constData());
	sendMsgResult.status = isds_send_message(session, message);

	sendMsgResult.dbID =
	    this->recipientTableWidget->item(row, RTW_ID)->text();
	sendMsgResult.recipientName =
	    this->recipientTableWidget->item(row, RTW_NAME)->text();
	{
		bool ok = false;
		sendMsgResult.dmId = QString(envelope->dmID).toLongLong(&ok);
		if (!ok) {
			sendMsgResult.dmId = -1;
		}
	}
	sendMsgResult.isPDZ =
	    this->recipientTableWidget->item(row, RTW_PDZ)->text() == tr("yes");
	sendMsgResult.errInfo = isdsLongMessage(session);

	return sendMsgResult;

fail:
	return MsgSendingResult();
}


/* ========================================================================= */
/*
 * Send message/multiple message
 */
void DlgSendMessage::sendMessage(void)
/* ========================================================================= */
{
	QString detailText;

	struct isds_message *message = NULL;

	/* List of send message result */
	QList<MsgSendingResult> sentMsgResultList;

	int successSendCnt = 0;
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

	message = (struct isds_message *) malloc(sizeof(struct isds_message));
	memset(message, 0, sizeof(struct isds_message));
	if (message == NULL) {
		logErrorNL("%s", "Memory allocation failed.");
		detailText = tr("An error has occurred during message creation."
		    " Memory allocation failed.");
		goto finish;
	}

	/* Attach envelope and attachment files to message structure. */
	message->documents = buildDocuments();
	if (NULL == message->documents) {
		detailText = tr("An error has occurred during loading "
		    "of attachments into message.");
		goto finish;
	}
	message->envelope = buildEnvelope();
	if (NULL == message->envelope) {
		detailText = tr("An error has occurred during "
		    "message envelope creation.");
		goto finish;
	}

	if (!isdsSessions.isConnectedToIsds(m_userName)) {
		detailText = tr("It was not possible to establish a "
		    "connection to the server or authorization failed.");
		goto finish;
	}

	/* Send message to all recipients. */
	for (int row = 0; row < this->recipientTableWidget->rowCount(); ++row) {
		MsgSendingResult sendingResult =
		    sendSingleMessage(message, row);

		sentMsgResultList.append(sendingResult);

		if (sendingResult.status == IE_SUCCESS) {
			QDateTime deliveryTime =
			    timevalToDateTime(message->envelope->dmDeliveryTime);

			MessageDb *messageDb = m_dbSet.accessMessageDb(deliveryTime, true);
			Q_ASSERT(0 != messageDb);

			/* TODO -- Move the function into worker. */
			messageDb->msgsInsertNewlySentMessageEnvelope(sendingResult.dmId,
			    m_dbId,
			    m_senderName,
			    this->recipientTableWidget->item(row, RTW_ID)->text(),
			    this->recipientTableWidget->item(row, RTW_NAME)->text(),
			    this->recipientTableWidget->item(row, RTW_ADDR)->text(),
			    this->subjectText->text());

			Worker::storeAttachments(*messageDb, sendingResult.dmId,
			    message->documents);

			successSendCnt++;
		}
	}

	foreach (const MsgSendingResult &result, sentMsgResultList) {
		if (result.status == IE_SUCCESS) {
			if (result.isPDZ) {
				detailText += tr("Message was successfully "
				    "sent to <i>%1 (%2)</i> as PDZ with number "
				    "<i>%3</i>.").
				    arg(result.recipientName).
				    arg(result.dbID).
				    arg(result.dmId) + "<br/>";
			} else {
				detailText += tr("Message was successfully "
				    "sent to <i>%1 (%2)</i> as message number "
				    "<i>%3</i>.").
				    arg(result.recipientName).
				    arg(result.dbID).
				    arg(result.dmId) + "<br/>";
			}
		} else {
			detailText += tr("Message was NOT successfully "
			    "sent to <i>%1 (%2)</i>. Server says: %3").
			    arg(result.recipientName).
			    arg(result.dbID).
			    arg(result.errInfo) + "<br/>";
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

		isds_message_free(&message);

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
			isds_message_free(&message);
			this->close(); /* Set return code to closed. */
			return;
		} else {
			isds_message_free(&message);
			return;
		}
	}

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
	isds_message_free(&message);
	this->close();
}


/* ========================================================================= */
/*
 * Enter DB ID manually
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
