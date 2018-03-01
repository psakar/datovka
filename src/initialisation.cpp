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

#ifndef WIN32
#include <clocale>
#endif /* !WIN32 */
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QTranslator>

#include "src/cli/cli_parser.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/datovka_shared/io/records_management_db.h"
#include "src/datovka_shared/localisation/localisation.h"
#include "src/datovka_shared/settings/pin.h"
#include "src/datovka_shared/settings/records_management.h"
#include "src/global.h"
#include "src/initialisation.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/io/file_downloader.h"
#include "src/io/filesystem.h"
#include "src/io/message_db_set_container.h"
#include "src/io/tag_db.h"
#include "src/settings/accounts.h"
#include "src/settings/preferences.h"
#include "src/settings/proxy.h"

void setDefaultLocale(void)
{
#ifndef WIN32
	QString lang(QProcessEnvironment::systemEnvironment().value("LANG"));
	if (lang.isEmpty() || ("c" == lang.toLower())) {
#define LANG_DEF "en_GB.UTF-8"
		logWarning("The LANG environment variable is missing or unset. "
		    "Setting to '%s'.\n", LANG_DEF);
		if (NULL == setlocale(LC_ALL, LANG_DEF)) {
			logError("Setting locale to '%s' failed.\n", LANG_DEF);
		}
#undef LANG_DEF
	}
#endif /* !WIN32 */
}

int preferencesSetUp(const QCommandLineParser &parser,
    GlobPreferences &prefs, LogDevice &log)
{
	int logFileId = -1;

	if (parser.isSet(CONF_SUBDIR_OPT)) {
		prefs.confSubdir = parser.value(CONF_SUBDIR_OPT);
	}
	if (parser.isSet(LOAD_CONF_OPT)) {
		prefs.loadFromConf = parser.value(LOAD_CONF_OPT);
	}
	if (parser.isSet(SAVE_CONF_OPT)) {
		prefs.saveToConf = parser.value(SAVE_CONF_OPT);
	}
	if (parser.isSet(LOG_FILE)) {
		QString logFileName = parser.value(LOG_FILE);
		logFileId = log.openFile(logFileName.toUtf8().constData(),
		    LogDevice::LM_APPEND);
		if (-1 == logFileId) {
			logError("Cannot open log file '%s'.\n",
			    logFileName.toUtf8().constData());
			return -1;
		}
		/* Log warnings. */
		log.setLogLevels(logFileId, LOGSRC_ANY,
		    LOG_UPTO(LOG_WARNING));
	}
#ifdef DEBUG
	if (parser.isSet(DEBUG_OPT) || parser.isSet(DEBUG_VERBOSITY_OPT)) {
		log.setLogLevels(LogDevice::LF_STDERR, LOGSRC_ANY,
		    LOG_UPTO(LOG_DEBUG));
		if (-1 != logFileId) {
			log.setLogLevels(logFileId, LOGSRC_ANY,
			    LOG_UPTO(LOG_DEBUG));
		}
	}
	if (parser.isSet(DEBUG_VERBOSITY_OPT)) {
		bool ok;
		int value = parser.value(DEBUG_VERBOSITY_OPT).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid debug-verbosity parameter.");
			exit(EXIT_FAILURE);
		}
		logInfo("Setting debugging verbosity to value '%d'.\n", value);
		log.setDebugVerbosity(value);
	}
#endif /* DEBUG */
	if (parser.isSet(LOG_VERBOSITY_OPT)) {
		bool ok;
		int value = parser.value(LOG_VERBOSITY_OPT).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid log-verbosity parameter.");
			exit(EXIT_FAILURE);
		}
		logInfo("Setting logging verbosity to value '%d'.\n", value);
		log.setLogVerbosity(value);
	}

	return 0;
}

void downloadCRL(void)
{
	/* Start downloading the CRL files. */
	QList<QUrl> urlList;
	FileDownloader fDown(true);
	const struct crl_location *crl = crl_locations;
	const char **url;
	while ((NULL != crl) && (NULL != crl->file_name)) {
		urlList.clear();

		url = crl->urls;
		while ((NULL != url) && (NULL != *url)) {
			urlList.append(QUrl(*url));
			++url;
		}

		QByteArray data = fDown.download(urlList, 2000);
		if (!data.isEmpty()) {
			if (0 != crypto_add_crl(data.data(),
			        data.size())) {
				logWarning(
				    "Couldn't load downloaded CRL file '%s'.\n",
				    crl->file_name);
			} else {
				logInfo("Loaded CRL file '%s'.\n",
				    crl->file_name);
			}
		} else {
			logWarning("Couldn't download CRL file '%s'.\n",
			    crl->file_name);
		}

		++crl;
	}
}

void loadLocalisation(const GlobPreferences &prefs)
{
	static QTranslator qtTranslator, appTranslator;

	QSettings settings(prefs.loadConfPath(), QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	const QString language(
	    settings.value("preferences/language").toString());

	/* Check for application localisation location. */
	QString localisationDir;
	QString localisationFile;

	localisationDir = appLocalisationDir();

	logInfo("Loading application localisation from path '%s'.\n",
	    localisationDir.toUtf8().constData());

	localisationFile = "datovka_" + Localisation::shortLangName(language);

	Localisation::setProgramLocale(language);

	if (!appTranslator.load(localisationFile, localisationDir)) {
		logWarning("Could not load localisation file '%s' "
		    "from directory '%s'.\n",
		    localisationFile.toUtf8().constData(),
		    localisationDir.toUtf8().constData());
	}

	QCoreApplication::installTranslator(&appTranslator);

	localisationDir = qtLocalisationDir();

	logInfo("Loading Qt localisation from path '%s'.\n",
	    localisationDir.toUtf8().constData());

	{
		const QString langName(Localisation::shortLangName(language));
		if (langName != Localisation::langEn) {
			localisationFile =
			    "qtbase_" + Localisation::shortLangName(language);
		} else {
			localisationFile = QString();
		}
	}

	if (!localisationFile.isEmpty() &&
	    !qtTranslator.load(localisationFile, localisationDir)) {
		logWarning("Could not load localisation file '%s' "
		    "from directory '%s'.\n",
		    localisationFile.toUtf8().constData(),
		    localisationDir.toUtf8().constData());
	}

	QCoreApplication::installTranslator(&qtTranslator);
}

int allocGlobSettings(void)
{
	GlobInstcs::prefsPtr = new (std::nothrow) GlobPreferences;
	if (Q_NULLPTR == GlobInstcs::prefsPtr) {
		logErrorNL("%s", "Cannot allocate preferences.");
		goto fail;
	}

	GlobInstcs::proxSetPtr = new (std::nothrow) ProxiesSettings;
	if (Q_NULLPTR == GlobInstcs::proxSetPtr) {
		logErrorNL("%s", "Cannot allocate proxy settings.");
		goto fail;
	}

	GlobInstcs::pinSetPtr = new (std::nothrow) PinSettings;
	if (Q_NULLPTR == GlobInstcs::pinSetPtr) {
		logErrorNL("%s", "Cannot allocated PIN settings.");
		goto fail;
	}

	GlobInstcs::recMgmtSetPtr =
	    new (std::nothrow) RecordsManagementSettings;
	if (Q_NULLPTR == GlobInstcs::recMgmtSetPtr) {
		logErrorNL("%s",
		    "Cannot allocate records management settings.");
		goto fail;
	}

	return 0;

fail:
	deallocGlobSettings();
	return -1;
}

void deallocGlobSettings(void)
{
	if (Q_NULLPTR != GlobInstcs::recMgmtSetPtr) {
		delete GlobInstcs::recMgmtSetPtr;
		GlobInstcs::recMgmtSetPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::pinSetPtr) {
		delete GlobInstcs::pinSetPtr;
		GlobInstcs::pinSetPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::proxSetPtr) {
		delete GlobInstcs::proxSetPtr;
		GlobInstcs::proxSetPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::prefsPtr) {
		delete GlobInstcs::prefsPtr;
		GlobInstcs::prefsPtr = Q_NULLPTR;
	}
}

int allocGlobContainers(const GlobPreferences &prefs)
{
	SQLiteDb::OpenFlags flags = SQLiteDb::NO_OPTIONS;

	/*
	 * These objects cannot be globally accessible static objects.
	 * The unpredictable order of constructing and destructing these
	 * objects causes segmentation faults upon their destruction.
	 *
	 * TODO -- Solve the problem of this globally accessible structures.
	 */

	GlobInstcs::isdsSessionsPtr = new (std::nothrow) IsdsSessions;
	if (Q_NULLPTR == GlobInstcs::isdsSessionsPtr) {
		logErrorNL("%s", "Cannot allocate session container.");
		goto fail;
	}

	GlobInstcs::accntDbPtr = new (std::nothrow) AccountDb("accountDb", false);
	if (Q_NULLPTR == GlobInstcs::accntDbPtr) {
		logErrorNL("%s", "Cannot allocate account db.");
		goto fail;
	}
	/* Open accounts database. */
	flags = SQLiteDb::CREATE_MISSING;
	flags |= prefs.store_additional_data_on_disk ?
	    SQLiteDb::NO_OPTIONS : SQLiteDb::FORCE_IN_MEMORY;
	if (!GlobInstcs::accntDbPtr->openDb(prefs.accountDbPath(), flags)) {
		logErrorNL("Error opening account db '%s'.",
		    prefs.accountDbPath().toUtf8().constData());
		goto fail;
	}

	/* Create message DB container. */
	GlobInstcs::msgDbsPtr = new (std::nothrow) DbContainer("GLOBALDBS");
	if (Q_NULLPTR == GlobInstcs::msgDbsPtr) {
		logErrorNL("%s", "Cannot allocate message db container.");
		goto fail;
	}

	GlobInstcs::tagDbPtr = new (std::nothrow) TagDb("tagDb", false);
	if (Q_NULLPTR == GlobInstcs::tagDbPtr) {
		logErrorNL("%s", "Cannot allocate tag db.");
		goto fail;
	}
	/* Open tags database. */
	flags = SQLiteDb::CREATE_MISSING;
	if (!GlobInstcs::tagDbPtr->openDb(prefs.tagDbPath(), flags)) {
		logErrorNL("Error opening tag db '%s'.",
		    prefs.tagDbPath().toUtf8().constData());
		goto fail;
	}

	GlobInstcs::recMgmtDbPtr = new (std::nothrow)
	    RecordsManagementDb("recordsManagementDb", false);
	if (Q_NULLPTR == GlobInstcs::recMgmtDbPtr) {
		logErrorNL("%s", "Cannot allocate records management db.");
		goto fail;
	}
	/* Open records management database. */
	flags = SQLiteDb::CREATE_MISSING;
	if (!GlobInstcs::recMgmtDbPtr->openDb(
	        prefs.recordsManagementDbPath(), flags)) {
		logErrorNL("Error opening records management db '%s'.",
		    prefs.recordsManagementDbPath().toUtf8().constData());
		goto fail;
	}

	GlobInstcs::acntMapPtr = new (std::nothrow) AccountsMap;
	if (Q_NULLPTR == GlobInstcs::acntMapPtr) {
		logErrorNL("%s", "Cannot allocate account container.");
		goto fail;
	}

	return 0;

fail:
	deallocGlobContainers();
	return -1;
}

void deallocGlobContainers(void)
{
	if (Q_NULLPTR != GlobInstcs::acntMapPtr) {
		delete GlobInstcs::acntMapPtr;
		GlobInstcs::acntMapPtr = Q_NULLPTR;
	}

	if (Q_NULLPTR != GlobInstcs::recMgmtDbPtr) {
		delete GlobInstcs::recMgmtDbPtr;
		GlobInstcs::recMgmtDbPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::tagDbPtr) {
		delete GlobInstcs::tagDbPtr;
		GlobInstcs::tagDbPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::msgDbsPtr) {
		delete GlobInstcs::msgDbsPtr;
		GlobInstcs::msgDbsPtr = Q_NULLPTR;
	}
	if (Q_NULLPTR != GlobInstcs::accntDbPtr) {
		delete GlobInstcs::accntDbPtr;
		GlobInstcs::accntDbPtr = Q_NULLPTR;
	}

	if (Q_NULLPTR != GlobInstcs::isdsSessionsPtr) {
		delete GlobInstcs::isdsSessionsPtr;
		GlobInstcs::isdsSessionsPtr = Q_NULLPTR;
	}
}
