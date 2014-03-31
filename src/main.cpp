

#include <QApplication>
#include <QtWidgets>

#include "gui/datovka.h"

#define LOCALE_PATH "locale"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QString locale = QLocale::system().name();
    QTranslator *translator = new QTranslator;
    translator->load(QString("datovka_") + locale, LOCALE_PATH);
    app.installTranslator(translator);
/*
#if defined(Q_OS_UNIX)
#elif defined(Q_OS_WIN)
#elif defined(Q_OS_MAC)
#endif
*/
    MainWindow mainwin;
    mainwin.show();

    return app.exec();
}
