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
#include <QDir>
#include <QtWidgets>

#include "src/about.h"
#include "src/cli/cli_parser.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_threads.h"
#include "src/crypto/crypto_funcs.h"
#include "src/gui/datovka.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/initialisation.h"
#include "src/io/db_tables.h"
#include "src/io/file_downloader.h"
#include "src/io/filesystem.h"
#include "src/io/message_db_set_container.h"
#include "src/io/tag_db.h"
#include "src/io/records_management_db.h"
#include "src/io/sqlite/db.h"
#include "src/localisation/localisation.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
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

	QDialog *viewDialog = new DlgViewZfo(fileName, Q_NULLPTR);
	viewDialog->exec();

	delete viewDialog;

	return 0;
}

/* ========================================================================= */
static
void loadLocalisation(QApplication &app)
/* ========================================================================= */
{
	static QTranslator qtTranslator, appTranslator;

	QSettings settings(globPref.loadConfPath(),
	    QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	QString language =
	    settings.value("preferences/language").toString();

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

	app.installTranslator(&appTranslator);


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

	app.installTranslator(&qtTranslator);
}

/* ========================================================================= */
/* ========================================================================= */
int main(int argc, char *argv[])
/* ========================================================================= */
/* ========================================================================= */
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
	if (0 != setupCmdLineParser(parser)) {
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
	const QStringList srvcArgs(CLIServiceArgs(parser.optionNames()));

	enum RunMode runMode = RM_GUI;
	QSplashScreen *splash = new QSplashScreen;

	if (!srvcArgs.isEmpty()) {
		runMode = RM_CLI;
	} else if (!cmdLineFileNames.isEmpty()) {
		runMode = RM_ZFO;
	} else {
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
	MainWindow::ensureConfPresence();

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
					logWarning("Couldn't load downloaded "
					    "CRL file '%s'.\n",
					    crl->file_name);
				} else {
					logInfo("Loaded CRL file '%s'.\n",
					    crl->file_name);
				}
			} else {
				logWarning(
				    "Couldn't download CRL file '%s'.\n",
				    crl->file_name);
			}

			++crl;
		}
	}

	if (!SQLiteDb::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    SQLiteDb::dbDriverType.toUtf8().constData());
		/* TODO -- throw a dialog notifying the user. */
		return EXIT_FAILURE;
	}

	logDebugLv0NL("GUI main thread: %p.", QThread::currentThreadId());

	logDebugLv0NL("App path: '%s'.",
	    app.applicationDirPath().toUtf8().constData());

	loadLocalisation(app);

	/* set splash action text */
	if (runMode == RM_GUI) {
		Qt::Alignment align = Qt::AlignCenter;
		splash->showMessage(QObject::tr("Application is loading..."),
		align, Qt::white);
		app.processEvents();
	}

	/* Localise description in tables. */
	accntinfTbl.reloadLocalisedDescription();
	userinfTbl.reloadLocalisedDescription();
	pwdexpdtTbl.reloadLocalisedDescription();
	msgsTbl.reloadLocalisedDescription();
	flsTbl.reloadLocalisedDescription();
	hshsTbl.reloadLocalisedDescription();
	evntsTbl.reloadLocalisedDescription();
	prcstTbl.reloadLocalisedDescription();
	rwmsgdtTbl.reloadLocalisedDescription();
	rwdlvrinfdtTbl.reloadLocalisedDescription();
	smsgdtTbl.reloadLocalisedDescription();
	crtdtTbl.reloadLocalisedDescription();
	msgcrtdtTbl.reloadLocalisedDescription();

	/*
	 * These objects cannot be globally accessible static objects.
	 * The unpredictable order of constructing and destructing these
	 * objects causes segmentation faults upon their destruction.
	 *
	 * TODO -- Solve the problem of this globally accessible structures.
	 */
	{
		globAccountDbPtr = new (std::nothrow) AccountDb("accountDb");
		if (0 == globAccountDbPtr) {
			logErrorNL("%s", "Cannot allocate account db.");
			return EXIT_FAILURE;
		}
		/* Open accounts database. */
		if (!globAccountDbPtr->openDb(globPref.accountDbPath())) {
			logErrorNL("Error opening account db '%s'.",
			    globPref.accountDbPath().toUtf8().constData());
			return EXIT_FAILURE;
		}

		/* Create message DB container. */
		globMessageDbsPtr = new (std::nothrow) DbContainer("GLOBALDBS");
		if (0 == globMessageDbsPtr) {
			logErrorNL("%s",
			    "Cannot allocate message db container.");
			return EXIT_FAILURE;
		}

		globTagDbPtr = new (std::nothrow) TagDb("tagDb");
		if (0 == globTagDbPtr) {
			logErrorNL("%s", "Cannot allocate tag db.");
			return EXIT_FAILURE;
		}
		/* Open tags database. */
		if (!globTagDbPtr->openDb(globPref.tagDbPath())) {
			logErrorNL("Error opening tag db '%s'.",
			    globPref.tagDbPath().toUtf8().constData());
			return EXIT_FAILURE;
		}

		globRecordsManagementDbPtr =
		    new (std::nothrow) RecordsManagementDb("recordsManagementDb");
		if (Q_NULLPTR == globRecordsManagementDbPtr) {
			logErrorNL("%s", "Cannot allocate records management db.");
			return EXIT_FAILURE;
		}
		/* Open records management database. */
		if (!globRecordsManagementDbPtr->openDb(globPref.recordsManagementDbPath())) {
			logErrorNL("Error opening records management db '%s'.",
			    globPref.recordsManagementDbPath().toUtf8().constData());
			return EXIT_FAILURE;
		}
	}

	int ret = EXIT_SUCCESS;

	/* Parse account information. */
	{
		QSettings settings(globPref.loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		AccountModel::globAccounts.loadFromSettings(settings);
	}

	/* Start worker threads. */
	globWorkPool.start();
	logInfo("%s\n", "Worker pool started.");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	// https://blog.qt.io/blog/2016/01/26/high-dpi-support-in-qt-5-6/
	//logInfoNL("%s", "Enabling high DPI scaling.");
	//QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif /* >= Qt-5.6 */

	if (runMode == RM_CLI) {
		delete splash;
		ret = runCLIService(srvcArgs, parser);
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

	if (Q_NULLPTR != globRecordsManagementDbPtr) {
		delete globRecordsManagementDbPtr;
		globRecordsManagementDbPtr = Q_NULLPTR;
	}

	if (0 != globTagDbPtr) {
		delete globTagDbPtr;
		globTagDbPtr = 0;
	}

	if (0 != globMessageDbsPtr) {
		delete globMessageDbsPtr;
		globMessageDbsPtr = 0;
	}
	if (0 != globAccountDbPtr) {
		delete globAccountDbPtr;
		globAccountDbPtr = 0;
	}

	return ret;
}
