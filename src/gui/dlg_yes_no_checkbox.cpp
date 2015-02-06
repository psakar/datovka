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

#include "dlg_yes_no_checkbox.h"


YesNoCheckboxDialog::YesNoCheckboxDialog(QString dlgTitleText,
    QString questionText, QString checkBoxText, QString detailText,
    QWidget *parent) :
    QDialog(parent),
    m_dlgTitleText(dlgTitleText),
    m_questionText(questionText),
    m_checkBoxText(checkBoxText),
    m_detailText(detailText)
{
	setupUi(this);

	this->setWindowTitle(m_dlgTitleText);
	this->labelQuestion->setText(m_questionText);
	this->advanceDeleteCheckBox->setText(m_checkBoxText);
	if (m_detailText.isNull() || m_detailText.isEmpty()) {
		this->labelDetailText->setEnabled(false);
	} else {
		this->labelDetailText->setText(m_detailText + "\n");
		this->labelDetailText->setEnabled(true);
	}

	this->adjustSize();

	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(isCheckboxChecked()));
}


void YesNoCheckboxDialog::isCheckboxChecked(void)
{
	if (this->advanceDeleteCheckBox->isChecked()) {
		done(YesChecked);
	} else {
		done(YesUnchecked);
	}
}
