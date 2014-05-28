

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

	qint64 start, stop, diff;
	start = QDateTime::currentMSecsSinceEpoch();
	logInfo("Starting at %lld.%03d .\n", start / 1000, (int) start % 1000);
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
	logInfo("Stopping at %lld.%03d; ran for %lld.%03d seconds.\n",
	    stop / 1000, (int) stop % 1000,
	    diff / 1000, (int) diff % 1000);

	return ret;
}
