#include <QApplication>
#include <QtWidgets>

#include "datovka.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QString locale = QLocale::system().name();
    qDebug() << locale;
    QTranslator *translator = new QTranslator;
    translator->load(QString("datovka_") + locale, "/home/martin/Git/qdatovka/locale/");
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
