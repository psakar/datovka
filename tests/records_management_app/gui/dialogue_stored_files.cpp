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

#include <limits> // std::numeric_limits
#include <QInputDialog>
#include <QMessageBox>
#include <QtGlobal> // qrand
#include <QTime>

#include "src/records_management/conversion.h"
#include "tests/records_management_app/gui/dialogue_stored_files.h"
#include "ui_dialogue_stored_files.h"

DialogueStoredFiles::DialogueStoredFiles(QWidget *parent)
    : QDialog(parent),
    m_ui(new Ui::DialogueStoredFiles)
{
	m_ui->setupUi(this);

	setWindowTitle("Stored Files Request");

	connect(m_ui->dmButton, SIGNAL(clicked(bool)),
	    this, SLOT(generateDmIds()));
	connect(m_ui->diButton, SIGNAL(clicked(bool)),
	    this, SLOT(generateDiIds()));
}

DialogueStoredFiles::~DialogueStoredFiles(void)
{
	delete m_ui;
}

/*!
 * @brief Split string and convert to integers.
 *
 * @param[in]  str String to be split.
 * @param[in]  sep Separator to be used for splitting.
 * @param[out] ok Set to true, if all values have been converted.
 * @return Empty list on error, list of numbers else.
 */
static
QList<qint64> readIdentifiers(const QString &str, QChar sep, bool *ok)
{
	if (str.isEmpty()) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return QList<qint64>();
	}

	QStringList strIds(str.split(sep));
	QList<qint64> ids(createIdList(strIds, Q_NULLPTR));
	if (ids.size() != strIds.size()) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return QList<qint64>();
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ids;
}

#define SEPARATOR ','

DialogueStoredFiles::Values DialogueStoredFiles::getIdentifiers(void)
{
	Values retVal;

	DialogueStoredFiles dlg;
	int ret = dlg.exec();
	if (ret == QDialog::Rejected) {
		return Values();
	}

	/* Read dialogue content. */
	bool ok = false;
	retVal.dmIds = readIdentifiers(dlg.m_ui->dmLine->text(), SEPARATOR, &ok);
	if (!ok) {
		QMessageBox::warning(&dlg, QStringLiteral("Conversion Error"),
		    QStringLiteral("Could not convert all data message identifiers to non-negative numbers."));
		return Values();
	}
	retVal.diIds = readIdentifiers(dlg.m_ui->diLine->text(), SEPARATOR, &ok);
	if (!ok) {
		QMessageBox::warning(&dlg, QStringLiteral("Conversion Error"),
		    QStringLiteral("Could not convert all delivery info identifiers to non-negative numbers."));
		return Values();
	}

	return retVal;
}

/*!
 * @brief Asks the user for a number.
 *
 * @param[in] parent Parent widget.
 * @return Non-negative integer, or negative integer on error.
 */
static
int askNumberOfIdentifiers(QWidget *parent)
{
	return QInputDialog::getInt(parent,
	    QStringLiteral("Number of Identifiers"),
	    QStringLiteral("Enter number of identifiers that should be generated:"),
	    0, 0, std::numeric_limits<int>::max(), 1);
}

/*!
 * @brief Fill random identifiers into line edit.
 *
 * @param[in]  parent Parent widget.
 * @param[out] lineEdit Line edit to be filled.
 */
static
void generateIdentifiers(QWidget *parent, QLineEdit &lineEdit)
{
	int num = askNumberOfIdentifiers(parent);
	if (num < 0) {
		Q_ASSERT(0);
		return;
	}

	qsrand((uint)QTime::currentTime().msec());
	QStringList strIds;
	for (int i = 0; i < num; ++i) {
		strIds.append(QString::number(qrand()));
	}

	lineEdit.setText(strIds.join(SEPARATOR));
}

void DialogueStoredFiles::generateDmIds(void)
{
	generateIdentifiers(this, *m_ui->dmLine);
}

void DialogueStoredFiles::generateDiIds(void)
{
	generateIdentifiers(this, *m_ui->diLine);
}
