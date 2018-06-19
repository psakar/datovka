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

#pragma once

#include <QDialog>
#include <QList>

#include "src/delegates/tags_delegate.h"
#include "src/models/tags_model.h"

class TagDb; /* Forward declaration. */

namespace Ui {
	class DlgTags;
}

/*!
 * @brief Tags management dialogue.
 */
class DlgTags : public QDialog {
	Q_OBJECT

public:
	/*!
	 * Describes the action the dialogue has performed.
	 */
	enum ReturnCode {
		NO_ACTION, /*!< Nothing happened. */
		ASSIGMENT_CHANGED, /*!< Assignment of tags to messages have changed. */
		TAGS_CHANGED /*!< Actual tags have been deleted or changed. */
	};

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Pointer to tag database.
	 * @param[in] msgIdList List of message ids, empty list if no assignment
	 *                      should be edited.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTags(const QString &userName, TagDb *tagDb,
	    const QList<qint64> &msgIdList, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgTags(void);

	/*!
	 * @brief Edit all tags.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Pointer to tag database.
	 * @param[in] parent Parent widget.
	 * @return Return code.
	 */
	static
	enum ReturnCode editAvailable(const QString &userName, TagDb *tagDb,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Edit assigned tags.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Pointer to tag database.
	 * @param[in] msgIdList List of message ids.
	 * @param[in] parent Parent widget.
	 * @return Return code.
	 */
	static
	enum ReturnCode editAssignment(const QString &userName, TagDb *tagDb,
	    const QList<qint64> &msgIdList, QWidget *parent = Q_NULLPTR);

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
	 *        (delete message tag records from database).
	 */
	void removeSelectedTagsFromMsgs(void);

	/*!
	 * @brief Remove all tags from messages
	 *        (delete message tag records from database).
	 */
	void removeAllTagsFromMsgs(void);

	/*!
	 * @brief Activate/deactivate buttons on available selection change.
	 */
	void handleAvailableSelectionChange(void);

	/*!
	 * @brief Activate/deactivate buttons on assigned selection change.
	 */
	void handleAssignedSelectionChange(void);

private:
	/*!
	 * @brief Fill all tag entries into list views from database.
	 */
	void fillTagsToListViews(void);

	/*!
	 * @brief Initialises the dialogue.
	 */
	void initDlg(void);

	/*!
	 * @brief Choose (select) all tags in the list view
	 *        which are assigned in selected messages.
	 */
	void selectAllAssingedTagsFromMsgs(void);

	Ui::DlgTags *m_ui; /*!< UI generated from UI file. */

	const QString m_userName; /*!< Account username. */
	TagDb *m_tagDbPtr; /*!< Tag db pointer. */
	const QList<qint64> m_msgIdList; /*!< List of message identifiers. */

	TagsDelegate m_availableTagsDelegate; /*!< Responsible for painting. */
	TagsModel m_availableTagsModel; /*!< Available tags model. */

	TagsDelegate m_assignedTagsDelegate; /*!< Responsible for painting. */
	TagsModel m_assignedTagsModel; /*!< Assigned tags model. */

	enum ReturnCode m_retCode; /*!< Dialogue return code. */
};
