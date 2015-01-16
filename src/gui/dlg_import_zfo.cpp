#include "dlg_import_zfo.h"
#include "src/common.h"

ImportZFODialog::ImportZFODialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	this->info->setText(tr("Here you can import whole messages and "
	    "message delivery information from ZFO files into local database."
	    " The message or delivery information import will succeed only "
	    "for those files whose validity can be approved by the Datové "
	    "schránky server (working connection to server is required). "
	    "Delivery information ZFO will be inserted into local database "
	    "only if a corresponding complete message already exists in the "
	    "database."));
	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(ImportFiles()));
}

void ImportZFODialog::ImportFiles(void)
{
	int zfoType = IMPORT_MESSAGE_ZFO;
	int zfoAaction = IMPORT_FROM_DIR;

	if (this->messageZFO->isChecked()) {
		zfoType = IMPORT_MESSAGE_ZFO;
	} else if (this->deliveryZFO->isChecked()) {
		zfoType = IMPORT_DELIVERY_ZFO;
	}

	if (this->radioImportAll->isChecked()) {
		zfoAaction = IMPORT_FROM_DIR;
	} else if (this->radioImportSelected->isChecked()) {
		zfoAaction = IMPORT_SEL_FILES;
	}

	emit returnZFOAction(zfoType, zfoAaction);
}
