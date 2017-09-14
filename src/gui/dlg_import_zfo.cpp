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

#include "src/gui/dlg_import_zfo.h"
#include "ui_dlg_import_zfo.h"

DlgImportZFO::DlgImportZFO(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgImportZFO)
{
	m_ui->setupUi(this);

	m_ui->infoLabel->setText(tr(
	    "Here you can import whole messages and message acceptance information from ZFO files into the local database. "
	    "The message or acceptance information import will succeed only for those files whose validity can be approved by the ISDS server (working connection to server is required). "
	    "Acceptance information ZFOs will be inserted into the local database only if a corresponding complete message already exists in the local database."));

	connect(m_ui->radioImportAll, SIGNAL(clicked()),
	    this, SLOT(setControlsActivity()));
	connect(m_ui->radioImportSelected, SIGNAL(clicked()),
	    this, SLOT(setControlsActivity()));

	m_ui->checkOnServer->setCheckState(Qt::Checked);
}

DlgImportZFO::~DlgImportZFO(void)
{
	delete m_ui;
}

bool DlgImportZFO::getImportConfiguration(enum Imports::Type &zfoType,
    enum ZFOlocation &locationType, bool &checkZfoOnServer, QWidget *parent)
{
	DlgImportZFO dlg(parent);
	if (QDialog::Accepted != dlg.exec()) {
		return false;
	}

	/* Obtain chosen values. */
	if (dlg.m_ui->messageZFO->isChecked()) {
		zfoType = Imports::IMPORT_MESSAGE;
	} else if (dlg.m_ui->deliveryZFO->isChecked()) {
		zfoType = Imports::IMPORT_DELIVERY;
	} else {
		zfoType = Imports::IMPORT_ANY;
	}

	if (dlg.m_ui->radioImportAll->isChecked()) {
		if (dlg.m_ui->includeSubDir->isChecked()) {
			locationType = IMPORT_FROM_SUBDIR;
		} else {
			locationType = IMPORT_FROM_DIR;
		}
	} else if (dlg.m_ui->radioImportSelected->isChecked()) {
		locationType = IMPORT_SEL_FILES;
	}

	checkZfoOnServer =
	    Qt::Unchecked != dlg.m_ui->checkOnServer->checkState();

	return true;
}

void DlgImportZFO::setControlsActivity(void)
{
	if (m_ui->radioImportAll->isChecked()) {
		m_ui->includeSubDir->setEnabled(true);
	} else {
		m_ui->includeSubDir->setEnabled(false);
	}
}
