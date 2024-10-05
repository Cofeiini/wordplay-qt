#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QStandardPaths>

auto main(qint32 argc, char *argv[]) -> qint32
{
    // NOLINTBEGIN(*-static-accessed-through-instance)

    const QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Cofeiini"));
    app.setApplicationName(QStringLiteral("WordplayQt"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));

    const auto wordsPath = QStringLiteral("%1/words").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (QDir().mkpath(wordsPath))
    {
        QFile defaultFile(QStringLiteral("%1/en-US.txt").arg(wordsPath));
        if (!defaultFile.exists())
        {
            if (defaultFile.open(QFile::Text | QFile::ReadWrite | QFile::Truncate))
            {
                QFile embeddedFile(QStringLiteral(":/words/en-US.txt"));
                if (embeddedFile.open(QFile::Text | QFile::ReadOnly))
                {
                    defaultFile.write(embeddedFile.readAll());
                }
            }
        }
    }

    Wordplay wordplay;

    for (const QString &locale : QLocale::system().uiLanguages())
    {
        if (wordplay.translator.load(QStringLiteral(":/i18n/wordplay_%1").arg(locale)))
        {
            app.installTranslator(&wordplay.translator);
            break;
        }
    }

    if (argc > 1)
    {
        ArgParser args(app);
        if (args.isSet(QStringLiteral("h")))
        {
            args.showHelp(); // This will exit the program with EXIT_SUCCESS
        }

        wordplay.processArguments(args);

        return wordplay.process();
    }

    MainWindow window(wordplay);
    window.show();

    return app.exec();

    // NOLINTEND(*-static-accessed-through-instance)
}
