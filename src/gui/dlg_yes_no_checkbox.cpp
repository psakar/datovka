/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include "src/gui/dlg_yes_no_checkbox.h"

DlgYesNoCheckbox::DlgYesNoCheckbox(const QString &title,
    const QString &questionText, const QString &checkBoxText,
    const QString &detailText, QWidget *parent)
    : QDialog(parent)
{
	setupUi(this);

	this->setWindowTitle(title);
	this->labelQuestion->setText(questionText);
	this->advanceDeleteCheckBox->setText(checkBoxText);
	this->labelDetailText->setEnabled(!detailText.isEmpty());
	this->labelDetailText->setText(detailText + "\n");

	this->adjustSize();

	connect(this->buttonBox, SIGNAL(accepted()),
	    this, SLOT(isCheckboxChecked()));
}

void DlgYesNoCheckbox::isCheckboxChecked(void)
{
	if (this->advanceDeleteCheckBox->isChecked()) {
		done(YesChecked);
	} else {
		done(YesUnchecked);
	}
}
