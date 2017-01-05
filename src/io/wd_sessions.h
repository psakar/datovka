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


#ifndef _WEBDATOVKA_SESSIONS_H_
#define _WEBDATOVKA_SESSIONS_H_

#include <QMap>
#include <QString>
#include <QNetworkCookie>


/*!
 * @brief Holds the webdatovka context structures.
 */
class GlobWDSessions {

public:
	GlobWDSessions(void);
	~GlobWDSessions(void);

	/*!
	 * @brief Returns true is active session exists.
	 */
	bool holdsSession(const QString &userName) const;

	/*!
	 * @brief Returns associated session cookie.
	 */
	QNetworkCookie sessionCookie(const QString &userName) const;

	/*!
	 * @brief Ping of webdatovka. Test if connection is active.
	 */
	bool isConnectedToWebdatovka(const QString &userName);

	/*!
	 * @brief Creates new session.
	 */
	void createCleanSession(const QString &userName);

	/*!
	 * @brief Set cookie to session associated to user name.
	 *
	 * @return True on success.
	 */
	bool setSessionCookie(const QString &userName,
	    const QNetworkCookie &cookie);

private:
	QMap<QString, QNetworkCookie> m_wdSessions;
};

/* Global webdatovka context container instance. */
extern GlobWDSessions wdSessions;

#endif /* _WEBDATOVKA_SESSIONS_H_ */
