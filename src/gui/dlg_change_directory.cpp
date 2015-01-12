/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */


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
	debugSlotCall();

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
