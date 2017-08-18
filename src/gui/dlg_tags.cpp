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

#include <QString>
#include <QItemSelectionModel>

#include "src/common.h"
#include "src/delegates/tags_delegate.h"
#include "src/gui/dlg_tag.h"
#include "src/gui/dlg_tags.h"
#include "src/io/tag_db.h"
#include "src/models/tags_model.h"
#include "ui_dlg_tags.h"

#define WRONG_TAG_ID -1 /** TODO -- Remove. */

DlgTags::DlgTags(const QString &userName, TagDb *tagDb, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTags),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_msgIdList(),
    m_tagsDelegate(Q_NULLPTR),
    m_tagsModel(Q_NULLPTR),
    m_retCode(NO_ACTION)
{
	m_ui->setupUi(this);
	initDlg();
}

DlgTags::DlgTags(const QString &userName, TagDb *tagDb,
    const QList<qint64> &msgIdList, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTags),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_msgIdList(msgIdList),
    m_tagsDelegate(Q_NULLPTR),
    m_tagsModel(Q_NULLPTR),
    m_retCode(NO_ACTION)
{
	m_ui->setupUi(this);
	initDlg();
	selectAllAssingedTagsFromMsgs();
}

DlgTags::~DlgTags(void)
{
	if (Q_NULLPTR != m_tagsDelegate) {
		delete m_tagsDelegate;
	}
	if (Q_NULLPTR != m_tagsModel) {
		delete m_tagsModel;
	}

	delete m_ui;
}

int DlgTags::exec(void)
{
	QDialog::exec();

	return m_retCode;
}

void DlgTags::addTag(void)
{
	DlgTag::createTag(m_tagDbPtr, this);

	fillTagsToListView();
}

void DlgTags::updateTag(void)
{
	TagItem tagItem(m_tagDbPtr->getTagData(
	    getTagIdFromIndex(
	        m_ui->tagListView->selectionModel()->currentIndex())));

	if (DlgTag::editTag(m_tagDbPtr, tagItem, this)) {
		/* Existing tag has very likely just been changed. */
		m_retCode = TAGS_CHANGED;
	}

	fillTagsToListView();
}

void DlgTags::deleteTag(void)
{
	QModelIndexList slctIdxs(
	    m_ui->tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	foreach (const QModelIndex &idx, slctIdxs) {
		m_tagDbPtr->deleteTag(getTagIdFromIndex(idx));
	}

	/* Existing tags have been removed. */
	m_retCode = TAGS_CHANGED;

	fillTagsToListView();
}

void DlgTags::assignSelectedTagsToMsgs(void)
{
	QModelIndexList slctIdxs(
	    m_ui->tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const qint64 &msgId, m_msgIdList) {
		foreach (const QModelIndex &idx, slctIdxs) {
			m_tagDbPtr->assignTagToMsg(m_userName,
			    getTagIdFromIndex(idx), msgId);
		}
	}

	/* Tag assignment was changed. */
	if (m_retCode != TAGS_CHANGED) {
		m_retCode = ASSIGMENT_CHANGED;
	}
}

void DlgTags::removeSelectedTagsFromMsgs(void)
{
	QModelIndexList slctIdxs(
	    m_ui->tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	Q_ASSERT(!m_userName.isEmpty());

	foreach (const qint64 &msgId, m_msgIdList) {
		foreach (const QModelIndex &idx, slctIdxs) {
			m_tagDbPtr->removeTagFromMsg(m_userName,
			    getTagIdFromIndex(idx), msgId);
		}
	}

	/* Tag assignment was changed. */
	if (m_retCode != TAGS_CHANGED) {
		m_retCode = ASSIGMENT_CHANGED;
	}
}

void DlgTags::removeAllTagsFromMsgs(void)
{
	Q_ASSERT(!m_userName.isEmpty());

	foreach (const qint64 &msgId, m_msgIdList) {
		m_tagDbPtr->removeAllTagsFromMsg(m_userName, msgId);
	}

	/* Tag assignment was changed. */
	if (m_retCode != TAGS_CHANGED) {
		m_retCode = ASSIGMENT_CHANGED;
	}
}

void DlgTags::handleSelectionChanged(void)
{
	QModelIndexList slctIdxs(
	    m_ui->tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		m_ui->pushButtonUpdate->setEnabled(false);
		m_ui->pushButtonDelete->setEnabled(false);
		m_ui->pushButtonRemove->setEnabled(false);
		m_ui->pushButtonAssign->setEnabled(false);
	} else {
		if (slctIdxs.count() > 1) {
			m_ui->pushButtonUpdate->setEnabled(false);
		} else {
			m_ui->pushButtonUpdate->setEnabled(true);
		}
		m_ui->pushButtonDelete->setEnabled(true);
		m_ui->pushButtonRemove->setEnabled(true);
		m_ui->pushButtonAssign->setEnabled(true);
	}
}

void DlgTags::fillTagsToListView(void)
{
	TagItemList tagList(m_tagDbPtr->getAllTags());
	tagList.sortNames();

	m_tagsModel->setTagList(tagList);
}

void DlgTags::initDlg(void)
{
	m_tagsDelegate = new TagsDelegate(this);
	m_tagsModel = new TagsModel(this);

	m_ui->tagListView->setItemDelegate(m_tagsDelegate);
	m_ui->tagListView->setModel(m_tagsModel);

	m_ui->tagListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_ui->tagListView->setSelectionBehavior(QAbstractItemView::SelectRows);

	m_ui->pushButtonUpdate->setEnabled(false);
	m_ui->pushButtonDelete->setEnabled(false);
	m_ui->pushButtonRemove->setEnabled(false);
	m_ui->pushButtonAssign->setEnabled(false);

	m_ui->tagAssignGroup->setEnabled(false);
	m_ui->tagAssignGroup->setVisible(false);

	connect(m_ui->pushButtonAdd, SIGNAL(clicked()), this,
	    SLOT(addTag()));
	connect(m_ui->pushButtonUpdate, SIGNAL(clicked()), this,
	    SLOT(updateTag()));
	connect(m_ui->pushButtonDelete, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));
	connect(m_ui->tagListView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(handleSelectionChanged()));


	/* any messages was selected */
	if (!m_msgIdList.isEmpty()) {

		connect(m_ui->pushButtonAssign, SIGNAL(clicked()), this,
		    SLOT(assignSelectedTagsToMsgs()));
		connect(m_ui->pushButtonRemove, SIGNAL(clicked()), this,
		    SLOT(removeSelectedTagsFromMsgs()));
		connect(m_ui->pushButtonRemoveAll, SIGNAL(clicked()), this,
		    SLOT(removeAllTagsFromMsgs()));

		m_ui->tagAssignGroup->setVisible(true);
		m_ui->tagAssignGroup->setEnabled(true);
	}

	fillTagsToListView();
}

int DlgTags::getTagIdFromIndex(const QModelIndex &idx)
{
	if (!idx.isValid()) {
		return WRONG_TAG_ID;
	}

	if (!idx.data().canConvert<TagItem>()) {
		return WRONG_TAG_ID;
	}

	TagItem tagItem(qvariant_cast<TagItem>(idx.data()));

	return tagItem.id;
}

void DlgTags::selectAllAssingedTagsFromMsgs(void)
{
	int rows = m_ui->tagListView->model()->rowCount();
	for (int i = 0; i < rows; ++i) {
		const QModelIndex idx = m_ui->tagListView->model()->index(i, 0);
		const qint64 id = getTagIdFromIndex(idx);
		foreach (const qint64 &msgId, m_msgIdList) {
			const TagItemList tags =
			    m_tagDbPtr->getMessageTags(m_userName, msgId);
			foreach (const TagItem &tag, tags) {
				if (tag.id == id) {
					m_ui->tagListView->selectionModel()->select(
					    idx, QItemSelectionModel::Select);
				}
			}
		}
	}
}
