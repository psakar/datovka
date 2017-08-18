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

#include "src/gui/dlg_msg_box_informative.h"

int DlgMsgBox::message(QWidget *parent, enum QMessageBox::Icon icon,
    const QString &title, const QString &text, const QString &infoText,
    const QString &detailText, QMessageBox::StandardButtons buttons,
    enum QMessageBox::StandardButton defaultButton)
{
	QMessageBox msgBox(parent);
	msgBox.setIcon(icon);
	msgBox.setWindowTitle(title);
	msgBox.setText(text);
	if (!infoText.isEmpty()) {
		msgBox.setInformativeText(infoText);
	}
	if (!detailText.isEmpty()) {
		msgBox.setDetailedText(detailText);
	}
	msgBox.setStandardButtons(buttons);
	msgBox.setDefaultButton(defaultButton);
	return msgBox.exec();
}
