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

#ifndef _DLG_PIN_INPUT_H_
#define _DLG_PIN_INPUT_H_

#include <QDialog>

#include "src/settings/pin.h"

namespace Ui {
	class DlgPinInput;
}

/*!
 * @brief Asks the user fot the current PIN.
 */
class DlgPinInput : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgPinInput(QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgPinInput(void);

	/*!
	 * @brief Asks the user for PIn value.
	 *
	 * @note Stores PIN in unencrypted form into settings structure.
	 *
	 * @param[in,out] sett PIN settings to be modified.
	 * @param[in] parent Parent widget.
	 * @return True if correct PIN has been entered.
	 */
	static
	bool queryPin(PinSettings &sett, QWidget *parent = Q_NULLPTR);

private:
	Ui::DlgPinInput *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_PIN_INPUT_H_ */
