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

#include "src/gui/dlg_import_zfo_result.h"
#include "ui_dlg_import_zfo_result.h"

DlgImportZFOResult::DlgImportZFOResult(int filesCnt,
    QList<QPair<QString,QString>> errImportList,
    QList<QPair<QString,QString>> succImportList,
    QList<QPair<QString,QString>> existImportList,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgImportZFOResult),
    m_filesCnt(filesCnt),
    m_errImportList(errImportList),
    m_succImportList(succImportList),
    m_existImportList(existImportList)
{
	m_ui->setupUi(this);

	m_ui->zfoTotal->setText(QString::number(m_filesCnt));
	m_ui->zfoInserted->setStyleSheet("QLabel { color: green }");
	m_ui->zfoInserted->setText(QString::number(m_succImportList.size()));
	m_ui->zfoExist->setText(QString::number(m_existImportList.size()));
	m_ui->zfoErrors->setStyleSheet("QLabel { color: red }");
	m_ui->zfoErrors->setText(QString::number(m_errImportList.size()));

	QString zfoList;

	if (m_succImportList.size() == 0) {
		m_ui->succListHeader->setEnabled(false);
		m_ui->succListHeader->hide();
		m_ui->successList->setEnabled(false);
		m_ui->successList->hide();
	} else {

		for (int i = 0; i < m_succImportList.size(); ++i) {
			zfoList += " <b>" + QString::number(i + 1) +
			    ". " + m_succImportList.at(i).first + "</b><br/>"
			    + m_succImportList.at(i).second;
		}
		m_ui->successList->setText(zfoList);
	}

	zfoList.clear();

	if (m_existImportList.size() == 0) {
		m_ui->existListHeader->setEnabled(false);
		m_ui->existListHeader->hide();
		m_ui->existList->setEnabled(false);
		m_ui->existList->hide();
	} else {
		for (int i = 0; i < m_existImportList.size(); ++i) {
			zfoList += " <b>" + QString::number(i + 1) +
			    ". " + m_existImportList.at(i).first + "</b><br/>"
			    + m_existImportList.at(i).second;
		}

		m_ui->existList->setText(zfoList);
	}

	zfoList.clear();

	if (m_errImportList.size() == 0) {
		m_ui->errListHeader->setEnabled(false);
		m_ui->errListHeader->hide();
		m_ui->errorList->setEnabled(false);
		m_ui->errorList->hide();
	} else {
		for (int i = 0; i < m_errImportList.size(); i++) {
			zfoList += " <b>" + QString::number(i+1) +
			    ". " + m_errImportList.at(i).first + "</b><br/>"
			    + m_errImportList.at(i).second + "<br/>";
		}

		m_ui->errorList->setText(zfoList);
	}
}

DlgImportZFOResult::~DlgImportZFOResult(void)
{
	delete m_ui;
}
