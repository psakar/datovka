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


#include "dlg_import_zfo_result.h"
#include "src/common.h"


ImportZFOResultDialog::ImportZFOResultDialog(int filesCnt,
    QList<QPair<QString,QString>> errImportList,
    QList<QPair<QString,QString>> succImportList,
    QList<QPair<QString,QString>> existImportList,
    QWidget *parent) :
    QDialog(parent),
    m_filesCnt(filesCnt),
    m_errImportList(errImportList),
    m_succImportList(succImportList),
    m_existImportList(existImportList)
{
	setupUi(this);

	QString zfoList = "";

	this->zfoInserted->setStyleSheet("QLabel { color: green }");
	this->zfoErrors->setStyleSheet("QLabel { color: red }");
	this->zfoTotal->setText(QString::number(m_filesCnt));
	this->zfoInserted->setText(QString::number(m_succImportList.size()));
	this->zfoErrors->setText(QString::number(m_errImportList.size()));
	this->zfoExist->setText(QString::number(m_existImportList.size()));

	if (m_succImportList.size() == 0) {
		this->succListHeader->setEnabled(false);
		this->succListHeader->hide();
		this->successList->setEnabled(false);
		this->successList->hide();
	} else {

		for (int i = 0; i < m_succImportList.size(); i++) {
			zfoList += " <b>" + QString::number(i+1) +
			    ". " + m_succImportList.at(i).first + "</b><br/>"
			    + m_succImportList.at(i).second;
		}
		this->successList->setText(zfoList);
	}


	zfoList = "";

	if (m_existImportList.size() == 0) {
		this->existListHeader->setEnabled(false);
		this->existListHeader->hide();
		this->existList->setEnabled(false);
		this->existList->hide();
	} else {
		for (int i = 0; i < m_existImportList.size(); i++) {
			zfoList += " <b>" + QString::number(i+1) +
			    ". " + m_existImportList.at(i).first + "</b><br/>"
			    + m_existImportList.at(i).second;
		}

		this->existList->setText(zfoList);
	}

	zfoList = "";

	if (m_errImportList.size() == 0) {
		this->errListHeader->setEnabled(false);
		this->errListHeader->hide();
		this->errorList->setEnabled(false);
		this->errorList->hide();
	} else {
		for (int i = 0; i < m_errImportList.size(); i++) {
			zfoList += " <b>" + QString::number(i+1) +
			    ". " + m_errImportList.at(i).first + "</b><br/>"
			    + m_errImportList.at(i).second + "<br/>";
		}

		this->errorList->setText(zfoList);
	}
}
