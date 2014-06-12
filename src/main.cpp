

#include <QApplication>
#include <QCommandLineParser>
#include <QtWidgets>

#include "src/common.h"
#include "src/gui/datovka.h"
#include "src/log/log.h"

#define LOCALE_PATH "locale"

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
	QCoreApplication::setApplicationName("qdatovka");
	QCoreApplication::setApplicationVersion(VERSION);

	qInstallMessageHandler(globalLogOutput);

	QApplication app(argc, argv);

	QTranslator translator;
	translator.load("datovka_" + QLocale::system().name(), LOCALE_PATH);
	app.installTranslator(&translator);

	QCommandLineParser parser;
	parser.setApplicationDescription(QObject::tr("Data box application"));
	parser.addHelpOption();
	parser.addVersionOption();
	/* Options with values. */
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
/*
#if defined(Q_OS_UNIX)
#elif defined(Q_OS_WIN)
#elif defined(Q_OS_MAC)
#endif
*/
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
