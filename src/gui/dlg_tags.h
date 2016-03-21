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


#ifndef DLG_TAGS_H
#define DLG_TAGS_H

#include <QDialog>

#define WRONG_TAG_ID -1

namespace Ui {
	class TagsDialog;
}

class TagsDialog : public QDialog
{
	Q_OBJECT

public:

	TagsDialog(QWidget *parent = 0);
	TagsDialog(QList<qint64> & msgIdList, QWidget *parent = 0);
	~TagsDialog(void);

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

private:

	/*!
	 * @brief Fill all tags to tableview from database.
	 */
	void fillTagsToListView(void);

	/*!
	 * @brief Init tags dialog.
	 */
	void initTagsDialog(void);

	/*!
	 * @brief Get tag id from selected item (current index).
	 *
	 * @return Tag id if success else -1.
	 */
	int getTagIdFromCurrentIndex(void);

	QList<qint64> m_msgIdList;
	Ui::TagsDialog *ui;
};

#endif // DLG_TAGS_H
