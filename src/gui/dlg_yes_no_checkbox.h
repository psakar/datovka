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

#ifndef _DLG_YES_NO_CHECKBOX_H_
#define _DLG_YES_NO_CHECKBOX_H_

#include <QMessageBox>

class DlgYesNoCheckbox : public QMessageBox {
	Q_OBJECT

public:
	/*!
	 * @brief Value returned by exec().
	 */
	enum RetVal {
		No = 0,
		YesUnchecked = 1,
		YesChecked = 2
	};

	/*!
	 * @brief Constructs question dialogue containing a checkbox.
	 *
	 * @param[in] title Window title.
	 * @param[in] questionText Question text.
	 * @param[in] checkBoxText Text displayed next to the checkbox.
	 * @param[in] detailText Detailed description text.
	 * @param[in] parent Window parent.
	 */
	DlgYesNoCheckbox(const QString &title, const QString &questionText,
	    const QString &checkBoxText, const QString &detailText,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Check whether the dialogue has been accepted, calls done().
	 */
	void buttonClicked(QAbstractButton *button);

private:
	const QAbstractButton *m_yesButton; /*!< Pointer to accept button. */
};

#endif /* _DLG_YES_NO_CHECKBOX_H_ */
