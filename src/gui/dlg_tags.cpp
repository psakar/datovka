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

#include <QStrint>

#include "src/gui/dlg_tag.h"
#include "src/gui/dlg_tags.h"
#include "src/io/tag_db.h"


DlgTags::DlgTags(QWidget *parent)
    : QDialog(parent),
    m_msgIdList()
{
	setupUi(this);

	initDlg();
}

DlgTags::DlgTags(const QList<qint64> &msgIdList, QWidget *parent)
    : QDialog(parent),
    m_msgIdList(msgIdList)
{
	setupUi(this);

	initDlg();
}

void DlgTags::addTag(void)
{
	QDialog *tagDlg = new DlgTag(this);
	tagDlg->exec();
	tagDlg->deleteLater();

	fillTagsToListView();
}

void DlgTags::updateTag(void)
{
	int tagId = getTagIdFromCurrentIndex();

	TagItem tagItem(globTagDbPtr->getTagData(tagId));

	QDialog *tagDlg = new DlgTag(tagId, tagItem.name, tagItem.colour,
	    this);
	tagDlg->exec();
	tagDlg->deleteLater();

	fillTagsToListView();
}

void DlgTags::deleteTag(void)
{
	globTagDbPtr->deleteTag(getTagIdFromCurrentIndex());

	fillTagsToListView();
}

void DlgTags::assignSelectedTagsToMsgs(void)
{
	int tagId = getTagIdFromCurrentIndex();

	foreach (const qint64 &msgId, m_msgIdList) {
		globTagDbPtr->assignTagToMsg(tagId, msgId);
	}
}

void DlgTags::removeSelectedTagsFromMsgs(void)
{
	int tagId = getTagIdFromCurrentIndex();

	foreach (const qint64 &msgId, m_msgIdList) {
		globTagDbPtr->removeTagFromMsg(tagId, msgId);
	}
}

void DlgTags::fillTagsToListView(void)
{
	QList<TagItem> tagList = globTagDbPtr->getAllTags();

	this->tagTableWidget->clearContents();
	this->tagTableWidget->setRowCount(0);

	foreach (const TagItem &tagItem, tagList) {

		int row = this->tagTableWidget->rowCount();
		this->tagTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(tagItem.name);
		item->setForeground(QColor("#"+ tagItem.colour));
		this->tagTableWidget->setItem(row, 0 , item);
		item = new QTableWidgetItem;
		item->setText(QString::number(tagItem.id));
		this->tagTableWidget->setItem(row, 1 ,item);
	}

	if (this->tagTableWidget->rowCount() > 0) {
		this->tagTableWidget->selectColumn(0);
		this->tagTableWidget->selectRow(0);
	}
}

void DlgTags::initDlg(void)
{
	this->tagAssignGroup->setEnabled(false);
	this->tagAssignGroup->setVisible(false);

	connect(this->pushButtonAdd, SIGNAL(clicked()), this,
	    SLOT(addTag()));
	connect(this->pushButtonUpdate, SIGNAL(clicked()), this,
	    SLOT(updateTag()));
	connect(this->pushButtonDelete, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));

	this->tagTableWidget->setColumnHidden(1, true);

	/* any messages was selected */
	if (!m_msgIdList.isEmpty()) {

		connect(this->pushButtonAssign, SIGNAL(clicked()), this,
		    SLOT(assignSelectedTagsToMsgs()));
		connect(this->pushButtonRemove, SIGNAL(clicked()), this,
		    SLOT(removeSelectedTagsFromMsgs()));

		this->tagAssignGroup->setVisible(true);
		this->tagAssignGroup->setEnabled(true);
	}

	fillTagsToListView();
}

int DlgTags::getTagIdFromCurrentIndex(void)
{
	QModelIndex currentIndex = this->tagTableWidget->currentIndex();

	if (!currentIndex.isValid()) {
		return WRONG_TAG_ID;
	}

	int row = currentIndex.row();
	return this->tagTableWidget->item(row, 1)->text().toInt();
}
