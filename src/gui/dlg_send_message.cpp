#include <QMimeDatabase>
#include "dlg_send_message.h"
#include "dlg_ds_search.h"
#include "dlg_contacts.h"
#include "src/models/accounts_model.h"
#include "ui_dlg_send_message.h"
#include "src/io/message_db.h"

DlgSendMessage::DlgSendMessage(MessageDb &db, QWidget *parent,
    QTreeView *accountList, QTableView *messageList,
    const QString &action, const QString &reSubject,
    const QString &senderId, const QString &sender,
    const QString &senderAddress) :
    QDialog(parent),
    m_accountList(accountList),
    m_messageList(messageList),
    m_action(action),
    reSubject(reSubject),
    senderId(senderId),
    sender(sender),
    senderAddress(senderAddress),
    m_messDb(db)
{
	setupUi(this);
	initNewMessageDialog();
}

void DlgSendMessage::on_cancelButton_clicked(void)
{
	this->close();
}

void DlgSendMessage::initNewMessageDialog(void)
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
	    m_accountList->model());
	index = m_accountList->currentIndex();
	Q_ASSERT(index.isValid()); /* TODO -- Deal with invalid. */

	item = accountModel->itemFromIndex(index);
	Q_ASSERT(0 != item);
	const QStandardItem *accountItemTop = AccountModel::itemTop(item);
	const AccountModel::SettingsMap &itemSettings =
	    accountItemTop->data(ROLE_CONF_SETINGS).toMap();

	this->fromUser->setText("<strong>" + accountItemTop->text() +
	    "</strong>" + " (" + itemSettings[USER].toString() + ")");

	index = m_messageList->currentIndex();
	//Q_ASSERT(index.isValid()); /* TODO -- Deal with invalid. */
/*
	QAbstractItemModel *messageModel = m_messageList->model();
	index = messageModel->index(index.row(), 0);
	const QMap<int, QVariant> messageItemData =
	    messageModel->itemData(index);

	qDebug() << "ID " << messageItemData.first().toString();
*/
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsInserted(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));
	connect(this->recipientTableWidget->model(),
	    SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
	    SLOT(tableItemInsRem()));

	if (m_action == "Reply") {
		this->subjectText->setText("Re: " + reSubject);

		int row = this->recipientTableWidget->rowCount();
		this->recipientTableWidget->insertRow(row);

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(senderId);
		this->recipientTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(sender);
		this->recipientTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(senderAddress);
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


}

void DlgSendMessage::addAttachmentFile(void)
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
	}
}

void DlgSendMessage::attItemSelect(void)
{

	this->removeAttachment->setEnabled(true);
	this->openAttachment->setEnabled(true);
}


void DlgSendMessage::recItemSelect(void)
{
	this->removeRecipient->setEnabled(true);
}


void DlgSendMessage::tableItemInsRem(void)
{
	checkInputFields();
}

void DlgSendMessage::showOptionalForm(int state)
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


void DlgSendMessage::addRecipientData(void)
{
	QDialog *dlg_ds_search = new dlg_ds_search_dialog(this,
	    this->recipientTableWidget, "Add");
	dlg_ds_search->show();

}


void DlgSendMessage::deleteAttachmentFile(void)
{
	int row = this->attachmentTableWidget->currentRow();
	if (row >= 0) {
		this->attachmentTableWidget->removeRow(row);
		this->removeAttachment->setEnabled(false);
		this->openAttachment->setEnabled(false);
	}
}


void DlgSendMessage::checkInputFields(void)
{
	bool buttonEnabled = !this->subjectText->text().isEmpty()
		    && (this->recipientTableWidget->rowCount() > 0)
		    && (this->attachmentTableWidget->rowCount() > 0);
	this->sendButton->setEnabled(buttonEnabled);
}


void DlgSendMessage::deleteRecipientData(void)
{
	int row = this->recipientTableWidget->currentRow();
	if (row >= 0) {
		this->recipientTableWidget->removeRow(row);
		this->removeRecipient->setEnabled(false);
	}
}

void DlgSendMessage::findRecipientData(void)
{
	QDialog *dlg_cont = new dlg_contacts(this,
	    this->recipientTableWidget, &m_messDb);
	dlg_cont->show();
}


void DlgSendMessage::openAttachmentFile(void)
{
	/* TODO */
}


void DlgSendMessage::sendMessage(void)
{
	/* TODO */
}
