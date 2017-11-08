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

#include <QByteArray>

#include "src/settings/records_management.h"

/* Records management configuration entry names. */
namespace RMnames {
	const QLatin1String rmGroup("records_management");

	const QLatin1String url("location_url");
	const QLatin1String token("access_token");
}

RecordsManagementSettings globRecordsManagementSet;

RecordsManagementSettings::RecordsManagementSettings(void)
    : m_url(),
    m_token()
{
}

void RecordsManagementSettings::loadFromSettings(const QSettings &settings)
{
	const QString prefix(RMnames::rmGroup + QLatin1String("/"));

	m_url = settings.value(prefix + RMnames::url, QString()).toString();
	m_token = QByteArray::fromBase64(
	    settings.value(prefix + RMnames::token,
	        QByteArray()).toString().toUtf8());

	if (m_url.isEmpty() || m_token.isEmpty()) {
		m_url.clear();
		m_token.clear();
	}
}

void RecordsManagementSettings::saveToSettings(QSettings &settings) const
{
	if (!m_url.isEmpty() && !m_token.isEmpty()) {
		settings.beginGroup(RMnames::rmGroup);

		settings.setValue(RMnames::url, m_url);
		settings.setValue(RMnames::token,
		    QString(m_token.toUtf8().toBase64()));

		settings.endGroup();
	}
}

bool RecordsManagementSettings::isSet(void) const
{
	return !m_url.isEmpty() && !m_token.isEmpty();
}

const QString &RecordsManagementSettings::url(void) const
{
	return m_url;
}

void RecordsManagementSettings::setUrl(const QString &url)
{
	m_url = url;
}

const QString &RecordsManagementSettings::token(void) const
{
	return m_token;
}

void RecordsManagementSettings::setToken(const QString &token)
{
	m_token = token;
}
