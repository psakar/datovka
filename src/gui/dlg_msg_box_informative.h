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

#ifndef _DLG_MSG_BOX_INFORMATIVE_H_
#define _DLG_MSG_BOX_INFORMATIVE_H_

#include <QMessageBox>

/*!
 * @brief Encapsulates QMessageBox with informative field.
 */
class DlgMsgBox {

private:
	/*!
	 * @brief Private constructor.
	 */
	DlgMsgBox(void);

public:
	/*!
	 * @brief Show message box with informative text.
	 *
	 * @param[in] parent Window parent.
	 * @param[in] icon Windows icon.
	 * @param[in] title Window title.
	 * @param[in] text Text to be displayed.
	 * @param[in] infoText Informative text.
	 * @param[in] detailText Detailed text.
	 * @param[in] buttons Displayed buttons.
	 * @param[in] defaultButton Default button.
	 * @return Standard button code.
	 */
	static
	int message(QWidget *parent, enum QMessageBox::Icon icon,
	    const QString &title, const QString &text,
	    const QString &infoText, const QString &detailText,
	    QMessageBox::StandardButtons buttons = QMessageBox::Ok,
	    enum QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
};

#endif /* _DLG_MSG_BOX_INFORMATIVE_H_ */
