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


#include <QDebug>
#include <QObject>

#include "src/io/wd_sessions.h"
#include "src/log/log.h"

GlobWDSessions wdSessions;

/* ========================================================================= */
GlobWDSessions::GlobWDSessions(void)
/* ========================================================================= */
    : m_wdSessions()
{
}


/* ========================================================================= */
GlobWDSessions::~GlobWDSessions(void)
/* ========================================================================= */
{
	m_wdSessions.clear();
}


/* ========================================================================= */
bool GlobWDSessions::holdsSession(const QString &userName) const
/* ========================================================================= */
{
	return m_wdSessions.contains(userName);
}


/* ========================================================================= */
/*
 * Is connect to databox given by account index
 */
bool GlobWDSessions::isConnectedToWebdatovka(const QString &userName)
/* ========================================================================= */
{
	if (!holdsSession(userName)) {
		return false;
	} else {
		if (m_wdSessions.value(userName).name().isEmpty()) {
			return false;
		}
		if (m_wdSessions.value(userName).value().isEmpty()) {
			return false;
		}
	}
	return true;
}


/* ========================================================================= */
/*
 * Creates new session.
 */
void GlobWDSessions::createCleanSession(const QString &userName)
/* ========================================================================= */
{
	m_wdSessions.insert(userName, QNetworkCookie());
}


/* ========================================================================= */
/*
 * Set cookie to session associated to user name.
 */
bool GlobWDSessions::setSessionCookie(const QString &userName,
    const QNetworkCookie &cookie)
/* ========================================================================= */
{
	m_wdSessions.insert(userName, cookie);
	return true;
}


/* ========================================================================= */
/*
 * Returns associated session.
 */
QNetworkCookie GlobWDSessions::sessionCookie(const QString &userName) const
/* ========================================================================= */
{
	return m_wdSessions.value(userName);
}
