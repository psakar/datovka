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

	ui->tagTableWidget->setColumnHidden(1, true);

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
	int row = ui->tagTableWidget->currentIndex().row();
	int id = ui->tagTableWidget->item(row, 1)->text().toInt();

	TagItem tagItem = globTagDbPtr->getTagData(id);

	QDialog *tagDialog = new TagDialog(id, tagItem.tagName,
	    tagItem.tagColor, this);
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
	int row = ui->tagTableWidget->currentIndex().row();
	int id = ui->tagTableWidget->item(row, 1)->text().toInt();

	globTagDbPtr->deleteTag(id);
}


/* ========================================================================= */
/*
 * Fill tags to list view.
 */
void TagsDialog::fillTagsToListView(void)
/* ========================================================================= */
{
	QList<TagItem> tagList = globTagDbPtr->getAllTags();

	ui->tagTableWidget->setRowCount(0);

	for (int i = 0; i < tagList.count(); ++i) {

		int row = ui->tagTableWidget->rowCount();
		ui->tagTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(tagList.at(i).tagName);
		item->setForeground(QColor("#"+ tagList.at(i).tagColor));
		ui->tagTableWidget->setItem(row, 0 , item);
		item = new QTableWidgetItem;
		item->setText(QString::number(tagList.at(i).id));
		ui->tagTableWidget->setItem(row, 1 ,item);
	}

	if (ui->tagTableWidget->rowCount() > 0) {
		ui->tagTableWidget->selectColumn(0);
		ui->tagTableWidget->selectRow(0);
	}
}
