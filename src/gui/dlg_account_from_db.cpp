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


#include "dlg_account_from_db.h"
#include "src/common.h"

CreateAccountFromDbDialog::CreateAccountFromDbDialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	this->info->setText(tr("A new account will be created according to "
	    "the name and the content of the database file. This account will "
	    "operate over the selected database. Should such an account or "
	    "database file already exist in Datovka then the association will fail."
	    " During the association no database file copy is created nor is the "
	    "content of the database file modified. Nevertheless, we strongly"
	    " advice you to back-up all important files before associating a "
	    "database file. In order for the association to succeed you will need "
	    "an active connection to the ISDS server."));
	connect(this->buttonBox, SIGNAL(accepted()),
	    this, SLOT(CreateAccountFromDbDialogAction()));
}

void CreateAccountFromDbDialog::CreateAccountFromDbDialogAction(void)
{
	if (this->directory->isChecked()) {
		emit returnAction(true);
	} else {
		emit returnAction(false);
	}
}
