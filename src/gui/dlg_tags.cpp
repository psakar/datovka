/*
 * Copyright (C) 2014-2018 CZ.NIC
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
#include "src/gui/dlg_tag.h"
#include "src/gui/dlg_tags.h"
#include "src/io/tag_db.h"
#include "ui_dlg_tags.h"

#define WRONG_TAG_ID -1 /** TODO -- Remove. */

DlgTags::DlgTags(const QString &userName, TagDb *tagDb,
    const QList<qint64> &msgIdList, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgTags),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_msgIdList(msgIdList),
    m_availableTagsDelegate(this),
    m_availableTagsModel(this),
    m_assignedTagsDelegate(this),
    m_assignedTagsModel(this),
    m_retCode(NO_ACTION)
{
	m_ui->setupUi(this);
	/* Tab order is defined in UI file. */

	initDlg();
	fillTagsToListViews();
	//selectAllAssingedTagsFromMsgs();
}

DlgTags::~DlgTags(void)
{
	delete m_ui;
}

enum DlgTags::ReturnCode DlgTags::editAvailable(const QString &userName,
    TagDb *tagDb, QWidget *parent)
{
	if (Q_UNLIKELY(tagDb == Q_NULLPTR)) {
		Q_ASSERT(0);
		return NO_ACTION;
	}

	DlgTags dlg(userName, tagDb, QList<qint64>(), parent);
	dlg.exec();

	return dlg.m_retCode;
}

enum DlgTags::ReturnCode DlgTags::editAssignment(const QString &userName,
    TagDb *tagDb, const QList<qint64> &msgIdList, QWidget *parent)
{
	if (Q_UNLIKELY((tagDb == Q_NULLPTR) || msgIdList.isEmpty())) {
		Q_ASSERT(0);
		return NO_ACTION;
	}

	DlgTags dlg(userName, tagDb, msgIdList, parent);
	dlg.exec();

	return dlg.m_retCode;
}

void DlgTags::addTag(void)
{
	DlgTag::createTag(m_tagDbPtr, this);

	fillTagsToListViews();
}

/*!
 * @brief Get tag id from index.
 *
 * @param[in] idx Model index.
 * @return Tag id if success else -1.
 */
static inline
int getTagIdFromIndex(const QModelIndex &idx)
{
	if (Q_UNLIKELY(!idx.isValid())) {
		return WRONG_TAG_ID;
	}

	if (Q_UNLIKELY(!idx.data().canConvert<TagItem>())) {
		return WRONG_TAG_ID;
	}

	TagItem tagItem(qvariant_cast<TagItem>(idx.data()));

	return tagItem.id;
}

/*!
 * @brief Get selected index.
 *
 * @param[in] view List view.
 * @return Selected index.
 */
static inline
QModelIndex selectedIndex(const QListView *view)
{
	if (Q_UNLIKELY(view == Q_NULLPTR)) {
		Q_ASSERT(0);
		return QModelIndex();
	}

	return view->selectionModel()->currentIndex();
}

void DlgTags::updateTag(void)
{
	TagItem tagItem(m_tagDbPtr->getTagData(
	    getTagIdFromIndex(selectedIndex(m_ui->availableTagsView))));

	if (DlgTag::editTag(m_tagDbPtr, tagItem, this)) {
		/* Existing tag has very likely just been changed. */
		m_retCode = TAGS_CHANGED;
	}

	fillTagsToListViews();
}

/*!
 * @brief Get selected indexes.
 *
 * @param[in] view List view.
 * @return Selected indexes.
 */
static inline
QModelIndexList selectedIndexes(const QListView *view)
{
	if (Q_UNLIKELY(view == Q_NULLPTR)) {
		Q_ASSERT(0);
		return QModelIndexList();
	}

	return view->selectionModel()->selectedRows();
}

void DlgTags::deleteTag(void)
{
	QModelIndexList slctIdxs(selectedIndexes(m_ui->availableTagsView));
	if (Q_UNLIKELY(slctIdxs.isEmpty())) {
		/* Nothing to do. */
		return;
	}

	foreach (const QModelIndex &idx, slctIdxs) {
		m_tagDbPtr->deleteTag(getTagIdFromIndex(idx));
	}

	/* Existing tags have been removed. */
	m_retCode = TAGS_CHANGED;

	fillTagsToListViews();
}

void DlgTags::assignSelectedTagsToMsgs(void)
{
	QModelIndexList slctIdxs(selectedIndexes(m_ui->availableTagsView));
	if (Q_UNLIKELY(slctIdxs.isEmpty())) {
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

	fillTagsToListViews();
}

void DlgTags::removeSelectedTagsFromMsgs(void)
{
	QModelIndexList slctIdxs(selectedIndexes(m_ui->assignedTagsView));
	if (Q_UNLIKELY(slctIdxs.isEmpty())) {
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

	fillTagsToListViews();
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

	fillTagsToListViews();
}

void DlgTags::handleAvailableSelectionChange(void)
{
	QModelIndexList slctIdxs(selectedIndexes(m_ui->availableTagsView));

	m_ui->deleteTagButton->setEnabled(!slctIdxs.isEmpty());
	m_ui->updateTagButton->setEnabled(slctIdxs.count() == 1);
	m_ui->assignButton->setEnabled(!slctIdxs.isEmpty());

	if (!slctIdxs.isEmpty()) {
		m_ui->assignedTagsView->clearSelection();
	}
}

void DlgTags::handleAssignedSelectionChange(void)
{
	QModelIndexList slctIdxs(selectedIndexes(m_ui->assignedTagsView));

	m_ui->removeButton->setEnabled(!slctIdxs.isEmpty());

	if (!slctIdxs.isEmpty()) {
		m_ui->availableTagsView->clearSelection();
	}
}

void DlgTags::fillTagsToListViews(void)
{
	Q_ASSERT(!m_userName.isEmpty());

	/* This should also disable all related buttons. */
	m_ui->availableTagsView->clearSelection();
	m_ui->assignedTagsView->clearSelection();

	/* Get all available tags. */
	QSet<TagDb::TagEntry> availableTags(m_tagDbPtr->getAllTags().toSet());

	/*
	 * Get set of tags assigned to any supplied message and to all supplied
	 * messages.
	 */
	QSet<TagDb::TagEntry> assignedTagsUnion, assignedTagsIntersection;
	bool firstMsg = true;
	foreach (qint64 dmId, m_msgIdList) {
		const QSet<TagDb::TagEntry> msgTagSet(
		    m_tagDbPtr->getMessageTags(m_userName, dmId).toSet());

		assignedTagsUnion += msgTagSet;
		if (firstMsg) {
			assignedTagsIntersection = msgTagSet;
			firstMsg = false;
		} else {
			assignedTagsIntersection.intersect(msgTagSet);
		}
	}

	{
		/*
		 * Don't disable model interaction for tags that are already
		 * assigned to all related messages.
		 */

		TagItemList availableTagList(availableTags.toList());
		availableTagList.sortNames();
		m_availableTagsModel.setTagList(availableTagList);
	}

	{
		TagItemList assignedTagList(assignedTagsUnion.toList());
		assignedTagList.sortNames();
		m_assignedTagsModel.setTagList(assignedTagList);

		m_ui->removeAllButton->setEnabled(assignedTagList.count() > 0);
	}
}

void DlgTags::initDlg(void)
{
	m_ui->availableTagsView->setItemDelegate(&m_availableTagsDelegate);
	m_ui->availableTagsView->setModel(&m_availableTagsModel);
	m_ui->availableTagsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_ui->availableTagsView->setSelectionBehavior(QAbstractItemView::SelectRows);

	m_ui->assignedTagsView->setItemDelegate(&m_assignedTagsDelegate);
	m_ui->assignedTagsView->setModel(&m_assignedTagsModel);
	m_ui->assignedTagsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_ui->assignedTagsView->setSelectionBehavior(QAbstractItemView::SelectRows);

	connect(m_ui->availableTagsView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(handleAvailableSelectionChange()));

	connect(m_ui->assignedTagsView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(handleAssignedSelectionChange()));

	connect(m_ui->addTagButton, SIGNAL(clicked()), this, SLOT(addTag()));
	connect(m_ui->deleteTagButton, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));
	connect(m_ui->updateTagButton, SIGNAL(clicked()), this,
	    SLOT(updateTag()));

	/* Message tag assignment is going to be edited. */
	if (!m_msgIdList.isEmpty()) {
		connect(m_ui->assignButton, SIGNAL(clicked()), this,
		    SLOT(assignSelectedTagsToMsgs()));
		connect(m_ui->removeButton, SIGNAL(clicked()), this,
		    SLOT(removeSelectedTagsFromMsgs()));
		connect(m_ui->removeAllButton, SIGNAL(clicked()), this,
		    SLOT(removeAllTagsFromMsgs()));
	}

	m_ui->deleteTagButton->setEnabled(false);
	m_ui->updateTagButton->setEnabled(false);

	m_ui->assignButton->setEnabled(false);
	m_ui->assignButton->setVisible(!m_msgIdList.isEmpty());
	m_ui->removeButton->setEnabled(false);
	m_ui->removeButton->setVisible(!m_msgIdList.isEmpty());

	m_ui->assignedGroup->setEnabled(!m_msgIdList.isEmpty());
	m_ui->assignedGroup->setVisible(!m_msgIdList.isEmpty());
}

void DlgTags::selectAllAssingedTagsFromMsgs(void)
{
	int rows = m_ui->availableTagsView->model()->rowCount();
	for (int i = 0; i < rows; ++i) {
		const QModelIndex idx = m_ui->availableTagsView->model()->index(i, 0);
		const qint64 id = getTagIdFromIndex(idx);
		foreach (const qint64 &msgId, m_msgIdList) {
			const TagItemList tags =
			    m_tagDbPtr->getMessageTags(m_userName, msgId);
			foreach (const TagItem &tag, tags) {
				if (tag.id == id) {
					m_ui->availableTagsView->selectionModel()->select(
					    idx, QItemSelectionModel::Select);
				}
			}
		}
	}
}
