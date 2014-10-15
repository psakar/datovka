

#include <cstdlib>
#include <QApplication>
#include <QCommandLineParser>
#include <QtWidgets>

#include "src/common.h"
#include "src/crypto/crypto.h"
#include "src/gui/datovka.h"
#include "src/io/db_tables.h"
#include "src/io/message_db.h"
#include "src/log/log.h"

#define LOCALE_PATH "locale"

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
	    QObject::tr("Set verbosity of logged messages to <level>. Default is ") +
	    QString::number(globLog.logVerbosity()) + ".",
	    QObject::tr("level"));
	parser.addOption(logVerb);
	/* Boolean options. */
#ifdef DEBUG
	QCommandLineOption debugOpt(QStringList() << "D" << "debug",
	    "Enable debugging information.");
	parser.addOption(debugOpt);
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
#endif /* DEBUG */
	if (parser.isSet(logVerb)) {
		bool ok;
		qDebug() << parser.value(logVerb);
		int value = parser.value(logVerb).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid log-verbosity parameter.");
			exit(1);
		}
		globLog.setLogVerbosity(value);
	}

	qint64 start, stop, diff;
	start = QDateTime::currentMSecsSinceEpoch();
	logInfo("Starting at %lld.%03lld .\n", start / 1000, start % 1000);

	if (0 != init_crypto()) {
		logError("%s\n", "Cannot load cryptographic backend.");
		/* TODO -- throw a dialog notifying the user. */
		/*
		 * TODO -- the function should fail only when all certificates
		 * failed to load.
		 */
		return EXIT_FAILURE;
	}

	if (!dbContainer::dbDriverSupport()) {
		logError("Cannot load database driver '%s'.\n",
		    dbContainer::dbDriverType.toStdString().c_str());
		/* TODO -- throw a dialog notifying the user. */
		return EXIT_FAILURE;
	}

	qDebug() << "GUI main thread: " << QThread::currentThreadId();

	QTranslator translator;

	/* TODO - set language form .dsgui/dsgui.conf */
	QString language;

	if (language == "cs") {
		translator.load("datovka_cs", LOCALE_PATH);
	} else if (language == "en") {
		translator.load("datovka_en", LOCALE_PATH);
	} else {
		// default system
		translator.load("datovka_" + QLocale::system().name(),
		    LOCALE_PATH);
	}

	/* TODO - set Czech locale as default for testing
	 * It must be removed
	 */
	translator.load("datovka_cs", LOCALE_PATH);
	app.installTranslator(&translator);

	/* Localise description in tables. */
	accntinfTbl.reloadLocalisedDescription();
	pwdexpdtTbl.reloadLocalisedDescription();
	msgsTbl.reloadLocalisedDescription();
	msgsTbl.reloadLocalisedDescription();
	flsTbl.reloadLocalisedDescription();
	hshsTbl.reloadLocalisedDescription();
	evntsTbl.reloadLocalisedDescription();
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
