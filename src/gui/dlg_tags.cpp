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

#include <QDialog>
#include <QDebug>

#include "dlg_tags.h"
#include "dlg_tag.h"
#include "ui_dlg_tags.h"
#include "src/io/tag_db.h"


/* ========================================================================= */
/*
 * Constructor.
 */
TagsDialog::TagsDialog(QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
      ui(new Ui::TagsDialog)
{
	ui->setupUi(this);

	connect(ui->pushButtonAdd, SIGNAL(clicked()), this,
	    SLOT(addTag()));
	connect(ui->pushButtonUpdate, SIGNAL(clicked()), this,
	    SLOT(updateTag()));
	connect(ui->pushButtonDelete, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));

	fillTagsToListView();

}


/* ========================================================================= */
/*
 * Destructor.
 */
TagsDialog::~TagsDialog()
/* ========================================================================= */
{
	delete ui;
}


/* ========================================================================= */
/*
 * Add a new tag.
 */
void TagsDialog::addTag(void)
/* ========================================================================= */
{
	QDialog *tagDialog = new TagDialog(this);
	tagDialog->exec();

	delete tagDialog;
}


/* ========================================================================= */
/*
 * Update selected tag.
 */
void TagsDialog::updateTag(void)
/* ========================================================================= */
{
	/* TODO - get id and data of tag from database */
	int id = ui->tagListWidget->currentIndex().row();
	QString tagName;
	QString tagColor;

	QDialog *tagDialog = new TagDialog(id, tagName, tagColor, this);
	tagDialog->exec();

	delete tagDialog;
}


/* ========================================================================= */
/*
 * Delete tag.
 */
void TagsDialog::deleteTag(void)
/* ========================================================================= */
{
	/* TODO - get database id of tag */
	int id = ui->tagListWidget->currentIndex().row();
	globTagDbPtr->deleteTag(id);
}


/* ========================================================================= */
/*
 * Fill tags to list view.
 */
void TagsDialog::fillTagsToListView(void)
/* ========================================================================= */
{
}
