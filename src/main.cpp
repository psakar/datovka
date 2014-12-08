

#include <cstdlib>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QtWidgets>

#include "src/common.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_threadsafe.h"
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

	if (0 != cryptoInit()) {
		logError("%s\n", "Cannot load cryptographic back-end.");
		/* TODO -- throw a dialog notifying the user. */
		/*
		 * TODO -- the function should fail only when all certificates
		 * failed to load.
		 */
		return EXIT_FAILURE;
	}

	{
		/* TODO -- Obey proxy settings. */

		/* Start downloading the CRL files. */
		QList<QUrl> urlList;
		FileDownloader fDown;
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
				if (0 != cryptoAddCrl(data.data(),
				        data.size())) {
					logWarning("Couldn't load downloaded "
					    "CRL file '%s'.\n", crl->file_name);
				}
			} else {
				logWarning(
				    "Couldn't download CRL file '%s'.\n",
				    crl->file_name);
			}

			++crl;
		}
	}

	if (!dbContainer::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    dbContainer::dbDriverType.toStdString().c_str());
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

		/* Check for localisation location. */
		QString localisationDir = suppliedLocalisationDir();

		logInfo("Loading localisation from path '%s'.\n",
		    localisationDir.toStdString().c_str());

		QString qtLocalisation, datovkaLocalisation;

		if (language == "cs") {
			datovkaLocalisation = "datovka_cs";
			qtLocalisation = "qtbase_cs";
		} else if (language == "en") {
			datovkaLocalisation = "datovka_en";
			qtLocalisation = "qtbase_uk";
		} else {
			/* Use system locale. */
			datovkaLocalisation =
			    "datovka_" + QLocale::system().name();
			qtLocalisation = "qtbase_" + QLocale::system().name();
		}

		if (!appTranslator.load(datovkaLocalisation,
		        localisationDir)) {
			logWarning("Could not load '%s' from '%s'.\n",
			    datovkaLocalisation.toStdString().c_str(),
			    localisationDir.toStdString().c_str());
		}

		if (!qtTranslator.load(qtLocalisation, localisationDir)) {
			logWarning("Could not load '%s' from '%s'.\n",
			    qtLocalisation.toStdString().c_str(),
			    localisationDir.toStdString().c_str());
		}

		app.installTranslator(&qtTranslator);
		app.installTranslator(&appTranslator);
	}

	/* Localise description in tables. */
	accntinfTbl.reloadLocalisedDescription();
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

	return ret;
}
