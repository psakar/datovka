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
      m_msgIdList(QList<qint64>()),
      ui(new Ui::TagsDialog)
{
	ui->setupUi(this);
	initTagsDialog();
}


/* ========================================================================= */
/*
 * Constructor.
 */
TagsDialog::TagsDialog(QList<qint64> & msgIdList, QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
      m_msgIdList(msgIdList),
      ui(new Ui::TagsDialog)
{
	ui->setupUi(this);
	initTagsDialog();
}


/* ========================================================================= */
/*
 * Destructor.
 */
TagsDialog::~TagsDialog(void)
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

	fillTagsToListView();
}


/* ========================================================================= */
/*
 * Update selected tag.
 */
void TagsDialog::updateTag(void)
/* ========================================================================= */
{
	int tagId = getTagIdFromCurrentIndex();

	TagItem tagItem(globTagDbPtr->getTagData(tagId));

	QDialog *tagDialog = new TagDialog(tagId, tagItem.name, tagItem.colour,
	    this);
	tagDialog->exec();

	delete tagDialog;

	fillTagsToListView();
}


/* ========================================================================= */
/*
 * Delete tag.
 */
void TagsDialog::deleteTag(void)
/* ========================================================================= */
{
	globTagDbPtr->deleteTag(getTagIdFromCurrentIndex());

	fillTagsToListView();
}


/* ========================================================================= */
/*
 * Assign selected tag to messages.
 */
void TagsDialog::assignSelectedTagsToMsgs(void)
/* ========================================================================= */
{
	int tagId = getTagIdFromCurrentIndex();

	foreach (const qint64 &msgId, m_msgIdList) {
		globTagDbPtr->assignTagToMsg(tagId, msgId);
	}
}


/* ========================================================================= */
/*
 * Remove selected tag from messages.
 */
void TagsDialog::removeSelectedTagsFromMsgs(void)
/* ========================================================================= */
{
	int tagId = getTagIdFromCurrentIndex();

	foreach (const qint64 &msgId, m_msgIdList) {
		globTagDbPtr->removeTagFromMsg(tagId, msgId);
	}
}


/* ========================================================================= */
/*
 * Fill tags to list view.
 */
void TagsDialog::fillTagsToListView(void)
/* ========================================================================= */
{
	QList<TagItem> tagList = globTagDbPtr->getAllTags();

	ui->tagTableWidget->clearContents();
	ui->tagTableWidget->setRowCount(0);

	foreach (const TagItem &tagItem, tagList) {

		int row = ui->tagTableWidget->rowCount();
		ui->tagTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(tagItem.name);
		item->setForeground(QColor("#"+ tagItem.colour));
		ui->tagTableWidget->setItem(row, 0 , item);
		item = new QTableWidgetItem;
		item->setText(QString::number(tagItem.id));
		ui->tagTableWidget->setItem(row, 1 ,item);
	}

	if (ui->tagTableWidget->rowCount() > 0) {
		ui->tagTableWidget->selectColumn(0);
		ui->tagTableWidget->selectRow(0);
	}
}


/* ========================================================================= */
/*
 * Initialize dialog.
 */
void TagsDialog::initTagsDialog(void)
/* ========================================================================= */
{
	ui->tagAssignGroup->setEnabled(false);
	ui->tagAssignGroup->setVisible(false);

	connect(ui->pushButtonAdd, SIGNAL(clicked()), this,
	    SLOT(addTag()));
	connect(ui->pushButtonUpdate, SIGNAL(clicked()), this,
	    SLOT(updateTag()));
	connect(ui->pushButtonDelete, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));

	ui->tagTableWidget->setColumnHidden(1, true);

	/* any messages was selected */
	if (!m_msgIdList.isEmpty()) {

		connect(ui->pushButtonAssign, SIGNAL(clicked()), this,
		    SLOT(assignSelectedTagsToMsgs()));
		connect(ui->pushButtonRemove, SIGNAL(clicked()), this,
		    SLOT(removeSelectedTagsFromMsgs()));

		ui->tagAssignGroup->setVisible(true);
		ui->tagAssignGroup->setEnabled(true);
	}

	fillTagsToListView();
}


/* ========================================================================= */
/*
 * Get tag id from selected item (current index).
 */
int TagsDialog::getTagIdFromCurrentIndex(void)
/* ========================================================================= */
{
	QModelIndex currentIndex = ui->tagTableWidget->currentIndex();

	if (!currentIndex.isValid()) {
		return WRONG_TAG_ID;
	}

	int row = currentIndex.row();
	return ui->tagTableWidget->item(row, 1)->text().toInt();
}
