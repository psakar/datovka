#include "dlg_db_import.h"
#include "src/common.h"

DbImportDialog::DbImportDialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	this->info->setText(tr("A new account will be created according to "
	    "the name and the content of the database file. This account will "
	    "operate over the selected database. Should such an account or "
	    "database file already exist in Datovka then the import will fail."
	    " During the import no database file copy is created nor is the "
	    "content of the database file modified. Nevertheless, we strongly"
	    " advice you to back-up all important files before importing a "
	    "database file. In order for the import to succeed you will need "
	    "an active connection to the ISDS server."));
	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(ImportDbFiles()));
}

void DbImportDialog::ImportDbFiles(void)
{
	if (this->directory->isChecked()) {
		emit returnDbAction(IMPORT_FROM_DIR);
	} else {
		emit returnDbAction(IMPORT_SEL_FILES);
	}
}
