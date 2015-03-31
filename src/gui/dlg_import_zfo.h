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


#ifndef _DLG_IMPORT_ZFO_H_
#define _DLG_IMPORT_ZFO_H_


#include <QDialog>
#include "src/common.h"
#include "ui_dlg_import_zfo.h"


class ImportZFODialog : public QDialog, public Ui::ImportZFO
{
	Q_OBJECT

public:
	enum ZFOaction {
		IMPORT_FROM_DIR,
		IMPORT_FROM_SUBDIR,
		IMPORT_SEL_FILES
	};

	enum ZFOtype {
		IMPORT_ALL_ZFO,
		IMPORT_MESSAGE_ZFO,
		IMPORT_DELIVERY_ZFO
	};

public:
	ImportZFODialog(QWidget *parent = 0);

signals:
	void returnZFOAction(enum ImportZFODialog::ZFOtype,
	    enum ImportZFODialog::ZFOaction);

private slots:
	void ImportFiles(void);
	void ChangeRadioBox(void);
};

#endif /* _DLG_IMPORT_ZFO_H_ */
