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
	if (this->radioImportAll->isChecked()) {
		emit returnZFOAction(IMPOR_FROM_DIR);
	} else if (this->radioImportSelected->isChecked()) {
		emit returnZFOAction(IMPOR_SEL_FILES);
	}
}
