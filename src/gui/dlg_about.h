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

#ifndef _DLG_ABOUT_H_
#define _DLG_ABOUT_H_

#include <QDialog>

namespace Ui {
	class DlgAbout;
}

/*!
 * @brief About dialogue.
 */
class DlgAbout : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit DlgAbout(QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgAbout(void);

	/*!
	 * @brief View about dialogue.
	 *
	 * @param[in] parent Window parent widget.
	 */
	static
	void about(QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Loads the license file into the text field.
	 */
	void showLicence(void);

	/*!
	 * @brief Displays credits information in the text field.
	 */
	void showCredits(void);

private:
	Ui::DlgAbout *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_ABOUT_H_ */
