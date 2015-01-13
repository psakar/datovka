#include "dlg_import_zfo.h"
#include "src/common.h"

ImportZFODialog::ImportZFODialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	this->info->setText(tr("Zde je možné importovat do lokální databáze kompletní zprávy a informace o doručení zprávy ze souborů ZFO. Import kompletní zprávy bude úspěšný pouze pro takové ZFO soubory u kterých bude možné na serveru Datové schránky ověřit jejich pravost. Informace o doručení zprávy budou do databáze vloženy ze souboru ZFO pouze tehdy, existuje-li v lokální databáze adekvátní kompletní zpráva."));
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
