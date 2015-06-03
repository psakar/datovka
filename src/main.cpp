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

#include "src/common.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_threads.h"
#include "src/crypto/crypto_funcs.h"
#include "src/gui/datovka.h"
#include "src/io/db_tables.h"
#include "src/io/file_downloader.h"
#include "src/io/message_db.h"
#include "src/log/log.h"

#define CONF_SUBDIR_OPT "conf-subdir"
#define LOAD_CONF_OPT "load-conf"
#define SAVE_CONF_OPT "save-conf"


/* ========================================================================= */
/* ========================================================================= */
int main(int argc, char *argv[])
/* ========================================================================= */
/* ========================================================================= */
{
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
	QCoreApplication::setApplicationVersion(VERSION);

	qInstallMessageHandler(globalLogOutput);

	QApplication app(argc, argv);

	QCommandLineParser parser;
	parser.setApplicationDescription(QObject::tr("Data box application"));
	parser.addHelpOption();
	parser.addVersionOption();
	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(CONF_SUBDIR_OPT,
	        QObject::tr(
	            "Use <conf-subdir> subdirectory for configuration."),
	        QObject::tr("conf-subdir")))) {
		Q_ASSERT(0);
	}
	if (!parser.addOption(QCommandLineOption(LOAD_CONF_OPT,
	        QObject::tr("On start load <conf> file."),
	        QObject::tr("conf")))) {
		Q_ASSERT(0);
	}
	if (!parser.addOption(QCommandLineOption(SAVE_CONF_OPT,
	        QObject::tr("On stop save <conf> file."),
	        QObject::tr("conf")))) {
		Q_ASSERT(0);
	}
	QCommandLineOption logVerb(QStringList() << "L" << "log-verbosity",
	    QObject::tr("Set verbosity of logged messages to <level>. "
	        "Default is ") + QString::number(globLog.logVerbosity()) + ".",
	    QObject::tr("level"));
	parser.addOption(logVerb);
	/* Boolean options. */
#ifdef DEBUG
	QCommandLineOption debugOpt(QStringList() << "D" << "debug",
	    "Enable debugging information.");
	parser.addOption(debugOpt);
	QCommandLineOption debugVerb(QStringList() << "V" << "debug-verbosity",
	    QObject::tr("Set debugging verbosity to <level>. Default is ") +
	    QString::number(globLog.debugVerbosity()) + ".",
	    QObject::tr("level"));
	parser.addOption(debugVerb);
#endif /* DEBUG */
	/* Process command-line arguments. */
	parser.process(app);

	if (parser.isSet(CONF_SUBDIR_OPT)) {
		globPref.confSubdir = parser.value(CONF_SUBDIR_OPT);
	}
	if (parser.isSet(LOAD_CONF_OPT)) {
		globPref.loadFromConf = parser.value(LOAD_CONF_OPT);
	}
	if (parser.isSet(SAVE_CONF_OPT)) {
		globPref.saveToConf = parser.value(SAVE_CONF_OPT);
	}
#ifdef DEBUG
	if (parser.isSet(debugOpt)) {
		globLog.setLogLevels(GlobLog::LF_STDERR, LOGSRC_ANY,
		    LOG_UPTO(LOG_DEBUG));
	}
	if (parser.isSet(debugVerb)) {
		globLog.setLogLevels(GlobLog::LF_STDERR, LOGSRC_ANY,
		    LOG_UPTO(LOG_DEBUG));
		bool ok;
		int value = parser.value(debugVerb).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid debug-verbosity parameter.");
			exit(1);
		}
		logInfo("Setting debugging verbosity to value '%d'.\n", value);
		globLog.setDebugVerbosity(value);
	}
#endif /* DEBUG */
	if (parser.isSet(logVerb)) {
		bool ok;
		int value = parser.value(logVerb).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid log-verbosity parameter.");
			exit(1);
		}
		logInfo("Setting logging verbosity to value '%d'.\n", value);
		globLog.setLogVerbosity(value);
	}

	qint64 start, stop, diff;
	start = QDateTime::currentMSecsSinceEpoch();
	logInfo("Starting at %lld.%03lld .\n", start / 1000, start % 1000);

	if (0 != crypto_init()) {
		logError("%s\n", "Cannot load cryptographic back-end.");
		/* TODO -- throw a dialog notifying the user. */
		/*
		 * TODO -- the function should fail only when all certificates
		 * failed to load.
		 */
		return EXIT_FAILURE;
	}
	crypto_init_threads();

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

	if (!DbContainer::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    DbContainer::dbDriverType.toUtf8().constData());
		/* TODO -- throw a dialog notifying the user. */
		return EXIT_FAILURE;
	}

	qDebug() << "GUI main thread: " << QThread::currentThreadId();

	QTranslator qtTranslator, appTranslator;

	qDebug() << "App path : " << app.applicationDirPath();

	/* Load localisation. */
	{
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

	MainWindow mainwin;
	mainwin.show();

	int ret;
	ret = app.exec();

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
