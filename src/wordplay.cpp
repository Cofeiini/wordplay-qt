#include "wordplay.h"

#include <QFileInfo>

#include <algorithm>
#include <ranges>

auto Wordplay::extract(QString initial, const QString &word) -> std::optional<QString>
{
    for (const QChar &character : word)
    {
        if (character.isPunct())
        {
            continue;
        }

        const auto found = initial.indexOf(character, 0, Qt::CaseInsensitive);
        if (found == -1)
        {
            return {};
        }

        initial.removeAt(found);
    }

    return initial;
}

auto Wordplay::findAnagrams() -> QSet<QString>
{
    QStringList acc;

    if (!args.word.isEmpty())
    {
        const auto temp = extract(initialWord, args.word);
        if (!temp)
        {
            if (!args.gui)
            {
                qInfo("");
                qCritical("%s", qUtf8Printable(tr(R"([%1] Specified word "%2" cannot be extracted from initial string "%3".)").arg(tr("Error"), args.word, originalWord)));
                quick_exit(EXIT_FAILURE);
            }

            return {};
        }

        if (temp->isEmpty())
        {
            if (args.silent < SilenceLevel::NUMBERS)
            {
                qInfo("");
                qInfo("%s", qUtf8Printable(tr("Anagrams found:")));
            }
            printAnagram(1, args.word);

            if (!args.gui)
            {
                quick_exit(EXIT_SUCCESS);
            }

            return { args.word };
        }

        acc.push_back(args.word);
        initialWord = temp.value();
    }

    candidateWords = generateCandidates();
    const qint64 candidateSize = candidateWords.size();

    if (candidateSize == 0 || !args.recursive)
    {
        return {};
    }

    QHash<QChar, QPair<qint64, qint64>> candidateRanges;
    qint64 pos = 0;
    while (pos < candidateSize)
    {
        const qint64 start = pos;
        const QChar &letter = candidateWords.at(pos).second.at(0);
        while (++pos < candidateSize)
        {
            if (candidateWords.at(pos).second.at(0) != letter)
            {
                break;
            }
        }

        qint64 end = pos - 1;
        if (pos >= candidateSize)
        {
            end = candidateSize - 1;
        }

        candidateRanges[letter] = std::make_pair(start, end);
    }

    if (args.print && (args.silent < SilenceLevel::NUMBERS))
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("Anagrams found:")));
    }

    QSet<QString> result;
    qint64 anagramCount = 0;
    const std::function<void(const QString &, qint64)> recurse = [&](const QString &remaining, const qint64 iterator) {
        if (acc.size() > args.depth)
        {
            return;
        }

        if (remaining.isEmpty())
        {
            QList temp(acc.begin(), acc.end());
            if (args.allowRephrased)
            {
                std::ranges::sort(temp);
            }

            do
            {
                const auto out = temp.join(' ');
                if (args.print)
                {
                    printAnagram(++anagramCount, out);
                    continue;
                }

                result.insert(out);
            } while (args.allowRephrased && std::ranges::next_permutation(temp).found);

            return;
        }

        const auto [start, end] = candidateRanges.value(remaining.at(0));
        for (qint64 i = std::max(iterator, start); i <= end; ++i)
        {
            const auto &[candidate, normalized] = candidateWords.at(i);

            if (!args.allowDuplicates && !acc.empty() && acc.contains(candidate))
            {
                continue;
            }

            const auto extracted = extract(remaining, normalized);
            if (!extracted)
            {
                continue;
            }

            acc.append(candidate);
            recurse(extracted.value(), i + !args.allowDuplicates);
            acc.removeLast();
        }
    };
    recurse(initialWord, 0);

    if (args.print)
    {
        quick_exit(EXIT_SUCCESS);
    }

    return result;
}

auto Wordplay::generateCandidates() -> QList<StrPair>
{
    const auto candidates = readFile();

    QList<StrPair> words;
    for (const auto &word: candidates)
    {
        QString normalized = word;
        std::ranges::sort(normalized);
        words.emplace_back(word, normalized);
    }

    if (!args.includeInput)
    {
        for (auto iter = words.cbegin(); iter != words.cend(); ++iter)
        {
            if (iter->first == originalWord)
            {
                words.erase(iter);
                break;
            }
        }
    }
    std::ranges::sort(words, [](const StrPair &left, const StrPair &right) { return left.second < right.second; });

    if (args.listCandidates && (args.silent < SilenceLevel::RESULTS))
    {
        const qint64 candidateSize = words.size();

        QStringList output;
        output.reserve(candidateSize);
        for (const auto &word: std::ranges::views::keys(words))
        {
            output.push_back(word);
        }
        std::ranges::sort(output, [](const QString &left, const QString &right) { return (left.size() != right.size()) ? (left.size() < right.size()) : (left < right); });

        if (args.silent < SilenceLevel::NUMBERS)
        {
            qInfo("");
            qInfo("%s", qUtf8Printable(tr("List of candidate words:")));
        }

        for (qint64 i = 0; i < candidateSize; ++i)
        {
            printAnagram(i + 1, output.at(i));
        }
    }

    if (!args.recursive && !args.gui)
    {
        quick_exit(EXIT_SUCCESS);
    }

    return words;
}

void Wordplay::printAnagram(const qint64 index, const QString &out)
{
    QString result;
    if (args.silent < SilenceLevel::NUMBERS)
    {
        result = QStringLiteral("%1. ").arg(index, 8, 10, QChar(' '));
    }
    result += out;

    if (outFile.isOpen())
    {
        QTextStream(&outFile) << result << Qt::endl;
    }

    if (args.silent >= SilenceLevel::RESULTS)
    {
        return;
    }

    qInfo("%s", qUtf8Printable(result));
}

void Wordplay::printStats() const
{
    if (args.silent < SilenceLevel::STATS)
    {
        qInfo("%-32s%s", qUtf8Printable(tr("Include input word in results:")), qUtf8Printable(args.includeInput ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Candidate word list:")), qUtf8Printable(args.listCandidates ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Anagram generation:")), qUtf8Printable(args.recursive ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Allow duplicate words:")), qUtf8Printable(args.allowDuplicates ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Sort the results:")), qUtf8Printable(args.sort ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Rephrased results:")), qUtf8Printable(args.allowRephrased ? tr("yes") : tr("no")));
        qInfo("%-32s%s", qUtf8Printable(tr("Print immediately:")), qUtf8Printable(args.print ? tr("yes") : tr("no")));

        qInfo("");
        qInfo("%-32s%d", qUtf8Printable(tr("Silence level:")), args.silent);
        qInfo("%-32s%lld", qUtf8Printable(tr("Max anagram depth:")), args.depth);
        qInfo("%-32s%lld", qUtf8Printable(tr("Maximum word length:")), args.maximum);
        qInfo("%-32s%lld", qUtf8Printable(tr("Minimum word length:")), args.minimum);

        qInfo("");
        if (!args.letters.isEmpty())
        {
            qInfo(R"(%-32s"%s")", qUtf8Printable(tr("Included characters:")), qUtf8Printable(args.letters));
        }
        if (!args.word.isEmpty())
        {
            qInfo(R"(%-32s"%s")", qUtf8Printable(tr("Included word:")), qUtf8Printable(args.word));
        }
        if (args.file != QStringLiteral("-") && args.file != DEFAULT_WORD_FILE)
        {
            qInfo(R"(%-32s"%s")", qUtf8Printable(tr("Word list file:")), qUtf8Printable(args.file));
        }
        if (!args.output.isEmpty())
        {
            qInfo(R"(%-32s"%s")", qUtf8Printable(tr("Output file:")), qUtf8Printable(args.output));
        }
        qInfo(R"(%-32s"%s")", qUtf8Printable(tr("String to anagram:")), qUtf8Printable(initialWord));
    }
}

auto Wordplay::process() -> qint32
{
    if (!args.output.isEmpty())
    {
        outFile.setFileName(args.output);
        if (!outFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            qWarning("%s", qUtf8Printable(tr(R"([%1] Failed to open "%2" for writing.)").arg(tr("Warning"), args.output)));
            return EXIT_FAILURE;
        }
    }

    std::ranges::transform(initialWord, initialWord.begin(), [](const QChar character) { return character.toUpper(); });
    std::ranges::transform(args.word, args.word.begin(), [](const QChar character) { return character.toUpper(); });
    std::ranges::transform(args.letters, args.letters.begin(), [](const QChar character) { return character.toUpper(); });

    printStats();

    originalWord = initialWord;
    std::ranges::sort(initialWord);

    const auto result = findAnagrams();
    if (result.isEmpty())
    {
        if (args.silent < SilenceLevel::INFO)
        {
            qInfo("%s", qUtf8Printable(tr(R"([%1] No anagrams for "%2" were found.)").arg(tr("Info"), originalWord)));
        }

        return EXIT_SUCCESS;
    }
    finalResult.assign(result.begin(), result.end());

    if (args.sort)
    {
        std::ranges::sort(finalResult);
    }

    if (args.silent < SilenceLevel::NUMBERS)
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("Anagrams found:")));
    }

    const qint64 finalSize = finalResult.size();
    for (qint64 i = 0; i < finalSize; ++i)
    {
        printAnagram(i + 1, finalResult.at(i));
    }

    return EXIT_SUCCESS;
}

void Wordplay::processArguments(ArgParser &input)
{
    initialWord = input.positionalArguments().at(0);
    processWord(initialWord);
    if (initialWord.isEmpty())
    {
        input.showHelp();
    }

    args.silent = input.value(QStringLiteral("s")).toInt();
    args.includeInput = input.isSet(QStringLiteral("i"));
    args.listCandidates = input.isSet(QStringLiteral("l"));
    args.recursive = !input.isSet(QStringLiteral("x"));
    args.allowDuplicates = input.isSet(QStringLiteral("a"));
    args.sort = input.isSet(QStringLiteral("z"));
    args.allowRephrased = input.isSet(QStringLiteral("r"));
    args.print = input.isSet(QStringLiteral("p"));
    args.minimum = input.value(QStringLiteral("n")).toInt();
    args.maximum = input.value(QStringLiteral("m")).toInt();
    args.depth = input.value(QStringLiteral("d")).toInt();
    args.letters = input.value(QStringLiteral("c"));
    args.word = input.value(QStringLiteral("w"));
    args.file = input.value(QStringLiteral("f"));
    args.output = input.value(QStringLiteral("o"));

    if (args.silent < SilenceLevel::NONE)
    {
        args.silent = SilenceLevel::NONE;
        qWarning("%s", qUtf8Printable(tr("[%1] Silence level must be at least %2. Using %2 as fallback.").arg(tr("Warning")).arg(SilenceLevel::NONE)));
    }

    if (args.silent > SilenceLevel::ALL)
    {
        args.silent = SilenceLevel::ALL;
        qWarning("%s", qUtf8Printable(tr("[%1] Silence level must be at most %2. Using %2 as fallback.").arg(tr("Warning")).arg(SilenceLevel::ALL)));
    }

    if (args.minimum < 1)
    {
        args.minimum = 1;
        qWarning("%s", qUtf8Printable(tr("[%1] Minimum word length must be at least 1. Using 1 as fallback.").arg(tr("Warning"))));
    }

    if (args.maximum < 1)
    {
        args.maximum = 1;
        qWarning("%s", qUtf8Printable(tr("[%1] Maximum word length must be at least 1. Using 1 as fallback.").arg(tr("Warning"))));
    }
    if (args.maximum > MAX_WORD_LENGTH)
    {
        qWarning("%s", qUtf8Printable(tr("[%1] Exceeding the recommended maximum word length will impact performance.").arg(tr("Warning"))));
    }

    if (args.minimum > args.maximum)
    {
        args.minimum = args.maximum;
        qWarning("%s", qUtf8Printable(tr("[%1] Minimum word length must be less than or equal to maximum word length. Using maximum word length as fallback.").arg(tr("Warning"))));
    }

    if (args.maximum < args.minimum)
    {
        args.maximum = args.minimum;
        qWarning("%s", qUtf8Printable(tr("[%1] Maximum word length must be more than or equal to minimum word length. Using minimum word length as fallback.").arg(tr("Warning"))));
    }

    if (args.depth < 1)
    {
        args.depth = 1;
        qWarning("%s", qUtf8Printable(tr("[%1] The amount of words must be at least 1. Using 1 as fallback.").arg(tr("Warning"))));
    }

    if (!args.word.isEmpty())
    {
        processWord(args.word);
    }
}

void Wordplay::processWord(QString &word) const
{
    const auto size = word.size();
    const auto initial = word;

    word.removeIf([](const QChar character) { return !character.isLetter(); });

    if (size != word.size() && !args.gui)
    {
        qInfo("");
        qWarning("%s", qUtf8Printable(tr(R"([%1] Characters that are not part of the alphabets have been removed from "%2".)").arg(tr("Warning"), initial)));
    }
}

auto Wordplay::readFile() -> QSet<QString>
{
    if (args.silent < SilenceLevel::INFO)
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("[%1] Words are being loaded and filtered...").arg(tr("Info"))));
    }

    QFile inputFile(QFileInfo(args.file).absoluteFilePath());
    QTextStream inputStream(stdin);
    if (args.file != QStringLiteral("-"))
    {
        if (!inputFile.open(QFile::ReadOnly | QFile::Text))
        {
            qInfo("");
            qCritical("%s", qUtf8Printable(tr(R"([%1] Unable to open file "%2")").arg(tr("Error"), args.file)));

            return {};
        }

        inputStream.setDevice(&inputFile);
    }

    struct {
        qint64 processed = 0;
        qint64 longest = 0;
        struct
        {
            qint64 length = 0;
            qint64 invalid = 0;
            qint64 character = 0;
            qint64 extract = 0;
        } skipped;
    } stats;

    QSet<QString> wordSet;
    while (!inputStream.atEnd())
    {
        QString line = inputStream.readLine();

        line.removeIf([](const QChar character) { return character.isSpace(); });
        if (line.isEmpty())
        {
            continue;
        }

        ++stats.processed;
        if (line.size() < args.minimum || line.size() > args.maximum || line.size() > initialWord.size())
        {
            ++stats.skipped.length;
            continue;
        }

        if (std::ranges::any_of(line, [](const QChar character) { return character.isDigit(); }))
        {
            ++stats.skipped.invalid;
            continue;
        }

        std::ranges::transform(line, line.begin(), [](const QChar character) { return character.toUpper(); });

        if (!args.letters.isEmpty() && !std::ranges::any_of(args.letters, [&](const QChar character) { return line.contains(character); }))
        {
            ++stats.skipped.character;
            continue;
        }

        if (!extract(initialWord, line))
        {
            ++stats.skipped.extract;
            continue;
        }

        stats.longest = std::max(line.size(), stats.longest);

        wordSet.insert(line);
    }

    if (args.silent < SilenceLevel::INFO)
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("[%1] %2 of %3 words loaded. Longest word was %n letter(s).", nullptr, static_cast<int>(stats.longest)).arg(tr("Info")).arg(wordSet.size()).arg(stats.processed)));
        if (stats.skipped.length > 0)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] %2 words containing wrong amount of characters").arg(tr("Info")).arg(stats.skipped.length)));
        }
        if (stats.skipped.invalid > 0)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] %2 words containing non-alphabetical characters").arg(tr("Info")).arg(stats.skipped.invalid)));
        }
        if (stats.skipped.character > 0)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] %2 words not containing input characters").arg(tr("Info")).arg(stats.skipped.character)));
        }
        if (stats.skipped.extract > 0)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] %2 words not found in the input word").arg(tr("Info")).arg(stats.skipped.extract)));
        }
    }

    if (wordSet.empty())
    {
        if (args.silent < SilenceLevel::INFO)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] No candidate words were found.").arg(tr("Info"))));
        }
    }

    return wordSet;
}
