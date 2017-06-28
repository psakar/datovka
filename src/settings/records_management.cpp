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

#include <QByteArray>

#include "src/settings/records_management.h"

#define RM_GROUP QLatin1String("records_management")
#define RM_URL QLatin1String("location_url")
#define RM_TOKEN QLatin1String("access_token")

RecordsManagementSettings globRecordsManagementSet;

RecordsManagementSettings::RecordsManagementSettings(void)
    : url(),
    token()
{
}

void RecordsManagementSettings::loadFromSettings(const QSettings &settings)
{
	url = settings.value(RM_GROUP + QLatin1String("/") + RM_URL,
	    QString()).toString();
	token = QByteArray::fromBase64(
	    settings.value(RM_GROUP + QLatin1String("/") + RM_TOKEN,
	        QByteArray()).toString().toUtf8());

	if (url.isEmpty() || token.isEmpty()) {
		url.clear();
		token.clear();
	}
}

void RecordsManagementSettings::saveToSettings(QSettings &settings) const
{
	if (!url.isEmpty() && !token.isEmpty()) {
		settings.beginGroup(RM_GROUP);

		settings.setValue(RM_URL, url);
		settings.setValue(RM_TOKEN, QString(token.toUtf8().toBase64()));

		settings.endGroup();
	}
}

bool RecordsManagementSettings::isSet(void) const
{
	return !url.isEmpty() && !token.isEmpty();
}
