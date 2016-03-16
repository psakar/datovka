/*
 * Copyright (C) 2014-2015 CZ.NIC
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
#include <cstdlib>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QtWidgets>

#include "src/cli/cli.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_threads.h"
#include "src/crypto/crypto_funcs.h"
#include "src/gui/datovka.h"
#include "src/gui/dlg_about.h"
#include "src/gui/dlg_view_zfo.h"
#include "src/io/db_tables.h"
#include "src/io/file_downloader.h"
#include "src/io/filesystem.h"
#include "src/io/message_db_set_container.h"
#include "src/io/tag_db.h"
#include "src/io/sqlite/db.h"
#include "src/log/log.h"
#include "src/models/accounts_model.h"
#include "src/settings/proxy.h"
#include "src/single/single_instance.h"
#include "src/worker/pool.h"

#define CONF_SUBDIR_OPT "conf-subdir"
#define LOAD_CONF_OPT "load-conf"
#define SAVE_CONF_OPT "save-conf"
#define LOG_FILE "log-file"

#define LOG_VERBOSITY_OPT "log-verbosity"
#define DEBUG_OPT "debug"
#define DEBUG_VERBOSITY_OPT "debug-verbosity"


/* ========================================================================= */
static
int doCLI(const QStringList &serList, const QCommandLineParser &parser)
/* ========================================================================= */
{
	int ret = CLI_EXIT_ERROR;
	QString errmsg;
	QString serName;
	int index = 0;
	QTextStream cout(stderr);

	// every valid CLI action must have only one login parameter
	// or one login parameter and one name service
	switch (serList.count()) {
	case 0:
		errmsg = "No service has been defined for CLI action!";
		break;
	case 1:
		if (serList.contains(SER_LOGIN)) {
			ret = runService(parser.value(SER_LOGIN),
			    NULL, NULL);
			return ret;
		} else {
			errmsg = "Only service name was set. "
			    "Login parameter is missing!";
		}
		break;
	case 2:
		if (serList.contains(SER_LOGIN)) {
			index = serList.indexOf(SER_LOGIN);
			if (index == 0) {
				serName = serList.at(1);
			} else {
				serName = serList.at(0);
			}
			ret = runService(parser.value(SER_LOGIN),
			    serName, parser.value(serName));
			return ret;
		} else {
			errmsg = "Login parameter is missing! "
			    "Maybe two service names were set.";
		}
		break;
	default:
		errmsg = "More than two service names or logins were set! "
		    "This situation is not allowed.";
		break;
	}

	// print error to stderr
	cout << CLI_PREFIX << " error(" << CLI_ERROR << ") : "
	    << errmsg << endl;

	return ret;
}

/* ========================================================================= */
static
int showZfo(const QString &fileName)
/* ========================================================================= */
{
	if (fileName.isEmpty()) {
		return -1;
	}

	QDialog *viewDialog = new DlgViewZfo(fileName, 0);
	viewDialog->exec();

	delete viewDialog;

	return 0;
}

/* ========================================================================= */
static
int setupCmdLineParser(QCommandLineParser &parser)
/* ========================================================================= */
{
	parser.setApplicationDescription(QObject::tr("Data box application"));
	parser.addHelpOption();
	parser.addVersionOption();
	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(CONF_SUBDIR_OPT,
	        QObject::tr(
	            "Use <conf-subdir> subdirectory for configuration."),
	        QObject::tr("conf-subdir")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOAD_CONF_OPT,
	        QObject::tr("On start load <conf> file."),
	        QObject::tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SAVE_CONF_OPT,
	        QObject::tr("On stop save <conf> file."),
	        QObject::tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOG_FILE,
	        QObject::tr("Log messages to <file>."),
	        QObject::tr("file")))) {
		return -1;
	}
	QCommandLineOption logVerb(QStringList() << "L" << LOG_VERBOSITY_OPT,
	    QObject::tr("Set verbosity of logged messages to <level>. "
	        "Default is ") + QString::number(globLog.logVerbosity()) + ".",
	    QObject::tr("level"));
	if (!parser.addOption(logVerb)) {
		return -1;
	}
	/* Boolean options. */
#ifdef DEBUG
	QCommandLineOption debugOpt(QStringList() << "D" << DEBUG_OPT,
	    "Enable debugging information.");
	if (!parser.addOption(debugOpt)) {
		return -1;
	}
	QCommandLineOption debugVerb(QStringList() << "V" << DEBUG_VERBOSITY_OPT,
	    QObject::tr("Set debugging verbosity to <level>. Default is ") +
	    QString::number(globLog.debugVerbosity()) + ".",
	    QObject::tr("level"));
	if (!parser.addOption(debugVerb)) {
		return -1;
	}
#endif /* DEBUG */

	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(SER_LOGIN,
	        QObject::tr("Service: connect to isds and login into databox."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG_LIST,
	        QObject::tr("Service: download list of received/sent "
	        "messages from ISDS."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_SEND_MSG,
	        QObject::tr("Service: create and send a new message to ISDS."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG,
	        QObject::tr("Service: download complete message with "
	        "signature and time stamp of MV."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_DEL_INFO,
	        QObject::tr("Service: download delivery info of message "
	        "with signature and time stamp of MV."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_USER_INFO,
	        QObject::tr("Service: get information about user "
	        "(role, privileges, ...)."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_OWNER_INFO,
	        QObject::tr("Service: get information about owner and "
	        "its databox."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_CHECK_ATTACHMENT,
	        QObject::tr("Service: get list of messages where "
	        "attachment missing (local database only)."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_FIND_DATABOX,
	        QObject::tr("Service: find a databox via several parameters."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}

	parser.addPositionalArgument("[zfo-file]",
	    QObject::tr("ZFO file to be viewed."));

	return 0;
}

/* ========================================================================= */
static
int setupPreferences(const QCommandLineParser &parser,
    GlobPreferences &prefs, GlobLog &log)
/* ========================================================================= */
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
		    GlobLog::LM_APPEND);
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
		log.setLogLevels(GlobLog::LF_STDERR, LOGSRC_ANY,
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

	localisationFile = "datovka_";

	if (language == "cs") {
		localisationFile += "cs";
		programLocale = QLocale(QLocale::Czech, QLocale::CzechRepublic);
	} else if (language == "en") {
		localisationFile += "en";
		programLocale = QLocale(QLocale::English, QLocale::UnitedKingdom);
	} else {
		/* Use system locale. */
		localisationFile += QLocale::system().name();
		programLocale = QLocale::system();
	}

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

	localisationFile = "qtbase_";

	if (language == "cs") {
		localisationFile += "cs";
	} else if (language == "en") {
		localisationFile = QString();
	} else {
		/* Use system locale. */
		localisationFile += QLocale::system().name();
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
	globLog.setLogLevels(GlobLog::LF_STDERR, LOGSRC_ANY,
	    LOG_UPTO(LOG_WARNING));

#ifndef WIN32
	{
		QProcessEnvironment env =
		    QProcessEnvironment::systemEnvironment();
		QString lang = env.value("LANG");
		if (lang.isEmpty() || ("c" == lang.toLower())) {
#define LANG_DEF "en_GB.UTF-8"
			logWarning("The LANG environment variable "
			    "is missing or unset. Setting to '%s'.\n",
			    LANG_DEF);
			if (NULL == setlocale(LC_ALL, LANG_DEF)) {
				logError("Setting locale to '%s' failed.\n",
				    LANG_DEF);
			}
#undef LANG_DEF
		}
	}
#endif /* !WIN32 */

	/* TODO -- Make the following assignments configurable. */
	QCoreApplication::setApplicationName("Datovka");
	QCoreApplication::setApplicationVersion(VERSION " -- [" +
	    DlgAbout::libraryDependencies().join("; ") + "]");

	qInstallMessageHandler(globalLogOutput);

	QApplication app(argc, argv);

	QCommandLineParser parser;
	if (0 != setupCmdLineParser(parser)) {
		logErrorNL("%s", "Cannot set up command-line parser.");
		return EXIT_FAILURE;
	}

	/* Process command-line arguments. */
	parser.process(app);

	if (0 != setupPreferences(parser, globPref, globLog)) {
		logErrorNL("%s",
		    "Cannot apply command line arguments on global settings.");
		return EXIT_FAILURE;
	}

	QStringList cmdLineFileNames = parser.positionalArguments();

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

	{
		/* Obey proxy settings. */
		{
			QSettings settings(globPref.loadConfPath(),
			    QSettings::IniFormat);
			settings.setIniCodec("UTF-8");
			globProxSet.loadFromSettings(settings);
		}

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
	AccountDb globAccountDb("accountDb");
	globAccountDbPtr = &globAccountDb;
	DbContainer globMessageDbs("GLOBALDBS");
	globMessageDbsPtr = &globMessageDbs;

	{
		/* Open accounts database. */
		if (!globAccountDbPtr->openDb(globPref.accountDbPath())) {
			logErrorNL("Error opening account db '%s'.",
			    globPref.accountDbPath().toUtf8().constData());
			return EXIT_FAILURE;
		}
	}

	TagDb globTagDb("tagDb");
	globTagDbPtr = &globTagDb;

	{
		/* Open tags database. */
		if (!globTagDbPtr->openDb(globPref.tagDbPath())) {
			logErrorNL("Error opening tag db '%s'.",
			    globPref.tagDbPath().toUtf8().constData());
			return EXIT_FAILURE;
		}
	}


	int ret = EXIT_SUCCESS;

	/* Parse account information. */
	{
		QSettings settings(globPref.loadConfPath(),
		    QSettings::IniFormat);
		settings.setIniCodec("UTF-8");
		globProxSet.loadFromSettings(settings);
		AccountModel::globAccounts.loadFromSettings(settings);
	}

	/* Start worker threads. */
	globWorkPool.start();
	logInfo("%s\n", "Worker pool started.");

	QStringList inOptions = parser.optionNames();
	QStringList serList;

	// check if any CLI service was set from command line
	for (int i = 0; i < inOptions.count(); ++i) {
		for (int j = 0; j < serviceList.count(); ++j) {
			if (inOptions.at(i) == serviceList.at(j)) {
				serList.append(inOptions.at(i));
				break;
			}
		}
	}

	// if any CLI service was set run CLI
	if (!serList.isEmpty()) {
		ret = doCLI(serList, parser);
	} else if (cmdLineFileNames.isEmpty()) {
		MainWindow mainwin;
		mainwin.show();
		ret = (0 == app.exec()) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else {
		foreach (const QString &fileName, cmdLineFileNames) {
			ret = showZfo(fileName);
			if (0 != ret) {
				break;
			}
		}
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

	return ret;
}
