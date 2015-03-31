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
	connect(this->radioImportAll, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
	connect(this->radioImportSelected, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
}

void ImportZFODialog::ChangeRadioBox(void)
{
	if (this->radioImportAll->isChecked()) {
		this->includeSubDir->setEnabled(true);
	} else {
		this->includeSubDir->setEnabled(false);
	}
}

void ImportZFODialog::ImportFiles(void)
{
	enum ZFOtype zfoType = IMPORT_MESSAGE_ZFO;
	enum ZFOaction zfoAaction = IMPORT_FROM_DIR;

	if (this->messageZFO->isChecked()) {
		zfoType = IMPORT_MESSAGE_ZFO;
	} else if (this->deliveryZFO->isChecked()) {
		zfoType = IMPORT_DELIVERY_ZFO;
	} else {
		zfoType = IMPORT_ALL_ZFO;
	}

	if (this->radioImportAll->isChecked()) {
		if (this->includeSubDir->isChecked()) {
			zfoAaction = IMPORT_FROM_SUBDIR;
		} else {
			zfoAaction = IMPORT_FROM_DIR;
		}
	} else if (this->radioImportSelected->isChecked()) {
		zfoAaction = IMPORT_SEL_FILES;
	}

	emit returnZFOAction(zfoType, zfoAaction);
}
