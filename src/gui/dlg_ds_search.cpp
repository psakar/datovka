

#include "dlg_ds_search.h"
#include "src/io/isds_sessions.h"

DlgDsSearch::DlgDsSearch(Action action, QTableWidget *recipientTableWidget,
    QWidget *parent, QString userName)
    : QDialog(parent),
    m_recipientTableWidget(recipientTableWidget),
    m_action(action),
    m_userName(userName)

{
	setupUi(this);
	initSearchWindow();
}

void DlgDsSearch::initSearchWindow(void)
{
	this->dataBoxTypeCBox->addItem(tr("OMV - Orgán věřejné moci"));
	this->dataBoxTypeCBox->addItem(tr("PO - Právnická osoba"));
	this->dataBoxTypeCBox->addItem(
	    tr("PFO - Podnikající fyzická osoba"));
	this->dataBoxTypeCBox->addItem(tr("FO - Fyzická osoba"));

	this->resultsTableWidget->setColumnWidth(0,20);
	this->resultsTableWidget->setColumnWidth(1,60);
	this->resultsTableWidget->setColumnWidth(2,120);
	this->resultsTableWidget->setColumnWidth(3,100);

	connect(this->iDLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->iCLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->nameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->pscLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->dataBoxTypeCBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(checkInputFields()));
	connect(this->resultsTableWidget,SIGNAL(itemClicked(QTableWidgetItem*)),
	    this, SLOT(enableOkButton()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(insertDsItems()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchDataBox()));

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

}

void DlgDsSearch::checkInputFields(void)
{
	if (this->dataBoxTypeCBox->currentIndex() == 3) {
		this->iCLineEdit->setEnabled(false);
	} else {
		this->iCLineEdit->setEnabled(true);
	}

	if (!this->iDLineEdit->text().isEmpty()) {
		this->nameLineEdit->setEnabled(false);
		this->pscLineEdit->setEnabled(false);

		if (this->iDLineEdit->text().length() == 6) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->iCLineEdit->text().isEmpty()) {
		this->iDLineEdit->setEnabled(false);
		this->nameLineEdit->setEnabled(false);
		this->pscLineEdit->setEnabled(false);

		if (this->iCLineEdit->text().length() == 8) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->nameLineEdit->text().isEmpty()) {
		this->iDLineEdit->setEnabled(false);

		if (this->nameLineEdit->text().length() > 2) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else {
		this->searchPushButton->setEnabled(false);
		this->iDLineEdit->setEnabled(true);
		this->nameLineEdit->setEnabled(true);
		this->pscLineEdit->setEnabled(true);
	}
}


void DlgDsSearch::searchDataBox(void)
{
	/* TODO - apply FindDataBox funrtion from libisds */
	QList<QVector<QString>> list_contacts;
	QVector<QString> contact;
	contact.append("xxxx");
	contact.append(this->nameLineEdit->text());
	contact.append("uiopp");
	contact.append("62122");
	list_contacts.append(contact);
	addContactsToTable(list_contacts);
}


void DlgDsSearch::enableOkButton(void)
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->resultsTableWidget->rowCount(); i++) {
		if (this->resultsTableWidget->item(i,0)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


void DlgDsSearch::addContactsToTable(
    const QList< QVector<QString> > &contactList)
{
	this->resultsTableWidget->setRowCount(0);

	this->resultsTableWidget->setEnabled(true);
	for (int i = 0; i < contactList.count(); i++) {

		int row = this->resultsTableWidget->rowCount();
		this->resultsTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->resultsTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(0));
		this->resultsTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(1));
		this->resultsTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(2));
		this->resultsTableWidget->setItem(row,3,item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(3));
		this->resultsTableWidget->setItem(row,4,item);
	}
}


bool DlgDsSearch::isInRecipientTable(const QString &idDs) const
{
	Q_ASSERT(0 != m_recipientTableWidget);

	for (int i = 0; i < this->m_recipientTableWidget->rowCount(); i++) {
		if (this->m_recipientTableWidget->item(i,0)->text() == idDs) {
			return true;
		}
	}
	return false;
}


void DlgDsSearch::insertDsItems(void)
{
	if (ACT_ADDNEW == m_action) {
		Q_ASSERT(0 != m_recipientTableWidget);
		for (int i = 0; i < this->resultsTableWidget->rowCount(); i++) {
			if ((!this->resultsTableWidget->item(
			          i,0)->checkState()) ||
			    isInRecipientTable(this->resultsTableWidget->item(
			        i,1)->text())) {
				continue;
			}
			/* For all checked non-duplicated. */
			int row = m_recipientTableWidget->rowCount();
			m_recipientTableWidget->insertRow(row);
			QTableWidgetItem *item = new QTableWidgetItem;
			item->setText(this->resultsTableWidget->
			    item(i,1)->text());
			this->m_recipientTableWidget->setItem(row,0,item);
			item = new QTableWidgetItem;
			item->setText(this->resultsTableWidget->
			    item(i,2)->text());
			this->m_recipientTableWidget->setItem(row,1,item);
			item = new QTableWidgetItem;
			item->setText(this->resultsTableWidget->\
			    item(i,3)->text());
			this->m_recipientTableWidget->setItem(row,2,item);
		}
	}
}
