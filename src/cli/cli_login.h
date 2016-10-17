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

#ifndef _CLI_LOGIN_H_
#define _CLI_LOGIN_H_

#include <QString>

#include "src/io/isds_sessions.h"
#include "src/settings/accounts.h"

/*!
 * @brief Connects to ISDS and downloads basic information about the user.
 *
 * @param[in,out] isdsSessions Sessions container reference.
 * @param[in]     acntSettings Account settings.
 * @param[in]     pwd User password is used when no password in settings.
 * @param[in]     key Either certificate key or OTP key.
 * @return True on successful login.
 */
bool connectToIsdsCLI(IsdsSessions &isdsSessions, AcntSettings acntSettings,
    const QString &pwd, const QString &key);

#endif /* _CLI_LOGIN_H_ */
