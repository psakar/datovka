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


#ifndef _DLG_TAG_H_
#define _DLG_TAG_H_

#include <QDialog>
#include <QString>

#include "src/delegates/tag_item.h"
#include "ui_dlg_tag.h"
#include "src/io/tag_db.h"

/*!
 * @brief Create new tag dialogue.
 */
class DlgTag : public QDialog, public Ui::TagDialog {
    Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName     Account user name.
	 * @param[in] isWebDatovka is Webdatovka account.
	 * @param[in] parent       Parent widget.
	 */
	explicit DlgTag(const QString &userName,
	    bool isWebDatovkaAccount, QWidget *parent = 0);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName     Account user name.
	 * @param[in] isWebDatovka is Webdatovka account.
	 * @param[in] tag          Tag.
	 * @param[in] parent       Parent widget.
	 */
	explicit DlgTag(const QString &userName, bool isWebDatovkaAccount,
	    const TagItem &tag, QWidget *parent = 0);

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

	QString m_userName; /*!< Account username. */
	bool m_isWebDatovkaAccount; /*!< is WebDatovka account. */
	TagItem m_tagItem; /*!< Created tag. */
	TagDb *m_TagDbPtr; /*!< Tag db pointer. */
};

#endif /* _DLG_TAG_H_ */
