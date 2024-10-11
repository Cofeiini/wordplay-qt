#include "wordplay.h"

#include "common.h"

#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QStandardPaths>
#include <QThread>

#include <ranges>

namespace {

struct InputDirectory
{
    Q_NODISCARD_CTOR
    explicit InputDirectory(const QString &path)
    {
        original = QDir::currentPath();
        setCurrent(path);
    }

    ~InputDirectory()
    {
        QDir::setCurrent(original);
    }

    Q_DISABLE_COPY(InputDirectory)

    // ReSharper disable once CppMemberFunctionMayBeStatic
    // NOLINTNEXTLINE(*-convert-member-functions-to-static)
    void setCurrent(const QString &path) const
    {
        QDir::setCurrent(path);
    }

    QString original;
};

}

auto Wordplay::extract(QString initial, const QString &word) -> std::optional<QString>
{
    for (const QChar &character : word)
    {
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
    QStringList initialList;

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

        initialList.push_back(args.word);
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

        candidateRanges[letter] = { start, end };
    }

    if (args.print && (args.silent < SilenceLevel::NUMBERS))
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("Anagrams found:")));
    }

    QMutex mutex;
    QSet<QString> result;
    qint64 anagramCount = 0;
    const std::function<void(const QString &, qint64, QStringList)> recurse = [&](const QString &remaining, const qint64 iterator, QStringList acc) {
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
                const QMutexLocker lock(&mutex);
                if (args.print)
                {
                    printAnagram(++anagramCount, out);
                    continue;
                }

                result.insert(out);
            } while (args.allowRephrased && std::ranges::next_permutation(temp).found);

            return;
        }

        const auto [start, end] = candidateRanges.value(remaining.at(0), { 0, -1 });
        for (qint64 i = std::max(iterator, start); i <= end; ++i)
        {
            const auto &[candidate, normalized] = candidateWords.at(i);

            if (!args.allowDuplicates && !acc.isEmpty() && acc.contains(candidate))
            {
                continue;
            }

            const auto extracted = extract(remaining, normalized);
            if (!extracted)
            {
                continue;
            }

            acc.append(candidate);
            recurse(extracted.value(), i + static_cast<qint64>(!args.allowDuplicates), acc);
            acc.removeLast();
        }
    };

    QList<QThread *> workers;
    const auto [start, end] = candidateRanges.value(initialWord.at(0), { 0, -1 });
    for (qint64 i = std::max(0LL, start); i <= end; ++i)
    {
        const auto &[candidate, normalized] = candidateWords.at(i);

        if (!args.allowDuplicates && !initialList.isEmpty() && initialList.contains(candidate))
        {
            continue;
        }

        const auto extracted = extract(initialWord, normalized);
        if (!extracted)
        {
            continue;
        }

        initialList.append(candidate);
        auto *worker = QThread::create(recurse, extracted.value(), i + static_cast<qint64>(!args.allowDuplicates), initialList);
        QObject::connect(worker, &QThread::finished, worker, &QThread::deleteLater);
        workers.append(worker);
        worker->start();
        initialList.removeLast();
    }

    for (auto *worker : workers)
    {
        worker->wait();
    }

    if (args.print)
    {
        quick_exit(EXIT_SUCCESS);
    }

    return result;
}

auto Wordplay::generateCandidates() -> QList<StrPair>
{
    const auto candidates = readFile();

    QMutex mutex;
    const int cores = QThread::idealThreadCount();
    const qint64 total = candidates.size();
    const qint64 portions = std::floor(static_cast<float>(total) / static_cast<float>(cores));

    QList<StrPair> words;

    QList<QThread *> workers;
    workers.reserve(cores);
    for (int i = 0; i < cores; ++i)
    {
        const qint64 start = portions * i;
        const qint64 end = portions * (i + 1);
        const auto list = QStringList(candidates.begin() + start, candidates.begin() + end);

        auto *worker = QThread::create([=, &mutex, &words]() {
            for (const auto &word : list)
            {
                QString normalized = word;
                std::ranges::sort(normalized);

                const QMutexLocker lock(&mutex);
                words.emplace_back(word, normalized);
            }
        });
        QObject::connect(worker, &QThread::finished, worker, &QThread::deleteLater);
        workers.append(worker);
        worker->start();
    }

    for (auto *worker : workers)
    {
        worker->wait();
    }

    const qint64 extra = total - (portions * cores);
    if (extra > 0)
    {
        const qint64 start = portions * cores;
        const qint64 end = start + extra;
        for (qint64 i = start; i < end; ++i)
        {
            const auto &word = candidates.at(i);
            QString normalized = word;
            std::ranges::sort(normalized);
            words.emplace_back(word, normalized);
        }
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
        const QFileInfo info(CommonFunctions::handleTilde(args.output));
        outFile.setFileName(info.fileName());
        [[maybe_unused]] const InputDirectory directoryHandle(info.absolutePath());
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
    finalResult.clear();
    if (result.isEmpty())
    {
        if (args.silent < SilenceLevel::INFO)
        {
            qInfo("%s", qUtf8Printable(tr(R"([%1] No anagrams for "%2" were found.)").arg(tr("Info"), originalWord)));
        }

        return EXIT_SUCCESS;
    }
    finalResult.reserve(result.size());
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
    initialWord = input.positionalArguments().value(0, {});
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

    if (!args.letters.isEmpty())
    {
        processWord(args.letters);
    }
}

void Wordplay::processWord(QString &word) const
{
    const auto size = word.size();
    const auto initial = word;

    word.removeIf([](const QChar character) { return !character.isPrint(); });

    if (size != word.size() && !args.gui)
    {
        qInfo("");
        qWarning("%s", qUtf8Printable(tr(R"([%1] Characters that are not part of the alphabets have been removed from "%2".)").arg(tr("Warning"), initial)));
    }
}

auto Wordplay::readFile() -> QStringList
{
    if (args.silent < SilenceLevel::INFO)
    {
        qInfo("");
        qInfo("%s", qUtf8Printable(tr("[%1] Words are being loaded and filtered...").arg(tr("Info"))));
    }

    QTextStream inputStream(stdin);
    const QFileInfo info(CommonFunctions::handleTilde(args.file));
    QFile inputFile(info.fileName()); // Remember to specify the input file here, so that it doesn't get deleted before reading is done
    if (args.file != QStringLiteral("-"))
    {
        const InputDirectory directoryHandle(info.absolutePath());
        if (!inputFile.exists())
        {
            directoryHandle.setCurrent(QStringLiteral("%1/words").arg(QCoreApplication::applicationDirPath()));
            if (!inputFile.exists())
            {
                directoryHandle.setCurrent(QStringLiteral("%1/words").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)));
            }
        }

        if (!inputFile.open(QFile::ReadOnly | QFile::Text))
        {
            qCritical("%s", qUtf8Printable(tr(R"([%1] Unable to open file "%2")").arg(tr("Error"), args.file)));

            return {};
        }

        if (args.silent < SilenceLevel::INFO)
        {
            qInfo("%s", qUtf8Printable(tr(R"([%1] Reading file "%2" from "%3")").arg(tr("Info"), info.fileName(), QDir::currentPath())));
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

    QStringList wordList;
    while (!inputStream.atEnd())
    {
        QString line = inputStream.readLine();

        line.removeIf([](const QChar character) { return character.isSpace(); });
        if (line.isEmpty())
        {
            continue;
        }

        ++stats.processed;

        const qint64 lineSize = line.size();
        if (lineSize > initialWord.size() || lineSize < args.minimum || lineSize > args.maximum)
        {
            ++stats.skipped.length;
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

        wordList.append(line);
    }

    std::ranges::sort(wordList);
    wordList.erase(std::ranges::unique(wordList).cbegin(), wordList.cend());

    if (args.silent < SilenceLevel::INFO)
    {
        qInfo("");
        if (wordList.isEmpty())
        {
            qInfo("%s", qUtf8Printable(tr("[%1] No candidate words were found.").arg(tr("Info"))));
            return wordList;
        }

        qInfo("%s", qUtf8Printable(tr("[%1] %2 of %3 words loaded. Longest word was %n letter(s).", nullptr, static_cast<int>(stats.longest)).arg(tr("Info")).arg(wordList.size()).arg(stats.processed)));
        if (stats.skipped.length > 0)
        {
            qInfo("%s", qUtf8Printable(tr("[%1] %2 words containing wrong amount of characters").arg(tr("Info")).arg(stats.skipped.length)));
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

    return wordList;
}
