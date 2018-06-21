/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include <QMessageBox>

#include "src/gui/dlg_pin_setup.h"
#include "ui_dlg_pin_setup.h"

DlgPinSetup::DlgPinSetup(enum Operation op, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgPinSetup)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	m_ui->currentPinLine->setEchoMode(QLineEdit::Password);
	m_ui->newPinLine->setEchoMode(QLineEdit::Password);
	m_ui->newPinLine2->setEchoMode(QLineEdit::Password);

	m_ui->currentPinLabel->setEnabled(op != SET);
	m_ui->currentPinLine->setEnabled(op != SET);
	m_ui->newPinLabel->setEnabled(op != ERASE);
	m_ui->newPinLine->setEnabled(op != ERASE);
	m_ui->newPinLabel2->setEnabled(op != ERASE);
	m_ui->newPinLine2->setEnabled(op != ERASE);
}

DlgPinSetup::~DlgPinSetup(void)
{
	delete m_ui;
}

bool DlgPinSetup::change(PinSettings &sett, QWidget *parent)
{
	return update(!sett.pinConfigured() ? SET : MODIFY, sett, parent);
}

bool DlgPinSetup::erase(PinSettings &sett, QWidget *parent)
{
	return update(ERASE, sett, parent);
}

bool DlgPinSetup::update(enum Operation op, PinSettings &sett, QWidget *parent)
{
	if (Q_UNLIKELY((op != SET) && sett._pinVal.isEmpty())) {
		/* Decrypted PIN value must already be set. */
		Q_ASSERT(0);
		return false;
	}

	bool properlySet = false;

	do {
		DlgPinSetup dlg(op, parent);
		if (QDialog::Accepted == dlg.exec()) {
			if ((op != SET) &&
			    (sett._pinVal != dlg.m_ui->currentPinLine->text())) {
				/* Current PIN was not entered properly. */
				QMessageBox::warning(parent,
				    tr("Wrong PIN value"),
				    tr("Entered wrong current PIN."),
				    QMessageBox::Ok, QMessageBox::Ok);
				continue;
			}

			if ((op != ERASE) &&
			    (dlg.m_ui->newPinLine->text() != dlg.m_ui->newPinLine2->text())) {
				/* New PIN was not entered properly. */
				QMessageBox::warning(parent,
				    tr("Wrong PIN value"),
				    tr("Entered new PIN values are different."),
				    QMessageBox::Ok, QMessageBox::Ok);
				continue;
			}

			PinSettings::updatePinSettings(sett,
			    (op != ERASE) ? dlg.m_ui->newPinLine->text() : QString());

			properlySet = true;
		} else {
			/* Dialogue cancelled. */
			return false;
		}
	} while (!properlySet);

	return true;
}
