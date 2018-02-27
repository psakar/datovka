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

#include "src/global.h"
#include "src/io/message_db_set.h"
#include "src/io/message_db_set_container.h"
#include "src/log/log.h"
#include "src/model_interaction/account_interaction.h"
#include "src/settings/account.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"

/* TODO -- The method must be made shorter. */
MessageDbSet *AccountInteraction::accessDbSet(const QString &userName,
    enum AccessStatus &status, QString &dbDir, QString &namesStr)
{
	MessageDbSet *dbSet = Q_NULLPTR;
	int flags, dbPresenceCode;
	dbDir.clear();
	namesStr.clear();

	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		status = AS_ERR;
		return Q_NULLPTR;
	}

	/* Get user name and db location. */
	AcntSettings &itemSettings(globAccounts[userName]);

	if (!itemSettings.isValid()) {
		logWarningNL(
		    "Attempting to accessing database for user name '%s'. "
		    "The account seems not to exist.",
		    userName.toUtf8().constData());
		status = AS_ERR;
		return Q_NULLPTR;
	}

	dbDir = itemSettings.dbDir();
	if (dbDir.isEmpty()) {
		/* Set default directory name. */
		dbDir = globPref.confDir();
	}

	flags = 0;
	if (itemSettings.isTestAccount()) {
		flags |= MDS_FLG_TESTING;
	}
	if (itemSettings._createdFromScratch()) {
		/* Check database structure on account creation. */
		flags |= MDS_FLG_CHECK_QUICK;
	}
	dbPresenceCode =
	    MessageDbSet::checkExistingDbFile(dbDir, userName, flags);

	switch (dbPresenceCode) {
	case MDS_ERR_OK:
		if (itemSettings._createdFromScratch()) {
			/* Notify the user on account creation. */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			namesStr = "'" + dbFileNames[0] + "'";
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logInfoNL(
			    "Database files %s for user name '%s' already present in '%s'.",
			    namesStr.toUtf8().constData(),
			    userName.toUtf8().constData(),
			    dbDir.toUtf8().constData());
			status = AS_DB_ALREADY_PRESENT;
		} else {
			status = AS_OK;
		}
		dbSet = GlobInstcs::msgDbsPtr->accessDbSet(dbDir, userName,
		    itemSettings.isTestAccount(),
		    MessageDbSet::DO_UNKNOWN,
		    MessageDbSet::CM_MUST_EXIST);
		break;
	case MDS_ERR_MISSFILE:
		if (!itemSettings._createdFromScratch()) {
			/* Not on account creation. */
			logWarningNL(
			    "Missing database files for user name '%s' in '%s'.",
			    userName.toUtf8().constData(),
			    dbDir.toUtf8().constData());
			status = AS_DB_NOT_PRESENT;
		} else {
			status = AS_OK;
		}
		dbSet = GlobInstcs::msgDbsPtr->accessDbSet(dbDir, userName,
		    itemSettings.isTestAccount(),
		    MessageDbSet::DO_YEARLY,
		    MessageDbSet::CM_CREATE_EMPTY_CURRENT);
		break;
	case MDS_ERR_NOTAFILE:
		{
			/* Notify the user that the location is not a file. */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			namesStr = "'" + dbFileNames[0] + "'";
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarningNL(
			    "Some databases of %s in '%s' related to user name '%s' are not a file.",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			status = AS_DB_NOT_FILES;
		}
		break;
	case MDS_ERR_ACCESS:
		{
			/* Notify that the user does not have enough rights. */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			namesStr = "'" + dbFileNames[0] + "'";
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarningNL(
			    "Some databases of '%s' in '%s' related to user name '%s' cannot be accessed.",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			status = AS_DB_FILES_INACCESSIBLE;
		}
		break;
	case MDS_ERR_CREATE:
		{
			/* This error should not be returned. */
			Q_ASSERT(0);
			status = AS_ERR;
		}
		break;
	case MDS_ERR_DATA:
		{
			/*
			 * Database file is not a database file or is
			 * corrupted.
			 */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			namesStr = "'" + dbFileNames[0] + "'";
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarningNL(
			    "Some databases of %s in '%s' related to user name '%s' is probably corrupted.",
			    namesStr.toUtf8().constData(),
			    dbDir.toUtf8().constData(),
			    userName.toUtf8().constData());
			status = AS_DB_FILES_CORRUPT;
		}
		break;
	case MDS_ERR_MULTIPLE:
		{
			/*
			 * Multiple database organisation types reside in the
			 * same location.
			 */
			QStringList dbFileNames(
			    MessageDbSet::existingDbFileNamesInLocation(
			        dbDir, userName,
			        itemSettings.isTestAccount(),
			        MessageDbSet::DO_UNKNOWN, true));
			Q_ASSERT(!dbFileNames.isEmpty());
			namesStr = "'" + dbFileNames[0] + "'";
			for (int i = 1; i < dbFileNames.size(); ++i) {
				namesStr += ", '" + dbFileNames[i] + "'";
			}
			logWarningNL(
			    "Multiple databases %s for '%s' have been encountered in the location '%s'.",
			    namesStr.toUtf8().constData(),
			    userName.toUtf8().constData(),
			    dbDir.toUtf8().constData());
			status = AS_DB_CONFUSING_ORGANISATION;
		}
		break;
	default:
		/* The code should not end here. */
		Q_ASSERT(0);
		status = AS_ERR;
		break;
	}

	if (itemSettings._createdFromScratch()) {
		/* Notify only once. */
		itemSettings._setCreatedFromScratch(false);
	}

	if (Q_NULLPTR == dbSet) {
		logErrorNL(
		    "Database files for user name '%s' in '%s' cannot be created or is probably corrupted.",
		    userName.toUtf8().constData(),
		    dbDir.toUtf8().constData());
	}

	return dbSet;
}
