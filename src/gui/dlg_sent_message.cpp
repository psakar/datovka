#include "dlg_sent_message.h"
#include "ui_dlg_sent_message.h"

dlg_sent_message::dlg_sent_message(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
	initNewMessageDialog();
}

void dlg_sent_message::on_cancelButton_clicked()
{
	this->close();
}

void dlg_sent_message::initNewMessageDialog(void) {

	this->OptionalWidget->setHidden(true);
	connect(this->optionalFieldCheckBox, SIGNAL(stateChanged(int)), this,
	    SLOT(showOptionalForm(int)));

}

void dlg_sent_message::showOptionalForm(int state)
{
	this->OptionalWidget->setHidden(Qt::Unchecked == state);
}


void dlg_sent_message::sendMessage(void) {


}
