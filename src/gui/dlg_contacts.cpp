#include "dlg_contacts.h"

dlg_contacts::dlg_contacts(QWidget *parent) : QDialog(parent)
{
	setupUi(this);

	this->contactTableWidget->setColumnWidth(0,20);
	this->contactTableWidget->setColumnWidth(1,60);
	this->contactTableWidget->setColumnWidth(2,150);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	fillContactFromMessage();

	connect(this->filterLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(findContact(QString)));
	connect(this->clearPushButton, SIGNAL(clicked()), this,
	    SLOT(clearContactText()));

	this->contactTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

}


void dlg_contacts::findContact(QString text)
{
	qDebug() << text;
	if (this->filterLineEdit->text().length() > 2) {
	/* TODO */
	}
}

void dlg_contacts::clearContactText(void)
{
	this->filterLineEdit->clear();
}

void dlg_contacts::fillContactFromMessage()
{
	for (int i = 0; i < 3; i++) {
		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		//item->setFlags(Qt::ItemIsUserCheckable);
		this->contactTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText("sa65fs");
		this->contactTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText("Matesnice");
		this->contactTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText("Brno 61200, CZ");
		this->contactTableWidget->setItem(row,3,item);
	}
}

