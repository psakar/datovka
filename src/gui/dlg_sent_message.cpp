#include "src/models/accounts_model.h"
#include "dlg_sent_message.h"
#include "ui_dlg_sent_message.h"


dlg_sent_message::dlg_sent_message(QWidget *parent, QTreeView *accountList) :
    QDialog(parent),
    m_accountList(accountList)
{
	setupUi(this);
	initNewMessageDialog();
}

void dlg_sent_message::on_cancelButton_clicked()
{
	this->close();
}

void dlg_sent_message::initNewMessageDialog(void) {

	AccountModel *model = dynamic_cast<AccountModel*>(m_accountList->model());
	QModelIndex index = m_accountList->currentIndex();
	const QStandardItem *item = model->itemFromIndex(index);
	const QStandardItem *itemTop = AccountModel::itemTop(item);
	const AccountModel::SettingsMap &itemSettings =
	    itemTop->data(ROLE_CONF_SETINGS).toMap();

	this->fromUser->setText("<strong>"+ itemTop->text() + "</strong>"
	    + " (" + itemSettings[USER].toString() + ")");

	this->OptionalWidget->setHidden(true);
	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalForm(int)));

	connect(this->addRecipient, SIGNAL(clicked()), this,
	    SLOT(addRecipientData()));
	connect(this->addAttachment, SIGNAL(clicked()), this,
	    SLOT(addAttachmentFile()));

	//this->recipientTableView->setHorizontalHeader(0, new QStandardItem(tr("Accounts")));


}

void dlg_sent_message::addRecipientData(void) {

}

void dlg_sent_message::addAttachmentFile(void) {

	QString attachFileName = QFileDialog::getOpenFileName(this,
	    tr("Add file"), "", tr("All files (*.*)"));
	if (attachFileName != NULL) {

	} else {

	}
}


void dlg_sent_message::showOptionalForm(int state)
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


void dlg_sent_message::sendMessage(void) {


}
