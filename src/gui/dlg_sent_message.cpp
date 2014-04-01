#include "dlg_sent_message.h"
#include "ui_dlg_sent_message.h"

dlg_sent_message::dlg_sent_message(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
}

void dlg_sent_message::on_CancelButton_clicked()
{
	this->close();
}
