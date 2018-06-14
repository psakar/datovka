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

#include <QMap>
#include <QString>

/*!
 * @brief Container holding context structures of libisds.
 */
class IsdsSessions {

public:
	/*!
	 * @brief Constructor.
	 */
	IsdsSessions(void);

	/*!
	 * @brief Destructor.
	 */
	~IsdsSessions(void);

	/*!
	 * @brief Returns true is active session exists.
	 *
	 * @param[in] userName Username identifying the account.
	 */
	bool holdsSession(const QString &userName) const;

	/*!
	 * @brief Returns associated session.
	 *
	 * @param[in] userName Username identifying the account.
	 */
	struct isds_ctx *session(const QString &userName) const;

	/*!
	 * @brief Ping ISDS. Test whether connection is active.
	 *
	 * @param[in] userName Username identifying the account.
	 */
	bool isConnectedToIsds(const QString &userName);

	/*!
	 * @brief Creates new session.
	 *
	 * @param[in] userName Username identifying the newly created account.
	 * @param[in] connectionTimeoutMs Connection timeout in milliseconds.
	 * @return Pointer to new session or NULL on failure.
	 */
	struct isds_ctx *createCleanSession(const QString &userName,
	    unsigned int connectionTimeoutMs);

	/*!
	 * @brief Set time-out in milliseconds to session associated to
	 *     user name.
	 *
	 * @return True on success.
	 */
	bool setSessionTimeout(const QString &userName, unsigned int timeoutMs);

private:
	QMap<QString, struct isds_ctx *> m_sessions; /*!< Holds sessions. */
};
