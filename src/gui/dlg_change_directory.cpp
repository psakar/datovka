#include <QFileDialog>

#include "dlg_change_directory.h"
#include "dlg_change_directory.h"


dlg_change_directory::dlg_change_directory(QString dirPath, QWidget *parent) :
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
void dlg_change_directory::initDialog(void)
/* ========================================================================= */
{
	this->newPath->setText("");
	this->currentPath->setText(m_dirPath);

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
void dlg_change_directory::onDirectoryChange(void)
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
 * Set new data directory from action
 */
void dlg_change_directory::setNewDataDirectory(void)
/* ========================================================================= */
{
	/* TODO - Move/copy current account data and save to pref */

	qDebug() << __func__;
}
