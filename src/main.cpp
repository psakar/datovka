

#include <QApplication>
#include <QtWidgets>

#include "gui/datovka.h"

#define LOCALE_PATH "locale"


/* ========================================================================= */
/* ========================================================================= */
int main(int argc, char *argv[])
/* ========================================================================= */
/* ========================================================================= */
{
	QApplication app(argc, argv);
	QTranslator translator;
	int ret;


	translator.load(QString("datovka_") + QLocale::system().name(),
	    LOCALE_PATH);
	app.installTranslator(&translator);
/*
#if defined(Q_OS_UNIX)
#elif defined(Q_OS_WIN)
#elif defined(Q_OS_MAC)
#endif
*/
	MainWindow mainwin;
	mainwin.show();

	ret = app.exec();

	return ret;
}
