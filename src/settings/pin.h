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

#ifndef _PIN_H_
#define _PIN_H_

#include <QByteArray>
#include <QSettings>
#include <QString>

/*!
 * @brief Encapsulates code needed for storing PIN code.
 */
class PinSettings {
public:
	/*!
	 * @brief Constructor.
	 */
	PinSettings(void);

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] settings Settings structure.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[out] settings Settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Check whether PIN value is configured.
	 *
	 * @return True if algorithm, salt and encoded PIN are set.
	 */
	bool pinConfigured(void) const;

	/*!
	 * @brief Update PIN settings.
	 *
	 * @param[in,out] sett Settings to be modified.
	 * @param[in]     pinValue New pin value.
	 */
	static
	void updatePinSettings(PinSettings &sett, const QString &pinValue);

	/*!
	 * @brief Verifies the PIN.
	 *
	 * @note PIN value is stored within setting structure if ented PIN is
	 *     valid.
	 *
	 * @param[in,out] sett PIN settings to be verified.
	 * @param[in]     pinValue PIN to be verified.
	 */
	static
	void verifyPin(PinSettings &sett, const QString &pinValue);

	QString _pinVal; /*! PIN value is not read from the configuration file, nor it is stored to the configuration file. */
	QString pinAlg; /*!< PIN algorithm identifier. */
	QByteArray pinSalt; /*!< Salt value used to generate PIN hash. */
	QByteArray pinCode; /*!< Hashed PIN code. */
};

/*!
 * @brief Global instance of the structure.
 */
extern PinSettings globPinSet;

#endif /* _PIN_H_ */
