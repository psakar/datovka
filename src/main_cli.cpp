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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <cstdlib>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QtGlobal> /* qsrand() */
#include <QThread>

#include "src/about.h"
#include "src/cli/cli_parser.h"
#include "src/cli/cli_pin.h"
#include "src/crypto/crypto_funcs.h"
#include "src/crypto/crypto_threads.h"
#include "src/crypto/crypto_version.h"
#include "src/datovka_shared/io/sqlite/db.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/settings/pin.h"
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/initialisation.h"
#include "src/io/db_tables.h"
#include "src/io/filesystem.h"
#include "src/settings/accounts.h"
#include "src/settings/proxy.h"
#include "src/single/single_instance.h"

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	if (0 != allocGlobLog()) {
		/* Cannot continue without logging facility. */
		return EXIT_FAILURE;
	}

	/* Set random generator. */
	qsrand(QDateTime::currentDateTime().toTime_t());

	/* Log warnings. */
	GlobInstcs::logPtr->setLogLevelBits(LogDevice::LF_STDERR, LOGSRC_ANY,
	    LOG_UPTO(LOG_WARNING));

	setDefaultLocale();

	/* TODO -- Make the following assignments configurable. */
	QCoreApplication::setApplicationName("Datovka CLI");
	QCoreApplication::setApplicationVersion(VERSION " -- [" +
	    libraryDependencies().join("; ") + "]");

	qInstallMessageHandler(globalLogOutput);

	if (0 != allocGlobInfrastruct()) {
		return EXIT_FAILURE;
	}
	if (0 != allocGlobSettings()) {
		return EXIT_FAILURE;
	}

	QCommandLineParser parser;
	if (0 != CLIParser::setupCmdLineParser(parser)) {
		logErrorNL("%s", "Cannot set up command-line parser.");
		return EXIT_FAILURE;
	}

	/* Process command-line arguments. */
	parser.process(app);

	if (0 != preferencesSetUp(parser, *GlobInstcs::prefsPtr,
	        *GlobInstcs::logPtr)) {
		logErrorNL("%s",
		    "Cannot apply command line arguments on global settings.");
		return EXIT_FAILURE;
	}

	const QStringList srvcArgs(
	    CLIParser::CLIServiceArgs(parser.optionNames()));
	if (srvcArgs.isEmpty()) {
		logErrorNL("%s",
		    "No command line arguments supplied. Exiting.");
		return EXIT_FAILURE;
	}

	/* Ensure only one instance with given configuration file. */
	SingleInstance singleInstance(GlobInstcs::prefsPtr->loadConfPath());
	if (singleInstance.existsInSystem()) {
		logErrorNL("%s",
		    "Another application already uses same configuration. Exiting.");
		return EXIT_FAILURE;
	}

	qint64 start, stop, diff;
	start = QDateTime::currentMSecsSinceEpoch();
	logInfo("Starting at %lld.%03lld .\n", start / 1000, start % 1000);

	/* Create configuration file if file is missing. */
	GlobInstcs::prefsPtr->ensureConfPresence();

	logAppVersion();
	logTimeZone();
	logQtVersion();

	switch (crypto_compiled_lib_ver_check()) {
	case 1:
		logErrorNL("%s", "Cryptographic library mismatch.");
#if defined(Q_OS_WIN)
		/* Exit only on windows. */
		return EXIT_FAILURE;
#endif /* defined(Q_OS_WIN) */
		break;
	case 0:
		break;
	case -1:
	default:
		logErrorNL("%s",
		    "Error checking the version of the cryptographic library.");
		return EXIT_FAILURE;
		break;
	}

	if (0 != crypto_init()) {
		logError("%s\n", "Cannot load cryptographic back-end.");
		/*
		 * TODO -- the function should fail only when all certificates
		 * failed to load.
		 */
		return EXIT_FAILURE;
	}
	crypto_init_threads();

	{
		/* Stuff related to configuration file. */

		logDebugLv0NL("Load: '%s'.",
		    GlobInstcs::prefsPtr->loadConfPath().toUtf8().constData());
		logDebugLv0NL("Save: '%s'.",
		    GlobInstcs::prefsPtr->saveConfPath().toUtf8().constData());

		/* Change "\" to "/" */
		confFileFixBackSlashes(GlobInstcs::prefsPtr->loadConfPath());
	}

	/* Set up proxy. */
	{
		QSettings settings(GlobInstcs::prefsPtr->loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		/* Load proxy settings. */
		GlobInstcs::proxSetPtr->loadFromSettings(settings);
		/* Apply settings onto the environment because of libcurl. */
		GlobInstcs::proxSetPtr->setProxyEnvVars();
	}

	/* Start downloading the CRL files. */
	downloadCRL();

	if (!SQLiteDb::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    SQLiteDb::dbDriverType.toUtf8().constData());
		/* TODO -- throw a dialog notifying the user. */
		return EXIT_FAILURE;
	}

	logDebugLv0NL("CLI main thread: %p.", QThread::currentThreadId());

	logDebugLv0NL("App path: '%s'.",
	    app.applicationDirPath().toUtf8().constData());

	loadLocalisation(*GlobInstcs::prefsPtr);

	/* Ask for PIN. */
	{
		QSettings settings(GlobInstcs::prefsPtr->loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		GlobInstcs::pinSetPtr->loadFromSettings(settings);
		if (GlobInstcs::pinSetPtr->pinConfigured()) {
			if (!CLIPin::queryPin(*GlobInstcs::pinSetPtr, 2)) {
				return EXIT_FAILURE;
			}
		}
	}

	/* Localise description in tables. */
	localiseTableDescriptions();

	if (0 != allocGlobContainers(*GlobInstcs::prefsPtr)) {
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;

	/* Start worker threads. */
	GlobInstcs::workPoolPtr->start();
	logInfo("%s\n", "Worker pool started.");

	/* Parse account information. */
	{
		QSettings settings(GlobInstcs::prefsPtr->loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		GlobInstcs::acntMapPtr->loadFromSettings(
		    GlobInstcs::prefsPtr->confDir(), settings);
		GlobInstcs::acntMapPtr->decryptAllPwds(
		    GlobInstcs::pinSetPtr->_pinVal);
	}

	ret = CLIParser::runCLIService(srvcArgs, parser);

	/* Wait until all threads finished. */
	logInfo("%s\n", "Waiting for pending worker threads.");
	GlobInstcs::workPoolPtr->wait();
	GlobInstcs::workPoolPtr->stop();
	logInfo("%s\n", "All worker threads finished");

	stop = QDateTime::currentMSecsSinceEpoch();
	diff = stop - start;
	logInfo("Stopping at %lld.%03lld; ran for %lld.%03lld seconds.\n",
	    stop / 1000, stop % 1000, diff / 1000, diff % 1000);

	/*
	 * TODO -- The following clean-up function causes troubles.
	 * on OS X libcurl occasionally seems to operate while the lock
	 * de-initialisation is performed causing the application to crash
	 * on exit.
	 */
	//crypto_cleanup_threads();

	deallocGlobContainers();
	deallocGlobSettings();
	deallocGlobInfrastruct();
	deallocGlobLog();

	return ret;
}
