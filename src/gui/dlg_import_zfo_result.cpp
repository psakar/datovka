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

/*!
 * @brief Set text edit header and content according to supplied list.
 *
 * @param[in]     importList List of file name and error description pairs.
 * @param[in,out] listHeader Header label to set.
 * @param[in,out] textEdit Text field whose content to set.
 */
static
void setListData(const QList< QPair<QString, QString> > &importList,
    QLabel *listHeader, QTextEdit *textEdit)
{
	if (Q_UNLIKELY((Q_NULLPTR == listHeader) || (Q_NULLPTR == textEdit))) {
		Q_ASSERT(0);
		return;
	}

	if (importList.size() == 0) {
		listHeader->setEnabled(false);
		listHeader->hide();
		textEdit->setEnabled(false);
		textEdit->hide();
	} else {
		QString message;
		for (int i = 0; i < importList.size(); ++i) {
			message += " <b>" + QString::number(i + 1) +
			    ". " + importList.at(i).first + "</b><br/>"
			    + importList.at(i).second;
		}
		textEdit->setText(message);
	}
}

DlgImportZFOResult::DlgImportZFOResult(int filesCnt,
    const QList< QPair<QString, QString> > &succImportList,
    const QList< QPair<QString, QString> > &existImportList,
    const QList< QPair<QString, QString> > &errImportList,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgImportZFOResult)
{
	m_ui->setupUi(this);

	m_ui->zfoTotal->setText(QString::number(filesCnt));

	m_ui->zfoInserted->setStyleSheet("QLabel { color: green }");
	m_ui->zfoInserted->setText(QString::number(succImportList.size()));
	m_ui->zfoExist->setText(QString::number(existImportList.size()));
	m_ui->zfoErrors->setStyleSheet("QLabel { color: red }");
	m_ui->zfoErrors->setText(QString::number(errImportList.size()));

	setListData(succImportList, m_ui->succListHeader, m_ui->successList);
	setListData(existImportList, m_ui->existListHeader, m_ui->existList);
	setListData(errImportList, m_ui->errListHeader, m_ui->errorList);
}

DlgImportZFOResult::~DlgImportZFOResult(void)
{
	delete m_ui;
}
