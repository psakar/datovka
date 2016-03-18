/*
 * Copyright (C) 2014-2016 CZ.NIC
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
#include <QDebug>
#include <QMessageBox>

#include "dlg_tag.h"
#include "ui_dlg_tag.h"
#include "src/io/tag_db.h"


/* ========================================================================= */
/*
 * Constructor for new tag.
 */
TagDialog::TagDialog(QWidget *parent)
/* ========================================================================= */
    :
    QDialog(parent),
    m_tagid(NEWTAG_ID),
    m_tagName(QString()),
    m_tagColor(NEWTAG_COLOR),
    ui(new Ui::TagDialog)
{
	ui->setupUi(this);
	initTagDialog();

}


/* ========================================================================= */
/*
 * Constructor for existing tag.
 */
TagDialog::TagDialog(int tagId, QString tagName, QString tagColor,
    QWidget *parent)
/* ========================================================================= */
    :
    QDialog(parent),
    m_tagid(tagId),
    m_tagName(tagName),
    m_tagColor(tagColor),
    ui(new Ui::TagDialog)
{
	ui->setupUi(this);
	initTagDialog();
}


/* ========================================================================= */
/*
 * Destructor.
 */
TagDialog::~TagDialog()
/* ========================================================================= */
{
	delete ui;
}


/* ========================================================================= */
/*
 * Initialize dialog.
 */
void TagDialog::initTagDialog(void)
/* ========================================================================= */
{
	ui->currentColor->setEnabled(false);
	m_tagColor = "#" + m_tagColor;
	ui->tagNamelineEdit->setText(m_tagName);
	setPreviewButtonColor();

	connect(ui->changeColorPushButton, SIGNAL(clicked()), this,
	    SLOT(setNewColor()));
	connect(ui->buttonBox, SIGNAL(accepted()), this,
	    SLOT(saveTag()));
}


/* ========================================================================= */
/*
 * Change/choose a new color of tag.
 */
void TagDialog::setNewColor(void)
/* ========================================================================= */
{
	QColor color = QColorDialog::getColor(QColor(m_tagColor), this,
	    tr("Choose tag color"));

	if (color.isValid()) {
		m_tagColor = color.name().toLower();
		setPreviewButtonColor();
	}
}


/* ========================================================================= */
/*
 * Insert/update tag in database.
 */
void TagDialog::saveTag(void)
/* ========================================================================= */
{
	m_tagName = ui->tagNamelineEdit->text();

	if (m_tagName.isEmpty()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setWindowTitle(tr("Tag error"));
		msgBox.setText(tr("Tag name is empty.").arg(m_tagName));
		msgBox.setInformativeText(tr("Tag wasn't created."));
		msgBox.exec();
		return;
	}

	if (m_tagid != NEWTAG_ID) {
		globTagDbPtr->
		    updateTag(m_tagid, m_tagName, m_tagColor.replace("#", ""));
	} else {
		if (!globTagDbPtr->insertTag(m_tagName, m_tagColor.replace("#", ""))) {
			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setWindowTitle(tr("Tag error"));
			msgBox.setText(tr("Tag with name '%1'' already "
			   "exists in databaze.").arg(m_tagName));
			msgBox.setInformativeText(tr("Tag wasn't created again."));
			msgBox.exec();
		}
	}
}


/* ========================================================================= */
/*
 * Set current tag color to preview button.
 */
void TagDialog::setPreviewButtonColor(void)
/* ========================================================================= */
{
	QPalette pal = ui->currentColor->palette();
	pal.setColor(QPalette::Button, QColor(m_tagColor));
	const QString style = "border-style: outset; background-color: ";
	ui->currentColor->setStyleSheet(style + m_tagColor);
}
