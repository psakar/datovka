#include "dlg_ds_search.h"

dlg_ds_search_dialog::dlg_ds_search_dialog(QWidget *parent) : QDialog(parent)
{
	setupUi(this);
	initSearchWindow();
}


void dlg_ds_search_dialog::initSearchWindow(void)
{
	this->dataBoxTypeCBox->addItem(QString(tr("OMV - Orgán věřejné moci")));
	this->dataBoxTypeCBox->addItem(QString(tr("PO - Právnická osoba")));
	this->dataBoxTypeCBox->addItem(QString(tr("PFO - Podnikající fyzická osoba")));
	this->dataBoxTypeCBox->addItem(QString(tr("FO - Fyzická osoba")));

	this->resultsTableWidget->setColumnWidth(0,20);
	this->resultsTableWidget->setColumnWidth(1,60);
	this->resultsTableWidget->setColumnWidth(2,150);

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

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

}

void dlg_ds_search_dialog::checkInputFields(void)
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

