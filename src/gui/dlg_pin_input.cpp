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

#include <QPixmap>

#include "src/common.h"
#include "src/gui/dlg_pin_input.h"
#include "src/log/log.h"
#include "ui_dlg_pin_input.h"

/*!
 * @brief Remove and delete layout items.
 *
 * @param[in] layout Horizontal or vertical layout.
 * @param[in] pos Position identifier.
 */
static
void removeLayoutItem(QBoxLayout *layout, int pos)
{
	if (Q_UNLIKELY(layout == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	QLayoutItem *item = layout->itemAt(pos);
	if (Q_UNLIKELY(item == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	layout->removeItem(item);
	delete item;
}

DlgPinInput::DlgPinInput(bool viewLogo, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgPinInput)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	if (viewLogo) {
		m_ui->logoLabel->setPixmap(QPixmap(ICON_128x128_PATH "datovka.png"));
		m_ui->logoLabel->setAlignment(Qt::AlignHCenter);
		m_ui->horizontalLayout->setStretch(1, 10);

		m_ui->versionLabel->setTextFormat(Qt::RichText);
		m_ui->versionLabel->setText(QLatin1String("<b>") +
		    tr("Version") + QLatin1String(": ") + VERSION +
		    QLatin1String("</b>"));
	} else {
		/* Remove parts of the dialogue layout. */
		removeLayoutItem(m_ui->horizontalLayout, 0);

		removeLayoutItem(m_ui->verticalLayout_3, 2);
		removeLayoutItem(m_ui->verticalLayout_3, 0);

		m_ui->logoLabel->hide();
		m_ui->logoLabel->setEnabled(false);
		m_ui->versionLabel->hide();
		m_ui->versionLabel->setEnabled(false);

		this->adjustSize();
	}

	m_ui->pinLine->setEchoMode(QLineEdit::Password);
	/* Always display placeholder text. */
	m_ui->pinLine->setMinimumWidth(1.2 *
	    m_ui->pinLine->fontMetrics().width(
	        m_ui->pinLine->placeholderText()));
}

DlgPinInput::~DlgPinInput(void)
{
	delete m_ui;
}

bool DlgPinInput::queryPin(PinSettings &sett, bool viewLogo, QWidget *parent)
{
	if (Q_UNLIKELY(!sett.pinConfigured())) {
		Q_ASSERT(0);
		return false;
	}

	bool properlySet = false;

	do {
		DlgPinInput dlg(viewLogo, parent);
		if (QDialog::Accepted == dlg.exec()) {
			if (dlg.m_ui->pinLine->text().isEmpty()) {
				logWarningNL("%s", "Entered empty PIN value.");
				continue;
			}

			if (!PinSettings::verifyPin(sett,
			        dlg.m_ui->pinLine->text())) {
				logWarningNL("%s",
				    "Could not verify entered PIN value.");
				continue;
			}

			properlySet = true;
		} else {
			return false;
		}
	} while (!properlySet);

	return true;
}
