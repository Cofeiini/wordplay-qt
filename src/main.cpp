#include "argparser.h"
#include "mainwindow.h"
#include "wordplay.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

auto main(qint32 argc, char *argv[]) -> qint32
{
    // NOLINTBEGIN(*-static-accessed-through-instance)

    const QApplication app(argc, argv);
    app.setOrganizationName("Cofeiini");
    app.setApplicationName("WordplayQt");
    app.setApplicationVersion(APP_VERSION);

    Wordplay wordplay;

    QTranslator translator;
    for (const QString &locale : QLocale::system().uiLanguages())
    {
        if (translator.load(QString(":/i18n/wordplay_%1").arg(locale)))
        {
            app.installTranslator(&translator);
            break;
        }
    }

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

    MainWindow window(wordplay);
    window.show();

    return app.exec();

    // NOLINTEND(*-static-accessed-through-instance)
}
