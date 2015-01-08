#include "dlg_import_zfo.h"
#include "src/common.h"

ImportZFODialog::ImportZFODialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(ImportFiles()));
}

void ImportZFODialog::ImportFiles(void)
{
	int zfoType, zfoAaction;

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
