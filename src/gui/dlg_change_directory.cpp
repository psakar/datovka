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

#include <QFileDialog>

#include "src/gui/dlg_change_directory.h"
#include "ui_dlg_change_directory.h"

namespace Ui {
	class DlgChangeDirectory;
}

DlgChangeDirectory::DlgChangeDirectory(const QString &dirPath, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgChangeDirectory),
    m_dirPath(dirPath)
{
	m_ui->setupUi(this);

	m_ui->newPath->setText("");
	m_ui->currentPath->setText(m_dirPath);
	m_ui->labelWarning->setStyleSheet("QLabel { color: red }");

	if (m_ui->newPath->text().isEmpty()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (m_ui->currentPath->text() != m_ui->newPath->text()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		m_ui->labelWarning->show();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}

	connect(m_ui->chooseButton, SIGNAL(clicked()), this,
	    SLOT(onDirectoryChange(void)));

	connect(m_ui->buttonBox, SIGNAL(accepted()), this,
	    SLOT(setNewDataDirectory(void)));
}

DlgChangeDirectory::~DlgChangeDirectory(void)
{
	delete m_ui;
}

void DlgChangeDirectory::onDirectoryChange(void)
{
	QString newDir(QFileDialog::getExistingDirectory(this,
	    tr("Open Directory"), QString(),
	    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks));
	m_ui->newPath->setText(newDir);

	if (m_ui->newPath->text().isEmpty()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	} else if (m_ui->currentPath->text() != m_ui->newPath->text()) {
		m_ui->labelWarning->hide();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	} else {
		m_ui->labelWarning->show();
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}
}

void DlgChangeDirectory::setNewDataDirectory(void)
{
	QString action;
	if (m_ui->moveDataRadioButton->isChecked()) {
		action = "move";
	} else if (m_ui->copyDataRadioButton->isChecked()) {
		action = "copy";
	} else {
		action = "new";
	}

	emit sentNewPath(m_dirPath, m_ui->newPath->text(), action);
}
