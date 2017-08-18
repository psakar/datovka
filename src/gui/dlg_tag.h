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

#ifndef _DLG_TAG_H_
#define _DLG_TAG_H_

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
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Tag database.
	 * @param[in] tag Tag to be modified.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTag(const QString &userName, TagDb *tagDb,
	    const TagItem &tag, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgTag(void);

	/*!
	 * @brief Create new tag.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Tag database.
	 * @param[in] parent Parent widget.
	 */
	static
	void createTag(const QString &userName, TagDb *tagDb,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Edit existing tag.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] tagDb Tag database.
	 * @param[in] tag Tag to be modified.
	 * @param[in] parent Parent widget.
	 * @return True when tag has been changed.
	 */
	static
	bool editTag(const QString &userName, TagDb *tagDb, const TagItem &tag,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Insert or update tag data into database.
	 */
	void saveTag(void);

	/*!
	 * @brief Choose or change tag colour.
	 */
	void chooseNewColor(void);

private:
	/*!
	 * @brief Initialises new tag dialogue.
	 */
	void initDlg(void);

	/*!
	 * @brief Set actual tag colour on the preview button.
	 */
	void setPreviewButtonColor(void);

	Ui::DlgTag *m_ui; /*!< UI generated from UI file. */

	const QString m_userName; /*!< Account username. */
	TagDb *m_tagDbPtr; /*!< Tag db pointer. */
	TagItem m_tagItem; /*!< Created tag. */
};

#endif /* _DLG_TAG_H_ */
