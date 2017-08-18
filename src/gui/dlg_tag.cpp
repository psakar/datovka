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

#include <QColorDialog>
#include <QMessageBox>

#include "src/gui/dlg_tag.h"
#include "src/io/tag_db.h"
#include "ui_dlg_tag.h"

DlgTag::DlgTag(const QString &userName, TagDb *tagDb, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTag),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_tagItem()
{
	m_ui->setupUi(this);

	initDlg();
}

DlgTag::DlgTag(const QString &userName, TagDb *tagDb, const TagItem &tag,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTag),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_tagItem(tag)
{
	m_ui->setupUi(this);

	initDlg();
}

DlgTag::~DlgTag(void)
{
	delete m_ui;
}

void DlgTag::initDlg(void)
{
	m_ui->currentColor->setEnabled(false);
	m_ui->tagNamelineEdit->setText(m_tagItem.name);
	setPreviewButtonColor();

	connect(m_ui->changeColorPushButton, SIGNAL(clicked()), this,
	    SLOT(chooseNewColor()));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this,
	    SLOT(saveTag()));
}

void DlgTag::chooseNewColor(void)
{
	QColor colour = QColorDialog::getColor(QColor("#" + m_tagItem.colour),
	    this, tr("Choose tag colour"));

	if (colour.isValid()) {
		QString colourName = colour.name().toLower().replace("#", "");
		if (TagItem::isValidColourStr(colourName)) {
			m_tagItem.colour = colourName;
		}
		setPreviewButtonColor();
	}
}

void DlgTag::saveTag(void)
{
	m_tagItem.name = m_ui->tagNamelineEdit->text();

	if (m_tagItem.name.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle(tr("Tag error"));
		msgBox.setText(tr("Tag name is empty."));
		msgBox.setInformativeText(tr("Tag wasn't created."));
		msgBox.exec();
		return;
	}

	Q_ASSERT(TagItem::isValidColourStr(m_tagItem.colour));

	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);

	if (m_tagItem.id >= 0) {
		m_tagDbPtr->updateTag(m_tagItem.id, m_tagItem.name,
		    m_tagItem.colour);
	} else {
		if (!m_tagDbPtr->insertTag(m_tagItem.name, m_tagItem.colour)) {
			msgBox.setWindowTitle(tr("Tag error"));
			msgBox.setText(tr("Tag with name '%1'' already "
			    "exists in database.").arg(m_tagItem.name));
			msgBox.setInformativeText(
			    tr("Tag wasn't created again."));
			msgBox.exec();
		}
	}
}

void DlgTag::setPreviewButtonColor(void)
{
	QPalette pal(m_ui->currentColor->palette());
	pal.setColor(QPalette::Button, QColor("#" + m_tagItem.colour));
	const QString style = "border-style: outset; background-color: ";
	m_ui->currentColor->setStyleSheet(style + "#" + m_tagItem.colour);
}
