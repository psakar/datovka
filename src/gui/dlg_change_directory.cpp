

#include <QFileDialog>

#include "dlg_change_directory.h"
#include "src/log/log.h"

DlgChangeDirectory::DlgChangeDirectory(QString dirPath, QWidget *parent) :
    QDialog(parent),
    m_dirPath(dirPath)
{
	setupUi(this);
	initDialog();
}

/* ========================================================================= */
/*
 * Init dialog
 */
void DlgChangeDirectory::initDialog(void)
/* ========================================================================= */
{
	this->newPath->setText("");
	this->currentPath->setText(m_dirPath);
	this->labelWarning->setStyleSheet("QLabel { color: red }");

	if (this->newPath->text().isEmpty() || this->newPath->text().isNull()) {
		this->labelWarning->hide();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (this->currentPath->text() != this->newPath->text()) {
		this->labelWarning->hide();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		this->labelWarning->show();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}

	connect(this->chooseButton, SIGNAL(clicked()), this,
	    SLOT(onDirectoryChange(void)));

	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(setNewDataDirectory(void)));
}


/* ========================================================================= */
/*
 * Choose new data directory
 */
void DlgChangeDirectory::onDirectoryChange(void)
/* ========================================================================= */
{
	QString newdir = QFileDialog::getExistingDirectory(this,
	    tr("Open Directory"), NULL, QFileDialog::ShowDirsOnly |
	    QFileDialog::DontResolveSymlinks);
	this->newPath->setText(newdir);

	if (this->newPath->text().isEmpty() || this->newPath->text().isNull()) {
		this->labelWarning->hide();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (this->currentPath->text() != this->newPath->text()) {
		this->labelWarning->hide();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		this->labelWarning->show();
		this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}
}


/* ========================================================================= */
/*
 * Set new data directory and save path
 */
void DlgChangeDirectory::setNewDataDirectory(void)
/* ========================================================================= */
{
	debug_func_call();

	QString action;
	if (this->moveDataRadioButton->isChecked()) {
		action = "move";
	} else if (this->copyDataRadioButton->isChecked()) {
		action = "copy";
	} else {
		action = "new";
	}

	emit sentNewPath(m_dirPath, this->newPath->text(), action);
}
