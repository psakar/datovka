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


#ifndef DLG_TIMESTAMP_EXPIR_H
#define DLG_TIMESTAMP_EXPIR_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_timestamp_expir.h"


class TimestampExpirDialog : public QDialog, public Ui::DlgTimestampExpir
{
	Q_OBJECT

public:
	enum TSaction {
		CHECK_TIMESTAMP_CURRENT,
		CHECK_TIMESTAMP_ALL,
		CHECK_TIMESTAMP_ZFO,
		CHECK_TIMESTAMP_ZFO_SUB
	};

public:
	TimestampExpirDialog(QWidget *parent = 0);

signals:
	void returnAction(enum TimestampExpirDialog::TSaction);

private slots:
	void setRetValue(void);
	void ChangeRadioBox(void);
};

#endif // DLG_TIMESTAMP_EXPIR_H
