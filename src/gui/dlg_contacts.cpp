#include "dlg_contacts.h"

dlg_contacts::dlg_contacts(QWidget *parent,
    QTableWidget *recipientTableWidget, MessageDb *db):
       QDialog(parent),
       m_recipientTableWidget(recipientTableWidget),
       m_messdb(db)
{
	setupUi(this);

	this->contactTableWidget->setColumnWidth(0,20);
	this->contactTableWidget->setColumnWidth(1,60);
	this->contactTableWidget->setColumnWidth(2,150);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->clearPushButton->setEnabled(false);

	fillContactsFromMessageDb();

	connect(this->filterLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));
	connect(this->contactTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->clearPushButton, SIGNAL(clicked()), this,
	    SLOT(clearContactText()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(insertDsItems()));

	this->contactTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
}


void dlg_contacts::filterContact(QString text)
{
	this->clearPushButton->setEnabled(true);
	if (!text.isEmpty()) {
		for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
			contactTableWidget->hideRow(i);
		}
		QList<QTableWidgetItem *> items =
		    contactTableWidget->findItems(text, Qt::MatchContains);
		for (int i = 0; i < items.count(); i++) {
				contactTableWidget->showRow(items.at(i)->row());
		}
	} else {
		this->clearPushButton->setEnabled(false);
		for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
			contactTableWidget->showRow(i);
		}
	}
}

void dlg_contacts::clearContactText(void)
{
	this->filterLineEdit->clear();
	this->clearPushButton->setEnabled(false);
}

void dlg_contacts::fillContactsFromMessageDb()
{
	QList<QVector<QString>> contactList;
	contactList = m_messdb->selectContactsFromMessageDb();

	for (int i = 0; i < contactList.count(); i++) {

		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->contactTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].takeAt(0));
		this->contactTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].takeAt(0));
		this->contactTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].takeAt(0));
		this->contactTableWidget->setItem(row,3,item);
	}
}

void dlg_contacts::enableOkButton(void)
{

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


bool dlg_contacts::isInRecipientTable(QString idDs)
{
	for (int i = 0; i < this->m_recipientTableWidget->rowCount(); i++) {
		if (this->m_recipientTableWidget->item(i,0)->text() == idDs)
		return true;
	}
	return false;
}



void dlg_contacts::insertDsItems(void)
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			if (!isInRecipientTable(
			    this->contactTableWidget->item(i,1)->text())) {
				int row = m_recipientTableWidget->rowCount();
				m_recipientTableWidget->insertRow(row);
				QTableWidgetItem *item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->
				    item(i,1)->text());
				this->m_recipientTableWidget->setItem(row,0,item);
				item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->
				    item(i,2)->text());
				this->m_recipientTableWidget->setItem(row,1,item);
				item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->\
				    item(i,3)->text());
				this->m_recipientTableWidget->setItem(row,2,item);
			}
		}
	}
}
