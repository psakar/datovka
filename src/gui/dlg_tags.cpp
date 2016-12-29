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

#include <QString>
#include <QItemSelectionModel>

#include "src/common.h"
#include "src/delegates/tags_delegate.h"
#include "src/gui/dlg_tag.h"
#include "src/gui/dlg_tags.h"
#include "src/io/tag_db_container.h"
#include "src/models/tags_model.h"

#define WRONG_TAG_ID -1 /** TODO -- Remove. */

DlgTags::DlgTags(const QString &userName, TagDb *tagDb, QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_msgIdList(),
    m_msgIdWebDatovkaList(),
    m_tagsDelegate(Q_NULLPTR),
    m_tagsModel(Q_NULLPTR),
    m_retCode(NO_ACTION),
    m_isWebDatovkaAccount(false)
{
	setupUi(this);
	initDlg();
}

DlgTags::DlgTags(const QString &userName, TagDb *tagDb,
    const QList<qint64> &msgIdList,
    const QList<qint64> &msgIdWebDatovkaList, QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_tagDbPtr(tagDb),
    m_msgIdList(msgIdList),
    m_msgIdWebDatovkaList(msgIdWebDatovkaList),
    m_tagsDelegate(Q_NULLPTR),
    m_tagsModel(Q_NULLPTR),
    m_retCode(NO_ACTION),
    m_isWebDatovkaAccount(false)
{
	setupUi(this);
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
}

int DlgTags::exec(void)
{
	QDialog::exec();

	return m_retCode;
}

void DlgTags::addTag(void)
{
	QDialog *tagDlg = new DlgTag(m_userName, m_tagDbPtr,
	    m_isWebDatovkaAccount, this);
	tagDlg->exec();
	tagDlg->deleteLater();

	fillTagsToListView();
}

void DlgTags::updateTag(void)
{
	TagItem tagItem(m_tagDbPtr->getTagData(
	    getTagIdFromIndex(tagListView->selectionModel()->currentIndex())));

	QDialog *tagDlg = new DlgTag(m_userName, m_tagDbPtr,
	    m_isWebDatovkaAccount, tagItem, this);
	int retVal = tagDlg->exec();
	tagDlg->deleteLater();

	if (retVal == QDialog::Accepted) {
		/* Existing tag has very likely just been changed. */
		m_retCode = TAGS_CHANGED;
	}

	fillTagsToListView();
}

void DlgTags::deleteTag(void)
{
	QModelIndexList slctIdxs(tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	foreach (const QModelIndex &idx, slctIdxs) {

		if (m_isWebDatovkaAccount) {
			if (!m_jsonsLayer.deleteTag(m_userName,
			    getTagIdFromIndex(idx), m_errStr)) {
				continue;
			}
		}
		m_tagDbPtr->deleteTag(getTagIdFromIndex(idx));
	}

	/* Existing tags have been removed. */
	m_retCode = TAGS_CHANGED;

	fillTagsToListView();
}

void DlgTags::assignSelectedTagsToMsgs(void)
{
	QModelIndexList slctIdxs(tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	Q_ASSERT(!m_userName.isEmpty());

	if (m_isWebDatovkaAccount) {
		if (m_msgIdList.count() != m_msgIdWebDatovkaList.count()) {
			return;
		}
		for (int i = 0; i < m_msgIdList.count(); ++i) {
			foreach (const QModelIndex &idx, slctIdxs) {
				if (!m_jsonsLayer.assignTag(m_userName,
				    getTagIdFromIndex(idx),
				    m_msgIdWebDatovkaList.at(i), m_errStr)) {
					continue;
				}
				m_tagDbPtr->assignTagToMsg(m_userName,
				    getTagIdFromIndex(idx),
				    m_msgIdList.at(i));
			}

		}
	} else {
		foreach (const qint64 &msgId, m_msgIdList) {
			foreach (const QModelIndex &idx, slctIdxs) {
				m_tagDbPtr->assignTagToMsg(m_userName,
				    getTagIdFromIndex(idx), msgId);
			}
		}
	}

	/* Tag assignment was changed. */
	if (m_retCode != TAGS_CHANGED) {
		m_retCode = ASSIGMENT_CHANGED;
	}
}

void DlgTags::removeSelectedTagsFromMsgs(void)
{
	QModelIndexList slctIdxs(tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		/* Nothing to do. */
		return;
	}

	Q_ASSERT(!m_userName.isEmpty());

	if (m_isWebDatovkaAccount) {
		if (m_msgIdList.count() != m_msgIdWebDatovkaList.count()) {
			return;
		}
		for (int i = 0; i < m_msgIdList.count(); ++i) {
			foreach (const QModelIndex &idx, slctIdxs) {
				if (!m_jsonsLayer.removeTag(m_userName,
				    getTagIdFromIndex(idx),
				    m_msgIdWebDatovkaList.at(i), m_errStr)) {
					continue;
				}
				m_tagDbPtr->removeTagFromMsg(m_userName,
				    getTagIdFromIndex(idx), m_msgIdList.at(i));
			}

		}
	} else {
		foreach (const qint64 &msgId, m_msgIdList) {
			foreach (const QModelIndex &idx, slctIdxs) {
				m_tagDbPtr->removeTagFromMsg(m_userName,
				    getTagIdFromIndex(idx), msgId);
			}
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

	if (m_isWebDatovkaAccount) {
		if (m_msgIdList.count() != m_msgIdWebDatovkaList.count()) {
			return;
		}
		for (int i = 0; i < m_msgIdList.count(); ++i) {
			if (!m_jsonsLayer.removeAllTags(m_userName,
			   m_msgIdWebDatovkaList.at(i), m_errStr)) {
				continue;
			}
			m_tagDbPtr->removeAllTagsFromMsg(m_userName,
			    m_msgIdList.at(i));
		}
	} else {
		foreach (const qint64 &msgId, m_msgIdList) {
			m_tagDbPtr->removeAllTagsFromMsg(m_userName, msgId);
		}
	}

	/* Tag assignment was changed. */
	if (m_retCode != TAGS_CHANGED) {
		m_retCode = ASSIGMENT_CHANGED;
	}
}

void DlgTags::handleSelectionChanged(void)
{
	QModelIndexList slctIdxs(tagListView->selectionModel()->selectedRows());

	if (slctIdxs.isEmpty()) {
		this->pushButtonUpdate->setEnabled(false);
		this->pushButtonDelete->setEnabled(false);
		this->pushButtonRemove->setEnabled(false);
		this->pushButtonAssign->setEnabled(false);
	} else {
		if (slctIdxs.count() > 1) {
			this->pushButtonUpdate->setEnabled(false);
		} else {
			this->pushButtonUpdate->setEnabled(true);
		}
		this->pushButtonDelete->setEnabled(true);
		this->pushButtonRemove->setEnabled(true);
		this->pushButtonAssign->setEnabled(true);
	}
}

void DlgTags::fillTagsToListView(void)
{
	TagItemList tagList(m_tagDbPtr->getAllTags());

	m_tagsModel->setTagList(tagList);
}

void DlgTags::initDlg(void)
{
	if (isWebDatovkaAccount(m_userName)) {
		m_isWebDatovkaAccount = true;
	} else {
		m_isWebDatovkaAccount = false;
	}

	m_tagsDelegate = new TagsDelegate(this);
	m_tagsModel = new TagsModel(this);

	tagListView->setItemDelegate(m_tagsDelegate);
	tagListView->setModel(m_tagsModel);

	tagListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tagListView->setSelectionBehavior(QAbstractItemView::SelectRows);

	this->pushButtonUpdate->setEnabled(false);
	this->pushButtonDelete->setEnabled(false);
	this->pushButtonRemove->setEnabled(false);
	this->pushButtonAssign->setEnabled(false);

	this->tagAssignGroup->setEnabled(false);
	this->tagAssignGroup->setVisible(false);

	connect(this->pushButtonAdd, SIGNAL(clicked()), this,
	    SLOT(addTag()));
	connect(this->pushButtonUpdate, SIGNAL(clicked()), this,
	    SLOT(updateTag()));
	connect(this->pushButtonDelete, SIGNAL(clicked()), this,
	    SLOT(deleteTag()));
	connect(tagListView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
	    SLOT(handleSelectionChanged()));


	/* any messages was selected */
	if (!m_msgIdList.isEmpty()) {

		connect(this->pushButtonAssign, SIGNAL(clicked()), this,
		    SLOT(assignSelectedTagsToMsgs()));
		connect(this->pushButtonRemove, SIGNAL(clicked()), this,
		    SLOT(removeSelectedTagsFromMsgs()));
		connect(this->pushButtonRemoveAll, SIGNAL(clicked()), this,
		    SLOT(removeAllTagsFromMsgs()));

		this->tagAssignGroup->setVisible(true);
		this->tagAssignGroup->setEnabled(true);
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
	int rows = tagListView->model()->rowCount();
	for (int i = 0; i < rows; ++i) {
		const QModelIndex idx = tagListView->model()->index(i, 0);
		const qint64 id = getTagIdFromIndex(idx);
		foreach (const qint64 &msgId, m_msgIdList) {
			const TagItemList tags =
			    m_tagDbPtr->getMessageTags(m_userName, msgId);
			foreach (const TagItem &tag, tags) {
				if (tag.id == id) {
					tagListView->selectionModel()->select(
					    idx, QItemSelectionModel::Select);
				}
			}
		}
	}
}
