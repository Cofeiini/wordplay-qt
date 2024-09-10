#ifndef WORDPLAY_H
#define WORDPLAY_H

#include "argparser.h"

#include <QFile>
#include <QString>

using StrPair = QPair<QString, QString>;

struct Wordplay
{
    static auto extract(QString initial, const QString &word) -> std::optional<QString>;
    auto findAnagrams() -> QSet<QString>;
    auto generateCandidates() -> QList<StrPair>;
    void printAnagram(qint64 index, const QString &out);
    void printStats() const;
    auto process() -> qint32;
    void processArguments(ArgParser &input);
    void processWord(QString &word) const;
    auto readFile() -> QSet<QString>;

    static auto tr(const char *text, const char *disambiguation = nullptr, const int n = -1) -> QString
    {
        return QApplication::translate("Wordplay", text, disambiguation, n);
    }

    struct {
        bool gui = false;

        qint32 silent = SilenceLevel::NONE;
        bool allowDuplicates = false;
        bool includeInput = false;
        bool listCandidates = false;
        bool allowRephrased = false;
        bool recursive = true;
        bool sort = false;
        bool print = false;

        qint64 minimum = 1;
        qint64 maximum = MAX_WORD_LENGTH;
        qint64 depth = DEFAULT_ANAGRAM_WORDS;

        QString letters;
        QString word;

        QString file = DEFAULT_WORD_FILE;
        QString output;
    } args;

    QList<StrPair> candidateWords;
    QList<QString> finalResult;
    QFile outFile;
    QString initialWord;
    QString originalWord;
};

#endif // WORDPLAY_H
