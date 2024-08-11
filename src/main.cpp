#include "argparser.h"
#include "mainwindow.h"
#include "wordplay.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

qint32 main(qint32 argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Cofeiini");
    app.setApplicationName("WordplayQt");
    app.setApplicationVersion(APP_VERSION);

    Wordplay wordplay;

    if (argc > 1)
    {
        ArgParser args(app);
        if (args.isSet("h"))
        {
            args.showHelp();
        }

        wordplay.processArguments(args);

        return wordplay.process();
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            app.installTranslator(&translator);
            break;
        }
    }

    MainWindow window(wordplay);
    window.show();

    return app.exec();
}
