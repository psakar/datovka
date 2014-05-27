

#include <QApplication>
#include <QCommandLineParser>
#include <QtWidgets>

#include "common.h"
#include "gui/datovka.h"

#define LOCALE_PATH "locale"

#define LOAD_CONF_OPT "load-conf"
#define SAVE_CONF_OPT "save-conf"

/* ========================================================================= */
/* ========================================================================= */
int main(int argc, char *argv[])
/* ========================================================================= */
/* ========================================================================= */
{
	QApplication app(argc, argv);
	/* TODO -- Make the following assignments configurable. */
	QCoreApplication::setApplicationName("qdatovka");
	QCoreApplication::setApplicationVersion(VERSION);

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
	/* Process command-line arguments. */
	parser.process(app);

	if (parser.isSet(LOAD_CONF_OPT)) {
		globPref.loadFromConf = parser.value(LOAD_CONF_OPT);
	}
	if (parser.isSet(SAVE_CONF_OPT)) {
		globPref.saveToConf = parser.value(SAVE_CONF_OPT);
	}
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
	return ret;
}
