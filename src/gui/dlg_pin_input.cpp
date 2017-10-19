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

#include <QPixmap>

#include "src/common.h"
#include "src/gui/dlg_pin_input.h"
#include "ui_dlg_pin_input.h"

DlgPinInput::DlgPinInput(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgPinInput)
{
	m_ui->setupUi(this);

	m_ui->logoLabel->setPixmap(QPixmap(ICON_128x128_PATH "datovka.png"));

	m_ui->versionLabel->setTextFormat(Qt::RichText);
	m_ui->versionLabel->setText(
	    QLatin1String("<b>") + tr("Version") + QLatin1String(": ") + VERSION + QLatin1String("</b>"));

	m_ui->pinLine->setPlaceholderText(tr("Enter PIN code"));
	m_ui->pinLine->setEchoMode(QLineEdit::Password);
}

DlgPinInput::~DlgPinInput(void)
{
	delete m_ui;
}

bool DlgPinInput::queryPin(PinSettings &sett, QWidget *parent)
{
	DlgPinInput dlg(parent);
	if (QDialog::Accepted == dlg.exec()) {
		/* TODO */
		return true;
	} else {
		return false;
	}
}
