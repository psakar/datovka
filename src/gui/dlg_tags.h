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

#ifndef _DLG_TAGS_H_
#define _DLG_TAGS_H_

#include <QDialog>
#include <QList>

#include "ui_dlg_tags.h"

/*!
 * @brief Tags management dialogue.
 */
class DlgTags : public QDialog , public Ui::TagsDialog {
    Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTags(QWidget *parent = 0);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] msgIdList List of message ids.
	 * @param[in] parent    Parent widget.
	 */
	explicit DlgTags(const QList<qint64> &msgIdList, QWidget *parent = 0);

	/*!
	 * @brief Destructor.
	 */
	~DlgTags(void);

private slots:
	/*!
	 * @brief Add tag (insert into database).
	 */
	void addTag(void);

	/*!
	 * @brief Update tag (update data in database).
	 */
	void updateTag(void);

	/*!
	 * @brief Delete tag (delete tag data from database).
	 */
	void deleteTag(void);

	/*!
	 * @brief Assign selected tag(s) to messages (insert into database).
	 */
	void assignSelectedTagsToMsgs(void);

	/*!
	 * @brief Remove selected tag(s) from messages
	 *        (delete records from database).
	 */
	void removeSelectedTagsFromMsgs(void);

	/*!
	 * @brief Active/deactive tag buttons when selection model is changed.
	 */
	void handleSelectionChanged(QItemSelection current);

private:
	/*!
	 * @brief Fill all tags to table view from database.
	 */
	void fillTagsToListView(void);

	/*!
	 * @brief Initialises the dialogue.
	 */
	void initDlg(void);

	/*!
	 * @brief Get tag id from index.
	 *
	 * @return Tag id if success else -1.
	 */
	int getTagIdFromIndex(const QModelIndex &idx);

	QList<qint64> m_msgIdList; /*!< List of message identifiers. */
	class TagsDelegate *m_tagsDelegate; /*!< Responsible for painting. */
	class TagsModel *m_tagsModel; /*!< Tags model. */
};

#endif /* _DLG_TAGS_H_ */
