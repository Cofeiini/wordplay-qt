#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <QApplication>
#include <QCommandLineParser>
#include <QDate>

constexpr auto DEFAULT_WORD_FILE = ":/words/en-US.txt";
constexpr qint32 MAX_WORD_LENGTH = 128;
constexpr qint32 MAX_ANAGRAM_WORDS = 32;
constexpr qint32 DEFAULT_ANAGRAM_WORDS = 4;

namespace SilenceLevel {
    enum {
        NONE,
        INFO,
        NUMBERS,
        RESULTS,
        STATS,
        ALL,
    };
}

class ArgParser : public QCommandLineParser
{
public:
    explicit ArgParser(const QApplication &app)
    {
        const QHash<int, QString> silenceMap {
            { SilenceLevel::INFO, tr("no info messages") },
            { SilenceLevel::NUMBERS, tr("no line numbers") },
            { SilenceLevel::RESULTS, tr("no results") },
            { SilenceLevel::STATS, tr("no stats") },
        };
        QList<QString> silenceList;
        for (qint32 i = SilenceLevel::NONE + 1; i < SilenceLevel::ALL; ++i)
        {
            silenceList.push_back(QString("%1 = %2").arg(i).arg(silenceMap.value(i)));
        }

        const QString build = tr("Build %1").arg(QDate::fromString(__DATE__, "MMM d yyyy").toString("yyyy-MM-dd"));
        const QString credit = tr("original by Evans A Criswell");
        setApplicationDescription(QObject::tr("WordplayQt %1 (%2).").arg(build, credit));

        addHelpOption();
        addVersionOption();

        addPositionalArgument("word_to_anagram", tr("Input word for finding anagrams."));
        addOption({ { "a", "allow-multiple" }, tr("Allow multiple occurrences of a word in an anagram") });
        addOption({ { "c", "characters" }, tr("<letters> to include in words (useful with l option)"), tr("letters"), "" });
        addOption({ { "d", "depth" }, tr("Limit anagrams to <amount> of words"), tr("amount"), QString::number(MAX_ANAGRAM_WORDS) });
        addOption({ { "f", "file" }, tr(R"(<file> to use as word list ("-" for stdin))"), tr("file"), DEFAULT_WORD_FILE });
        addOption({ { "i", "include" }, tr("Include the input word in anagram list") });
        addOption({ { "l", "list" }, tr("List candidate words") });
        addOption({ { "m", "max", "maximum" }, tr("Candidate words must have a <maximum> number of characters"), tr("maximum"), QString::number(MAX_WORD_LENGTH) });
        addOption({ { "n", "min", "minimum" }, tr("Candidate words must have a <minimum> number of characters"), tr("minimum"), "1" });
        addOption({ { "o", "output" }, tr("<file> to use for writing results (can be used with s and z options)"), tr("file"), "" });
        addOption({ { "p", "print" }, tr("Print results immediately (ignores z option)") });
        addOption({ { "r", "rephrase" }, tr("Allow results to contain the same words in different order") });
        addOption({ { "s", "silent" }, tr("Silence <level> (%1)").arg(silenceList.join(", ")), tr("level"), "0" });
        addOption({ { "w", "word" }, tr("<word> to include in anagrams (assumes i option)"), tr("word"), "" });
        addOption({ { "x", "no-generate" }, tr("Do not generate anagrams (useful with l option)") });
        addOption({ { "z", "sort" }, tr("Sort the final results") });

        process(app);
    }
};

#endif // ARGPARSER_H
