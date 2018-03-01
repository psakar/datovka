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

#ifndef _DLG_PIN_SETUP_H_
#define _DLG_PIN_SETUP_H_

#include <QDialog>

#include "src/datovka_shared/settings/pin.h"

namespace Ui {
	class DlgPinSetup;
}

/*!
 * @brief PIN setup dialogue.
 */
class DlgPinSetup : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief PIN setting operation.
	 */
	enum Operation {
		SET, /*!< Set new value if no previous value is set. */
		MODIFY, /*!< Modify value if previous value is set. */
		ERASE /*!< Clear previously set value. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] op Operation to be performed.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgPinSetup(enum Operation op, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgPinSetup(void);

	/*!
	 * @brief Set up or change the PIN value.
	 *
	 * @param[in,out] sett PIN settings to be modified.
	 * @param[in]     parent Parent widget.
	 * @return True when dialogue has been accepted and settings data
	 *     have been updated.
	 */
	static
	bool change(PinSettings &sett, QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Clear the PIN value.
	 *
	 * @param[in,out] sett PIN settings to be modified.
	 * @param[in]     parent Parent widget.
	 * @return True when dialogue has been accepted and settings data
	 *     have been updated.
	 */
	static
	bool erase(PinSettings &sett, QWidget *parent = Q_NULLPTR);

private:
	/*!
	 * @brief Update the PIN value.
	 *
	 * @param[in]     op Type of operation to be performed.
	 * @param[in,out] sett PIN settings to be modified.
	 * @param[in]     parent Parent widget.
	 * @return True when dialogue has been accepted and settings data
	 *     have been updated.
	 */
	static
	bool update(enum Operation op, PinSettings &sett,
	    QWidget *parent = Q_NULLPTR);

	Ui::DlgPinSetup *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_PIN_SETUP_H_ */
