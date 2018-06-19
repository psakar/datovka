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

namespace Ui {
	class DlgTimestampExpir;
}

/*!
 * @brief Shows question about time stamp expiration check.
 */
class DlgTimestampExpir : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Specifies the selected action.
	 */
	enum Action {
		CHECK_NOTHING, /*!< Nothing to be preformed, action cancelled. */
		CHECK_SELECTED_ACNT, /*!< Check selected account. */
		CHECK_ALL_ACNTS, /*!< Check all accounts. */
		CHECK_DIR, /*!< Check directory. */
		CHECK_DIR_SUB /*!< Check directory including its subdirectories. */
	};

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTimestampExpir(QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgTimestampExpir(void);

	static
	enum Action askAction(QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Fired on radio box selection change.
	 */
	void radioSelectionChanged(void);

	/*!
	 * @brief Converts selection to chosen action.
	 *
	 * @return Chosen action.
	 */
	enum Action collectAction(void) const;

private:
	Ui::DlgTimestampExpir *m_ui; /*!< UI generated from UI file. */
};
