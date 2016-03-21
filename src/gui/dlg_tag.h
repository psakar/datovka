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


#ifndef DLG_TAG_H
#define DLG_TAG_H

#define NEWTAG_ID -1
/* default colour: 6 chars, hex format, RGB, lower characters */
#define NEWTAG_COLOR "ffffff"

#include <QDialog>

namespace Ui {
	class TagDialog;
}

class TagDialog : public QDialog
{
	Q_OBJECT

public:

	TagDialog(QWidget *parent = 0);
	TagDialog(int tagId = NEWTAG_ID, QString tagName = QString(),
	    QString tagColor = NEWTAG_COLOR, QWidget *parent = 0);
	~TagDialog();

private slots:

	/*!
	 * @brief Insert or update tag data into database.
	 */
	void saveTag(void);

	/*!
	 * @brief Choose or change tag color.
	 */
	void chooseNewColor(void);

private:

	/*!
	 * @brief Init new tag dialog.
	 */
	void initTagDialog(void);

	/*!
	 * @brief Set actual tag color on the preview button.
	 */
	void setPreviewButtonColor(void);

	int m_tagid;
	QString m_tagName;
	QString m_tagColor;
	Ui::TagDialog *ui;
};

#endif // DLG_TAG_H
