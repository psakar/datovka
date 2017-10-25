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

#include <cstdlib>
#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QSplashScreen>
#include <QtGlobal> /* qsrand() */
#include <QThread>

#include "src/about.h"
#include "src/cli/cli_parser.h"
#include "src/cli/cli_pin.h"
#include "src/crypto/crypto_funcs.h"
#include "src/crypto/crypto_threads.h"
#include "src/crypto/crypto_version.h"
#include "src/gui/datovka.h"
#include "src/gui/dlg_pin_input.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/initialisation.h"
#include "src/io/db_tables.h"
#include "src/io/filesystem.h"
#include "src/io/sqlite/db.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/settings/pin.h"
#include "src/settings/proxy.h"
#include "src/single/single_instance.h"
#include "src/worker/pool.h"

/*!
 * @brief Specified the mode the executable is being executed with.
 */
enum RunMode {
	RM_GUI, /*!< Start with active GUI. */
	RM_CLI, /*!< Execute command line service. */
	RM_ZFO /*!< View ZFO content. */
};

/*!
 * @brief View data message from ZFO file.
 *
 * @param[in] fileName ZFO file name.
 */
static
int showZfo(const QString &fileName)
{
	if (fileName.isEmpty()) {
		return -1;
	}

	DlgViewZfo::view(fileName, Q_NULLPTR);

	return 0;
}

int main(int argc, char *argv[])
{
	/* Set random generator. */
	qsrand(QDateTime::currentDateTime().toTime_t());

	/* Log warnings. */
	globLog.setLogLevels(LogDevice::LF_STDERR, LOGSRC_ANY,
	    LOG_UPTO(LOG_WARNING));

	setDefaultLocale();

	/* TODO -- Make the following assignments configurable. */
	QCoreApplication::setApplicationName("Datovka");
	QCoreApplication::setApplicationVersion(VERSION " -- [" +
	    libraryDependencies().join("; ") + "]");

	qInstallMessageHandler(globalLogOutput);

	QApplication app(argc, argv);

	QCommandLineParser parser;
	if (0 != CLIParser::setupCmdLineParser(parser)) {
		logErrorNL("%s", "Cannot set up command-line parser.");
		return EXIT_FAILURE;
	}

	/* Process command-line arguments. */
	parser.process(app);

	if (0 != preferencesSetUp(parser, globPref, globLog)) {
		logErrorNL("%s",
		    "Cannot apply command line arguments on global settings.");
		return EXIT_FAILURE;
	}

	const QStringList cmdLineFileNames(parser.positionalArguments());
	const QStringList srvcArgs(
	    CLIParser::CLIServiceArgs(parser.optionNames()));

	enum RunMode runMode = RM_GUI;
	QSplashScreen *splash = Q_NULLPTR; /* Used only in GUI mode. */

	if (!srvcArgs.isEmpty()) {
		runMode = RM_CLI;
	} else if (!cmdLineFileNames.isEmpty()) {
		runMode = RM_ZFO;
	} else {
		splash = new QSplashScreen;
		splash->setPixmap(QPixmap(":/splash/datovka-splash.png"));
		splash->show();
	}

	/* Ensure only one instance with given configuration file. */
	SingleInstance singleInstance(globPref.loadConfPath());
	if (singleInstance.existsInSystem()) {
		logErrorNL("%s",
		    "Another application already uses same configuration.");
		logInfo("%s\n", "Notifying key owner and exiting");
		if (!singleInstance.sendMessage(
		        SingleInstance::msgRaiseMainWindow)) {
			logErrorNL("%s",
			    "Could not send message to owner.");
		}
		return EXIT_FAILURE;
	}

	qint64 start, stop, diff;
	start = QDateTime::currentMSecsSinceEpoch();
	logInfo("Starting at %lld.%03lld .\n", start / 1000, start % 1000);

	/* Create configuration file is file is missing. */
	GlobPreferences::ensureConfPresence();

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
		/* TODO -- throw a dialogue notifying the user. */
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
		    globPref.loadConfPath().toUtf8().constData());
		logDebugLv0NL("Save: '%s'.",
		    globPref.saveConfPath().toUtf8().constData());

		/* Change "\" to "/" */
		confFileFixBackSlashes(globPref.loadConfPath());
	}

	/* Set up proxy. */
	{
		QSettings settings(globPref.loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		/* Load proxy settings. */
		globProxSet.loadFromSettings(settings);
		/* Apply settings onto the environment because of libcurl. */
		globProxSet.setProxyEnvVars();
	}

	/* Start downloading the CRL files. */
	downloadCRL();

	if (!SQLiteDb::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    SQLiteDb::dbDriverType.toUtf8().constData());
		/* TODO -- throw a dialog notifying the user. */
		return EXIT_FAILURE;
	}

	logDebugLv0NL("GUI main thread: %p.", QThread::currentThreadId());

	logDebugLv0NL("App path: '%s'.",
	    app.applicationDirPath().toUtf8().constData());

	loadLocalisation(globPref);

	/* Ask for PIN. */
	{
		QSettings settings(globPref.loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		globPinSet.loadFromSettings(settings);
		if (globPinSet.pinConfigured()) {
			bool pinOk = false;
			if (RM_GUI == runMode) {
				pinOk =
				    DlgPinInput::queryPin(globPinSet, splash);
			} else if (RM_CLI == runMode) {
				pinOk = CLIPin::queryPin(globPinSet, 2);
			}

			if (!pinOk) {
				delete splash;
				return EXIT_FAILURE;
			}
		}
	}

	/* set splash action text */
	if (runMode == RM_GUI) {
		Qt::Alignment align = Qt::AlignCenter;
		splash->showMessage(QObject::tr("Application is loading..."),
		align, Qt::white);
		app.processEvents();
	}

	/* Localise description in tables. */
	localiseTableDescriptions();

	if (0 != allocateGlobalObjects(globPref)) {
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;

	/* Start worker threads. */
	globWorkPool.start();
	logInfo("%s\n", "Worker pool started.");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	// https://blog.qt.io/blog/2016/01/26/high-dpi-support-in-qt-5-6/
	//logInfoNL("%s", "Enabling high DPI scaling.");
	//QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif /* >= Qt-5.6 */

	if (runMode == RM_CLI) {
		/* Parse account information. */
		{
			QSettings settings(globPref.loadConfPath(),
			    QSettings::IniFormat);
			settings.setIniCodec("UTF-8");
			globAccounts.loadFromSettings(globPref.confDir(),
			    settings);
			globAccounts.decryptAllPwds(globPinSet._pinVal);
		}
		delete splash;
		ret = CLIParser::runCLIService(srvcArgs, parser);
	} else if (runMode == RM_ZFO) {
		delete splash;
		foreach (const QString &fileName, cmdLineFileNames) {
			ret = showZfo(fileName);
			if (0 != ret) {
				break;
			}
		}
	} else {
		MainWindow mainwin;
		mainwin.show();
		splash->finish(&mainwin);
		delete splash;
		ret = (0 == app.exec()) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	/* Wait until all threads finished. */
	logInfo("%s\n", "Waiting for pending worker threads.");
	globWorkPool.wait();
	globWorkPool.stop();
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

	deallocateGlobalObjects();

	return ret;
}
