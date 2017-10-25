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

#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

#include <QMap>
#include <QObject>
#include <QSettings>
#include <QString>

#include "src/settings/account.h"

/* Meta object features are not supported for nested classes. */

/*!
 * @brief Associative array mapping user name to settings.
 */
class AccountsMap : public QObject, public QMap<QString, AcntSettings> {
	Q_OBJECT

public:
	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] confDir Configuration directory path.
	 * @param[in] settings Settings structure to load data from.
	 */
	void loadFromSettings(const QString &confDir,
	    const QSettings &settings);

	/*!
	 * @brief Decrypts all encrypted passwords.
	 *
	 * @param[in] pinVal Pin value.
	 */
	void decryptAllPwds(const QString &pinVal);

signals:
	/*!
	 * @brief Notifies that account data have changed.
	 *
	 * @note Currently the signal must be triggered manually.
	 *
	 * @param[in] userName User name.
	 */
	void accountDataChanged(const QString &userName);
};

/*!
 * @brief Holds account data related to account.
 *
 * @note Key is userName. The user name is held by the user name list.
 */
extern AccountsMap globAccounts;

#endif /* _ACCOUNTS_H_ */
