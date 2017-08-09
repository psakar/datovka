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

#include "src/settings/accounts.h"

AccountsMap globAccounts;

void AccountsMap::loadFromSettings(const QSettings &settings)
{
	QStringList groups = settings.childGroups();
	QRegExp credRe(CredNames::creds + ".*");
	AcntSettings itemSettings;

	/* Clear present rows. */
	this->clear();

	QStringList credetialList;
	/* Get list of credentials. */
	for (int i = 0; i < groups.size(); ++i) {
		/* Matches regular expression. */
		if (credRe.exactMatch(groups.at(i))) {
			credetialList.append(groups.at(i));
		}
	}

	/* Sort the credentials list. */
	qSort(credetialList.begin(), credetialList.end(),
	    AcntSettings::credentialsLessThan);

	/* For all credentials. */
	foreach(const QString &group, credetialList) {
		itemSettings.clear();

		itemSettings.loadFromSettings(settings, group);

		/* Associate map with item node. */
		Q_ASSERT(!itemSettings.userName().isEmpty());
		this->operator[](itemSettings.userName()) = itemSettings;
	}
}
