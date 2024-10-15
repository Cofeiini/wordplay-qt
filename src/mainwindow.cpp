#include "mainwindow.h"

#include "common.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <typeindex>

namespace {

struct OverrideCursor
{
    Q_NODISCARD_CTOR
    explicit OverrideCursor(const Qt::CursorShape cursor)
    {
        QApplication::setOverrideCursor(cursor);
    }

    ~OverrideCursor()
    {
        QApplication::restoreOverrideCursor();
    }

    Q_DISABLE_COPY(OverrideCursor)
};

}

MainWindow::MainWindow(Wordplay &core, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 (%2)").arg(QApplication::applicationName(), QApplication::applicationVersion()));

    wordplay = &core;
    wordplay->args.silent = SilenceLevel::ALL;
    wordplay->args.gui = true;

    setupConfig();

    output = new QTableWidget(1, 1);
    candidates = new QTableWidget(1, 1);

    auto *centralWidget = new QWidget();

    auto *inputLayout = new QHBoxLayout();

    input = new QLineEdit();
    addTranslatedWidget(input, QT_TR_NOOP("The word to anagram"));
    input->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    inputLayout->addWidget(input);

    auto *generate = new QPushButton();
    addTranslatedWidget(generate, QT_TR_NOOP("Generate"));
    inputLayout->addWidget(generate);

    connect(input, &QLineEdit::returnPressed, generate, &QPushButton::click);
    connect(generate, &QPushButton::clicked, this, &MainWindow::process);

    auto *controlsLayout = new QVBoxLayout();

    auto *minimum = new QSpinBox();
    minimum->setRange(1, MAX_WORD_LENGTH);
    minimum->setValue(static_cast<int>(config.options.minimum));
    auto *minimumLayout = new QHBoxLayout();
    minimumLayout->addWidget(minimum);
    auto *minimumGroup = new QGroupBox();
    addTranslatedWidget(minimumGroup, QT_TR_NOOP("Minimum characters per word"));
    addTranslatedTooltip(minimumGroup, QT_TR_NOOP("Useful for removing short words from the results"));
    minimumGroup->setLayout(minimumLayout);
    controlsLayout->addWidget(minimumGroup);

    auto *maximum = new QSpinBox();
    maximum->setRange(1, MAX_WORD_LENGTH);
    maximum->setValue(static_cast<int>(config.options.maximum));
    auto *maximumLayout = new QHBoxLayout();
    maximumLayout->addWidget(maximum);
    auto *maximumGroup = new QGroupBox();
    addTranslatedWidget(maximumGroup, QT_TR_NOOP("Maximum characters per word"));
    addTranslatedTooltip(maximumGroup, QT_TR_NOOP("Useful for removing long words from the results"));
    maximumGroup->setLayout(maximumLayout);
    controlsLayout->addWidget(maximumGroup);

    connect(minimum, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.minimum = value;
        config.options.minimum = value;
        maximum->setMinimum(value);
    });

    connect(maximum, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.maximum = value;
        config.options.maximum = value;
        minimum->setMaximum(value);
    });

    auto *depth = new QSpinBox();
    depth->setRange(1, MAX_ANAGRAM_WORDS);
    depth->setValue(static_cast<int>(config.options.depth));
    auto *depthWarning = new QLabel();
    addTranslatedWidget(depthWarning, QT_TR_NOOP("Big word limit will impact performance while giving poor results"));
    depthWarning->setStyleSheet(QStringLiteral("color: red; font: bold 12px;"));
    depthWarning->setVisible(false);
    auto *depthLayout = new QVBoxLayout();
    depthLayout->addWidget(depth);
    depthLayout->addWidget(depthWarning);
    auto *depthGroup = new QGroupBox();
    addTranslatedWidget(depthGroup, QT_TR_NOOP("Word limit per result"));
    addTranslatedTooltip(depthGroup, QT_TR_NOOP("Useful for removing long sentences from the results"));
    depthGroup->setLayout(depthLayout);
    controlsLayout->addWidget(depthGroup);

    connect(depth, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.depth = value;
        config.options.depth = value;
        depthWarning->setVisible(value > DEFAULT_ANAGRAM_WORDS);
    });

    letterFilter = new QLineEdit();
    addTranslatedWidget(letterFilter, QT_TR_NOOP("Optional letters for filtering the candidates"));
    letterFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    controlsLayout->addWidget(letterFilter);

    connect(letterFilter, &QLineEdit::returnPressed, generate, &QPushButton::click);

    wordFilter = new QLineEdit();
    addTranslatedWidget(wordFilter, QT_TR_NOOP("Optional word for filtering the results"));
    wordFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    controlsLayout->addWidget(wordFilter);

    connect(wordFilter, &QLineEdit::returnPressed, generate, &QPushButton::click);

    controlsLayout->addStretch();
    auto *controlsGroup = new QGroupBox();
    addTranslatedWidget(controlsGroup, QT_TR_NOOP("Controls"));
    controlsGroup->setLayout(controlsLayout);

    auto *optionsLayout = new QVBoxLayout();

    // Terminal option a
    auto *allowDuplicates = new QCheckBox();
    addTranslatedWidget(allowDuplicates, QT_TR_NOOP("Allow duplicates in anagrams"));
    addTranslatedTooltip(allowDuplicates, QT_TR_NOOP("Useful for adding unusual results, but often produces nonsense"));
    optionsLayout->addWidget(allowDuplicates);

    connect(allowDuplicates, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.allowDuplicates = checked;
        config.options.allowDuplicates = checked;
    });
    if (config.options.allowDuplicates)
    {
        allowDuplicates->toggle();
    }

    // Terminal option i
    auto *includeInput = new QCheckBox();
    addTranslatedWidget(includeInput, QT_TR_NOOP("Include input word"));
    addTranslatedTooltip(includeInput, QT_TR_NOOP("Useful for making a more complete list of results"));
    optionsLayout->addWidget(includeInput);

    connect(includeInput, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.includeInput = checked;
        config.options.includeInput = checked;
    });
    if (config.options.includeInput)
    {
        includeInput->toggle();
    }

    // Terminal option l
    auto *listCandidates = new QCheckBox();
    addTranslatedWidget(listCandidates, QT_TR_NOOP("List candidates"));
    addTranslatedTooltip(listCandidates, QT_TR_NOOP("Useful for visualizing the candidate words"));
    optionsLayout->addWidget(listCandidates);

    connect(listCandidates, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.listCandidates = checked;
        config.options.listCandidates = checked;
        candidates->setVisible(checked);
    });
    if (config.options.listCandidates)
    {
        listCandidates->toggle();
    }

    // Terminal option r
    auto *rephrase = new QCheckBox();
    addTranslatedWidget(rephrase, QT_TR_NOOP("Allow rephrased"));
    addTranslatedTooltip(rephrase, QT_TR_NOOP("Useful for getting many results, but exponentially increases resource usage"));
    optionsLayout->addWidget(rephrase);

    connect(rephrase, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.allowRephrased = checked;
        config.options.rephrase = checked;
    });
    if (config.options.rephrase)
    {
        rephrase->toggle();
    }

    // Terminal option x
    auto *noGenerate = new QCheckBox();
    addTranslatedWidget(noGenerate, QT_TR_NOOP("No anagrams"));
    addTranslatedTooltip(noGenerate, QT_TR_NOOP("Useful for inspecting candidates of very long input words"));
    optionsLayout->addWidget(noGenerate);

    connect(noGenerate, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.recursive = !checked;
        config.options.noGenerate = checked;
        output->setVisible(!checked);
    });
    if (config.options.noGenerate)
    {
        noGenerate->toggle();
    }

    // Terminal option z
    auto *sort = new QCheckBox();
    addTranslatedWidget(sort, QT_TR_NOOP("Sort results"));
    addTranslatedTooltip(sort, QT_TR_NOOP("Useful for organizing the results, but increases generation time"));
    optionsLayout->addWidget(sort);

    connect(sort, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.sort = checked;
        config.options.sort = checked;
    });
    if (config.options.sort)
    {
        sort->toggle();
    }

    optionsLayout->addStretch();

    // Terminal option f
    auto *wordListLabel = new QLabel();
    addTranslatedWidget(wordListLabel, QT_TR_NOOP("Wordlist:"));

    const auto configWordsPath = CommonFunctions::configPath(QStringLiteral("words"));
    const auto localWordsPath = CommonFunctions::applicationPath(QStringLiteral("words"));

    const auto nameFilter = QStringLiteral("*.txt");
    constexpr auto sortFlags = QDir::SortFlag::Name | QDir::SortFlag::IgnoreCase;
    const auto configFiles = QDir(configWordsPath, nameFilter, sortFlags, QDir::Filter::Files).entryList();
    const auto localFiles = QDir(localWordsPath, nameFilter, sortFlags, QDir::Filter::Files).entryList();

    wordlistFiles.clear();
    wordlistFiles.reserve(std::max(configFiles.size(), localFiles.size()));
    wordlistFiles.insert(QStringLiteral("en-US.txt"), DEFAULT_WORD_FILE);
    for (const auto &file : configFiles)
    {
        wordlistFiles.insert(file, QStringLiteral("%1/%2").arg(configWordsPath, file));
    }
    for (const auto &file : localFiles)
    {
        wordlistFiles.insert(file, QStringLiteral("%1/%2").arg(localWordsPath, file));
    }

    QStringList tempList;
    for (auto iter = wordlistFiles.constBegin(); iter != wordlistFiles.constEnd(); ++iter)
    {
        tempList.append(iter.key());
    }
    std::ranges::sort(tempList, [](const QString &left, const QString &right){ return left < right; });

    wordlist = new QComboBox();
    wordlist->addItems(tempList);

    connect(wordlist, &QComboBox::currentTextChanged, this, [=, this](const QString &text) {
        const auto file = wordlistFiles.value(text, DEFAULT_WORD_FILE);
        wordplay->args.file = file;
        config.wordlist = text;
        generate->setEnabled(QFile::exists(file));
    });
    wordlist->setCurrentIndex(static_cast<qint32>(tempList.indexOf(config.wordlist)));

    auto *wordListLayout = new QHBoxLayout();
    wordListLayout->addWidget(wordListLabel);
    wordListLayout->addWidget(wordlist);
    optionsLayout->addLayout(wordListLayout);

    // Terminal option o
    auto *savePath = new QLineEdit();
    addTranslatedWidget(savePath, QT_TR_NOOP("File to use for writing results"));
    savePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(savePath, &QLineEdit::textChanged, this, [this](const QString &text) {
        checks.hasOutput = !text.isEmpty();
        save->setEnabled(checks.canSave());
    });

    auto *saveLineNumbers = new QPushButton();
    saveLineNumbers->setCheckable(true);
    addTranslatedWidget(saveLineNumbers, QT_TR_NOOP("Line numbers"));
    addTranslatedTooltip(saveLineNumbers, QT_TR_NOOP("Save line numbers along with the results"));

    connect(saveLineNumbers, &QCheckBox::toggled, this, [this](const bool checked) {
        checks.saveLines = checked;
    });

    auto *saveBrowse = new QPushButton();
    addTranslatedWidget(saveBrowse, QT_TR_NOOP("Browse"));
    saveBrowse->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    connect(saveBrowse, &QPushButton::pressed, this, [=, this]() {
        savePath->setText(QFileDialog::getSaveFileName(this, tr("Select output file")));
    });

    save = new QPushButton();
    addTranslatedWidget(save, QT_TR_NOOP("Save"));
    save->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    save->setEnabled(false);

    connect(save, &QPushButton::pressed, this, [=, this]() {
        const QFileInfo info(CommonFunctions::handleTilde(savePath->text()));
        QFile file(info.absoluteFilePath());
        if (!file.open(QFile::Text | QFile::WriteOnly | QFile::Truncate))
        {
            QMessageBox::warning(this, tr("Output failed"), tr(R"(Output to file "%1" failed with error "%2".)").arg(info.absoluteFilePath(), file.errorString()));
            return;
        }

        quint32 lineNumbers = 1;
        const qint32 outputRows = output->rowCount();
        for (qint32 i = 0; i < outputRows; ++i)
        {
            if (output->isRowHidden(i))
            {
                continue;
            }

            if (checks.saveLines)
            {
                file.write(QStringLiteral("%1. %2\n").arg(lineNumbers++, 8, 10, QChar(' ')).arg(output->item(i, 0)->text()).toUtf8());
                continue;
            }

            file.write(QStringLiteral("%1\n").arg(output->item(i, 0)->text()).toUtf8());
        }

        QMessageBox::information(this, tr("Output saved"), tr(R"(Output to file "%1" was successful.)").arg(info.absoluteFilePath()));
    });

    auto *outputPathLayout = new QHBoxLayout();
    outputPathLayout->addWidget(savePath);
    outputPathLayout->addWidget(saveLineNumbers);
    outputPathLayout->addWidget(saveBrowse);
    outputPathLayout->addWidget(save);

    // Terminal environment variable LANGUAGE
    auto *languageLabel = new QLabel();
    addTranslatedWidget(languageLabel, QT_TR_NOOP("Language:"));

    auto *language = new QComboBox();
    translatedLanguages = QStringLiteral(I18N_TRANSLATED_LANGUAGES).split(QStringLiteral(","));
    for (const auto &translated : translatedLanguages)
    {
        language->addItem(QLocale(translated).nativeLanguageName());
    }
    for (const QString &locale : QLocale::system().uiLanguages())
    {
        const auto index = translatedLanguages.indexOf(locale);
        if (index > -1)
        {
            language->setCurrentIndex(static_cast<qint32>(index));
            break;
        }
    }

    connect(language, &QComboBox::currentIndexChanged, this, [this](const qint32 index) {
        const auto locale = translatedLanguages.value(index, config.language);

        if (wordplay->translator.load(QStringLiteral(":/i18n/wordplay_%1").arg(locale)))
        {
            config.language = locale;
            return;
        }

        if (wordplay->translator.load(QStringLiteral(":/i18n/wordplay_en-US")))
        {
            config.language = QStringLiteral("en-US");
            return;
        }

        QMessageBox::warning(this, tr("Warning"), tr("Failed to load translations for %1").arg(QLocale(locale).nativeLanguageName()), QMessageBox::Ok);
    });
    language->setCurrentIndex(static_cast<int>(translatedLanguages.indexOf(config.language)));

    auto *languageLayout = new QHBoxLayout();
    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(language);
    optionsLayout->addLayout(languageLayout);

    auto *optionsGroup = new QGroupBox();
    addTranslatedWidget(optionsGroup, QT_TR_NOOP("Options"));
    optionsGroup->setLayout(optionsLayout);

    outputSearch = new QLineEdit();
    //: The act of searching from the results
    addTranslatedWidget(outputSearch, QT_TR_NOOP("Search results"));
    outputSearch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(outputSearch, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!checks.hasOutput)
        {
            return;
        }

        const qint32 outputRows = output->rowCount();
        for (qint32 i = 0; i < outputRows; ++i)
        {
            output->showRow(i);
            if (!output->item(i, 0)->text().contains(text, Qt::CaseInsensitive))
            {
                output->hideRow(i);
            }
        }
    });

    candidateSearch = new QLineEdit();
    //: The act of searching from the candidates
    addTranslatedWidget(candidateSearch, QT_TR_NOOP("Search candidates"));
    candidateSearch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(candidateSearch, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!checks.hasCandidates)
        {
            return;
        }

        const qint32 outputRows = candidates->rowCount();
        for (qint32 i = 0; i < outputRows; ++i)
        {
            candidates->showRow(i);
            if (!candidates->item(i, 0)->text().contains(text, Qt::CaseInsensitive))
            {
                candidates->hideRow(i);
            }
        }
    });

    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(outputSearch);
    filterLayout->addWidget(candidateSearch);

    auto *outputLayout = new QHBoxLayout();

    output->horizontalHeader()->setVisible(false);
    output->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    output->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    output->setEditTriggers(QTableWidget::NoEditTriggers);
    output->setAlternatingRowColors(true);
    output->setItem(0, 0, new QTableWidgetItem(tr("No results...")));
    outputLayout->addWidget(output);

    candidates->horizontalHeader()->setVisible(false);
    candidates->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    candidates->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    candidates->setEditTriggers(QTableWidget::NoEditTriggers);
    candidates->setAlternatingRowColors(true);
    candidates->setVisible(config.options.listCandidates);
    candidates->setItem(0, 0, new QTableWidgetItem(tr("No candidates...")));
    outputLayout->addWidget(candidates);

    auto *centralHLayout = new QHBoxLayout();
    centralHLayout->addWidget(optionsGroup);
    centralHLayout->addWidget(controlsGroup);

    auto *centralVLayout = new QVBoxLayout();
    centralVLayout->addLayout(centralHLayout);
    centralVLayout->addLayout(inputLayout);
    centralVLayout->addLayout(filterLayout);
    centralVLayout->addLayout(outputLayout);
    centralVLayout->setStretchFactor(outputLayout, 1);
    centralVLayout->addLayout(outputPathLayout);

    centralWidget->setLayout(centralVLayout);

    setCentralWidget(centralWidget);
    setMinimumSize(768, 576);
    resize(768, 768);
}

void MainWindow::process()
{
    auto initial = input->text().trimmed();
    wordplay->processWord(initial);

    auto letters = letterFilter->text().trimmed();
    wordplay->processWord(letters);

    auto word = wordFilter->text();
    wordplay->processWord(word);

    [[maybe_unused]] const OverrideCursor overrideCursor(Qt::WaitCursor);

    wordplay->args.letters = letters;
    wordplay->args.word = word;
    wordplay->initialWord = initial;
    wordplay->process();

    candidates->clearContents();
    const auto candidateCount = static_cast<qint32>(wordplay->candidateWords.size());
    candidates->setRowCount(std::max(1, candidateCount));
    candidates->setItem(0, 0, new QTableWidgetItem(tr("No candidates...")));
    candidates->showRow(0);

    std::ranges::sort(wordplay->candidateWords, [](const StrPair &left, const StrPair &right) { return (left.first.size() != right.first.size()) ? (left.first.size() < right.first.size()) : (left.first < right.first); });

    const auto &searchedCandidate = candidateSearch->text();
    for (qint32 i = 0; i < candidateCount; ++i)
    {
        const auto &line = wordplay->candidateWords.at(i).first;
        candidates->setItem(i, 0, new QTableWidgetItem(line));
        candidates->showRow(i);
        if (!line.contains(searchedCandidate, Qt::CaseInsensitive))
        {
            candidates->hideRow(i);
        }
    }

    checks.hasCandidates = candidateCount > 0;

    output->clearContents();
    const auto resultCount = static_cast<qint32>(wordplay->finalResult.size());
    output->setRowCount(std::max(1, resultCount));
    output->setItem(0, 0, new QTableWidgetItem(tr("No results...")));
    output->showRow(0);

    const auto &searchedResult = outputSearch->text();
    for (qint32 i = 0; i < resultCount; ++i)
    {
        const auto &line = wordplay->finalResult.at(i);
        output->setItem(i, 0, new QTableWidgetItem(line));
        output->showRow(i);
        if (!line.contains(searchedResult, Qt::CaseInsensitive))
        {
            output->hideRow(i);
        }
    }

    checks.hasResults = resultCount > 0;
    save->setEnabled(checks.canSave());
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for (auto iter = translatableWidgets.constBegin(); iter != translatableWidgets.constEnd(); ++iter)
        {
            assignTranslation(iter.key(), iter.value());
        }

        for (auto iter = translatableTooltips.constBegin(); iter != translatableTooltips.constEnd(); ++iter)
        {
            assignTooltip(iter.key(), iter.value());
        }

        if (!checks.hasResults)
        {
            output->item(0, 0)->setText(tr("No results..."));
        }

        if (!checks.hasCandidates)
        {
            candidates->item(0, 0)->setText(tr("No candidates..."));
        }
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QJsonObject options {
        { "allow-multiple", config.options.allowDuplicates },
        { "depth", config.options.depth },
        { "include", config.options.includeInput },
        { "list", config.options.listCandidates },
        { "maximum", config.options.maximum },
        { "minimum", config.options.minimum },
        { "rephrase", config.options.rephrase },
        { "no-generate", config.options.noGenerate },
        { "sort", config.options.sort },
    };

    const QJsonObject json {
        { "language", config.language },
        { "wordlist", config.wordlist },
        { "options", options },
    };

    const QString error = ConfigFunctions::writeConfig(json);
    if (!error.isEmpty())
    {
        QMessageBox::warning(this, tr("Saving config failed"), tr(R"(Writing config failed with error "%1".)").arg(error));
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::setupConfig()
{
    const QJsonObject json = ConfigFunctions::readConfig();
    config.language = json.value(QStringLiteral("language")).toString(QStringLiteral("en-US"));
    config.wordlist = json.value(QStringLiteral("wordlist")).toString(QStringLiteral("en-US.txt"));

    const QJsonObject options = json.value(QStringLiteral("options")).toObject();
    config.options.allowDuplicates = options.value(QStringLiteral("allow-multiple")).toBool(false);
    config.options.depth = options.value(QStringLiteral("depth")).toInteger(DEFAULT_ANAGRAM_WORDS);
    config.options.includeInput = options.value(QStringLiteral("include")).toBool(true);
    config.options.listCandidates = options.value(QStringLiteral("list")).toBool(false);
    config.options.maximum = options.value(QStringLiteral("maximum")).toInteger(MAX_WORD_LENGTH);
    config.options.minimum = options.value(QStringLiteral("minimum")).toInteger(1);
    config.options.rephrase = options.value(QStringLiteral("rephrase")).toBool(false);
    config.options.noGenerate = options.value(QStringLiteral("no-generate")).toBool(false);
    config.options.sort = options.value(QStringLiteral("sort")).toBool(true);
}

template<typename T>
void MainWindow::assignTranslation(T *widget, const char *text)
{
    static const QHash<std::type_index, std::function<void(QWidget *, const char *)>> functions = {
        {
            std::type_index(typeid(QLineEdit)), [](QWidget *extWidget, const char *extText) {
                qobject_cast<QLineEdit *>(extWidget)->setPlaceholderText(tr(extText));
            }
        },
        {
            std::type_index(typeid(QPushButton)), [](QWidget *extWidget, const char *extText) {
                qobject_cast<QPushButton *>(extWidget)->setText(tr(extText));
            }
        },
        {
            std::type_index(typeid(QGroupBox)), [](QWidget *extWidget, const char *extText) {
                qobject_cast<QGroupBox *>(extWidget)->setTitle(tr(extText));
            }
        },
        {
            std::type_index(typeid(QLabel)), [](QWidget *extWidget, const char *extText) {
                qobject_cast<QLabel *>(extWidget)->setText(tr(extText));
            }
        },
        {
            std::type_index(typeid(QCheckBox)), [](QWidget *extWidget, const char *extText) {
                qobject_cast<QCheckBox *>(extWidget)->setText(tr(extText));
            }
        },
    };

    functions.value(std::type_index(typeid(*widget)), [](QWidget *extWidget, const char *extText) {
        qFatal(qPrintable(QStringLiteral("Missing assignment function for a widget type (%1) with text: %2").arg(typeid(*extWidget).name(), extText)));
    })(widget, text);
}

template<typename T>
void MainWindow::assignTooltip(T *widget, const char *text)
{
    widget->setToolTip(tr(text));
}

template<typename T>
void MainWindow::addTranslatedWidget(T *widget, const char *text)
{
    translatableWidgets.insert(widget, text);
    assignTranslation(widget, text);
}

template<typename T>
void MainWindow::addTranslatedTooltip(T *widget, const char *text)
{
    translatableTooltips.insert(widget, text);
    assignTooltip(widget, text);
}
