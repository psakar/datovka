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

#include "src/gui/dlg_msg_box_informative.h"
#include "src/gui/dlg_tag.h"
#include "src/io/tag_db.h"
#include "ui_dlg_tag.h"

DlgTag::DlgTag(const TagItem &tag, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTag),
    m_tagItem(tag)
{
	m_ui->setupUi(this);

	m_ui->currentColor->setEnabled(false);
	m_ui->tagNamelineEdit->setText(m_tagItem.name);
	tagNameChanged(m_tagItem.name);
	setPreviewButtonColor();

	connect(m_ui->tagNamelineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(tagNameChanged(QString)));
	connect(m_ui->changeColorPushButton, SIGNAL(clicked()),
	    this, SLOT(chooseNewColor()));
}

DlgTag::~DlgTag(void)
{
	delete m_ui;
}

bool DlgTag::createTag(TagDb *tagDb, QWidget *parent)
{
	if (Q_UNLIKELY(Q_NULLPTR == tagDb)) {
		Q_ASSERT(0);
		return false;
	}

	DlgTag dlg(TagItem(), parent);
	if (dlg.exec() == QDialog::Accepted) {
		return saveTag(tagDb, dlg.m_tagItem, parent);
	}
	return false;
}

bool DlgTag::editTag(TagDb *tagDb, const TagItem &tag, QWidget *parent)
{
	if (Q_UNLIKELY(Q_NULLPTR == tagDb)) {
		Q_ASSERT(0);
		return false;
	}

	DlgTag dlg(tag, parent);
	if (dlg.exec() == QDialog::Accepted) {
		return saveTag(tagDb, dlg.m_tagItem, parent);
	}
	return false;
}

void DlgTag::tagNameChanged(const QString &tagName)
{
	m_tagItem.name = tagName;

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    !tagName.isEmpty());
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

void DlgTag::setPreviewButtonColor(void)
{
	QPalette pal(m_ui->currentColor->palette());
	pal.setColor(QPalette::Button, QColor("#" + m_tagItem.colour));
	const QString style = "border-style: outset; background-color: ";
	m_ui->currentColor->setStyleSheet(style + "#" + m_tagItem.colour);
}

bool DlgTag::saveTag(TagDb *tagDb, const TagItem &tagItem, QWidget *parent)
{
	if (Q_UNLIKELY(Q_NULLPTR == tagDb)) {
		Q_ASSERT(0);
		return false;
	}

	if (Q_UNLIKELY(tagItem.name.isEmpty())) {
		DlgMsgBox::message(parent, QMessageBox::Critical,
		    tr("Tag error"), tr("Tag name is empty."),
		    tr("Tag wasn't created."), QString());
		return false;
	}

	Q_ASSERT(TagItem::isValidColourStr(tagItem.colour));

	if (tagItem.id >= 0) {
		tagDb->updateTag(tagItem.id, tagItem.name, tagItem.colour);
	} else {
		if (!tagDb->insertTag(tagItem.name, tagItem.colour)) {
			DlgMsgBox::message(parent, QMessageBox::Critical,
			    tr("Tag error"),
			    tr("Tag with name '%1'' already exists in database.")
			        .arg(tagItem.name),
			    tr("Tag wasn't created."), QString());
			return false;
		}
	}

	return true;
}
