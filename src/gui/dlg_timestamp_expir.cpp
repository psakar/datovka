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

#include "src/gui/dlg_timestamp_expir.h"
#include "ui_dlg_timestamp_expir.h"

DlgTimestampExpir::DlgTimestampExpir(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTimestampExpir)
{
	m_ui->setupUi(this);

	connect(m_ui->radioFromCurrent, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
	connect(m_ui->radioFromAll, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));
	connect(m_ui->radioFromDir, SIGNAL(clicked()),
	    this, SLOT(ChangeRadioBox()));

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(setRetValue()));
}

DlgTimestampExpir::~DlgTimestampExpir(void)
{
	delete m_ui;
}

void DlgTimestampExpir::ChangeRadioBox(void)
{
	if (m_ui->radioFromDir->isChecked()) {
		m_ui->includeSubDir->setEnabled(true);
	} else {
		m_ui->includeSubDir->setEnabled(false);
	}
}

void DlgTimestampExpir::setRetValue(void)
{
	enum TSaction action = CHECK_TIMESTAMP_CURRENT;

	if (m_ui->radioFromDir->isChecked()) {
		if (m_ui->includeSubDir->isChecked()) {
			action = CHECK_TIMESTAMP_ZFO_SUB;
		} else {
			action = CHECK_TIMESTAMP_ZFO;
		}
	} else if (m_ui->radioFromAll->isChecked()) {
		action = CHECK_TIMESTAMP_ALL;
	}

	emit returnAction(action);
}
