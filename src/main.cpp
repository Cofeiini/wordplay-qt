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
    for (const QString &locale : QLocale::system().uiLanguages())
    {
        if (translator.load(":/i18n/" + QLocale(locale).name()))
        {
            app.installTranslator(&translator);
            break;
        }
    }

    MainWindow window(wordplay);
    window.show();

    return app.exec();

    // NOLINTEND(*-static-accessed-through-instance)
}
