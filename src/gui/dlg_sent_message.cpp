#include <QMimeDatabase>
#include "src/models/accounts_model.h"
#include "dlg_sent_message.h"
#include "ui_dlg_sent_message.h"


dlg_sent_message::dlg_sent_message(QWidget *parent, QTreeView *accountList,
    QTableView *messageList, QString action) :
    QDialog(parent),
    m_accountList(accountList),
    m_messageList(messageList),
    m_action(action)
{
	setupUi(this);
	initNewMessageDialog();
}

void dlg_sent_message::on_cancelButton_clicked()
{
	this->close();
}

void dlg_sent_message::initNewMessageDialog(void)
{
	QModelIndex index;
	const QStandardItem *item;

	this->recipientTableWidget->setColumnWidth(0,50);
	this->recipientTableWidget->setColumnWidth(1,130);
	this->recipientTableWidget->setColumnWidth(2,130);

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
	Q_ASSERT(index.isValid()); /* TODO -- Deal with invalid. */

	QAbstractItemModel *messageModel = m_messageList->model();
	index = messageModel->index(index.row(), 0); /* First column. */
	const QMap<int, QVariant> messageItemData =
	    messageModel->itemData(index);
	/* TODO -- A more robust mapping to message id. */
	qDebug() << "ID " << messageItemData.first().toString();

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
	    SLOT(recItemSelect(QTableWidgetItem *)));

	connect(this->attachmentTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem *)), this,
	    SLOT(attItemSelect(QTableWidgetItem *)));

	this->attachmentTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	if (m_action == "Replay") {
		this->subjectText->setText("TODO");
	}
}

void dlg_sent_message::addAttachmentFile(void)
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

void dlg_sent_message::attItemSelect(QTableWidgetItem *item)
{
	this->removeRecipient->setEnabled(true);
}


void dlg_sent_message::recItemSelect(QTableWidgetItem *item)
{
	this->removeAttachment->setEnabled(true);
	this->openAttachment->setEnabled(true);
}


void dlg_sent_message::showOptionalForm(int state)
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


void dlg_sent_message::addRecipientData(void)
{

	/* TODO */
}



void dlg_sent_message::deleteAttachmentFile()
{
	int row = this->attachmentTableWidget->currentRow();
	if (row >= 0) {
		this->attachmentTableWidget->removeRow(row);
		this->removeAttachment->setEnabled(false);
		this->openAttachment->setEnabled(false);
	}
}


void dlg_sent_message::deleteRecipientData()
{
	int row = this->recipientTableWidget->currentRow();
	if (row >= 0) {
		this->recipientTableWidget->removeRow(row);
		this->removeRecipient->setEnabled(false);
	}
}

void dlg_sent_message::findRecipientData(void)
{
	/* TODO */
}


void dlg_sent_message::openAttachmentFile(void)
{
	/* TODO */
}


void dlg_sent_message::sendMessage(void)
{
	/* TODO */
}
