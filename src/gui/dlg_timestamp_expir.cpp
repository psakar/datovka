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


#include "dlg_timestamp_expir.h"
#include "src/common.h"

TimestampExpirDialog::TimestampExpirDialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(setRetValue()));
	connect(this->radioFromDir, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
	connect(this->radioFromCurrent, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
	connect(this->radioFromAll, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
}


void TimestampExpirDialog::ChangeRadioBox(void)
{
	if (this->radioFromDir->isChecked()) {
		this->includeSubDir->setEnabled(true);
	} else {
		this->includeSubDir->setEnabled(false);
	}
}


void TimestampExpirDialog::setRetValue(void)
{
	enum TSaction action = CHECK_TIMESTAMP_CURRENT;

	if (this->radioFromDir->isChecked()) {
		if (this->includeSubDir->isChecked()) {
			action = CHECK_TIMESTAMP_ZFO_SUB;
		} else {
			action = CHECK_TIMESTAMP_ZFO;
		}
	} else if (this->radioFromAll->isChecked()) {
		action = CHECK_TIMESTAMP_ALL;
	}

	emit returnAction(action);
}
