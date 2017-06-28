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

#ifndef _RECORDS_MANAGEMENT_H_
#define _RECORDS_MANAGEMENT_H_

#include <QSettings>
#include <QString>

/*
 * @brief Encapsulates records management service settings.
 */
class RecordsManagementSettings {
public:
	/*!
	 * @brief Constructor.
	 */
	RecordsManagementSettings(void);

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
	 * @brief Test whether records management service is set.
	 *
	 * @return True is URL and token are set.
	 */
	bool isSet(void) const;

	QString url; /*!< Service URL. */
	QString token; /*!< Service access token. */
};

/*!
 * @brief Global instance of the structure.
 */
extern RecordsManagementSettings globRecordsManagementSet;

#endif /* _RECORDS_MANAGEMENT_H_ */
