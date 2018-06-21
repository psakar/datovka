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
#include <QString>

#include "src/delegates/tag_item.h"

class TagDb; /* Forward declaration. */

namespace Ui {
	class DlgTag;
}

/*!
 * @brief Create new tag dialogue.
 */
class DlgTag : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] tag Tag to be modified.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTag(const TagItem &tag, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgTag(void);

	/*!
	 * @brief Create new tag.
	 *
	 * @param[in] tagDb Tag database.
	 * @param[in] parent Parent widget.
	 */
	static
	bool createTag(TagDb *tagDb, QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Edit existing tag.
	 *
	 * @param[in] tagDb Tag database.
	 * @param[in] tag Tag to be modified.
	 * @param[in] parent Parent widget.
	 * @return True when tag has been changed.
	 */
	static
	bool editTag(TagDb *tagDb, const TagItem &tag,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Sets window elements according to tag name.
	 *
	 * @param[in] tagName Tag name.
	 */
	void tagNameChanged(const QString &tagName);

	/*!
	 * @brief Choose or change tag colour.
	 */
	void chooseNewColor(void);

private:
	/*!
	 * @brief Set actual tag colour on the preview button.
	 */
	void setPreviewButtonColor(void);

	/*!
	 * @brief Insert or update tag data into database.
	 *
	 * @param[in] tagDb Tag database.
	 * @param[in] tagItem Tag to be saved.
	 * @param[in] parent Parent widget.
	 * @return True when tag has been saved.
	 */
	static
	bool saveTag(TagDb *tagDb, const TagItem &tagItem,
	    QWidget *parent = Q_NULLPTR);

	Ui::DlgTag *m_ui; /*!< UI generated from UI file. */

	TagItem m_tagItem; /*!< Created tag. */
};
