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


#ifndef DLG_YES_NO_CHECKBOX_H
#define DLG_YES_NO_CHECKBOX_H

#include <QDialog>
#include "ui_dlg_yes_no_checkbox.h"


class YesNoCheckboxDialog : public QDialog, public Ui::YesNoCheckboxDlg
{
	Q_OBJECT

public:

	enum RetVal {
		No = 0,
		YesUnchecked = 1,
		YesChecked = 2
	};

	YesNoCheckboxDialog(QString dlgTitleText, QString questionText,
	    QString checkBoxText, QString detailText, QWidget *parent = 0);

private slots:

	void isCheckboxChecked(void);

private:

	QString m_dlgTitleText;
	QString m_questionText;
	QString m_checkBoxText;
	QString m_detailText;
};

#endif // DLG_YES_NO_CHECKBOX_H
