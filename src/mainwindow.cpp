#include "mainwindow.h"

#include <QCheckBox>
#include <QCursor>
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
#include <typeinfo>

MainWindow::MainWindow(Wordplay &core, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 (%2)").arg(QApplication::applicationName(), QApplication::applicationVersion()));

    wordplay = &core;
    wordplay->args.silent = SilenceLevel::ALL;
    wordplay->args.gui = true;

    auto *centralWidget = new QWidget(this);

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
    minimum->setValue(1);
    auto *minimumLayout = new QHBoxLayout();
    minimumLayout->addWidget(minimum);
    auto *minimumGroup = new QGroupBox();
    addTranslatedWidget(minimumGroup, QT_TR_NOOP("Minimum characters per word"));
    addTranslatedTooltip(minimumGroup, QT_TR_NOOP("Useful for removing short words from the results"));
    minimumGroup->setLayout(minimumLayout);
    controlsLayout->addWidget(minimumGroup);

    auto *maximum = new QSpinBox();
    maximum->setRange(1, MAX_WORD_LENGTH);
    maximum->setValue(MAX_WORD_LENGTH);
    auto *maximumLayout = new QHBoxLayout();
    maximumLayout->addWidget(maximum);
    auto *maximumGroup = new QGroupBox();
    addTranslatedWidget(maximumGroup, QT_TR_NOOP("Maximum characters per word"));
    addTranslatedTooltip(maximumGroup, QT_TR_NOOP("Useful for removing long words from the results"));
    maximumGroup->setLayout(maximumLayout);
    controlsLayout->addWidget(maximumGroup);

    connect(minimum, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.minimum = value;
        maximum->setMinimum(value);
    });

    connect(maximum, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.maximum = value;
        minimum->setMaximum(value);
    });

    auto *depth = new QSpinBox();
    depth->setRange(1, MAX_ANAGRAM_WORDS);
    depth->setValue(DEFAULT_ANAGRAM_WORDS);
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
        depthWarning->setVisible(value > 4);
    });

    letterFilter = new QLineEdit();
    addTranslatedWidget(letterFilter, QT_TR_NOOP("Optional letters for filtering the results"));
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

    connect(allowDuplicates, &QCheckBox::toggled, this, [this](const bool checked) { wordplay->args.allowDuplicates = checked; });

    // Terminal option i
    auto *includeInput = new QCheckBox();
    addTranslatedWidget(includeInput, QT_TR_NOOP("Include input word"));
    addTranslatedTooltip(includeInput, QT_TR_NOOP("Useful for making a more complete list of results"));
    optionsLayout->addWidget(includeInput);

    connect(includeInput, &QCheckBox::toggled, this, [this](const bool checked) { wordplay->args.includeInput = checked; });
    includeInput->toggle();

    // Terminal option l
    auto *listCandidates = new QCheckBox();
    addTranslatedWidget(listCandidates, QT_TR_NOOP("List candidates"));
    addTranslatedTooltip(listCandidates, QT_TR_NOOP("Useful for visualizing the candidate words"));
    optionsLayout->addWidget(listCandidates);

    connect(listCandidates, &QCheckBox::toggled, this, [this](const bool checked) {
        wordplay->args.listCandidates = checked;
        candidates->setVisible(checked);
    });

    // Terminal option r
    auto *rephrase = new QCheckBox();
    addTranslatedWidget(rephrase, QT_TR_NOOP("Allow rephrased"));
    addTranslatedTooltip(rephrase, QT_TR_NOOP("Useful for getting many results, but exponentially increases resource usage"));
    optionsLayout->addWidget(rephrase);

    connect(rephrase, &QCheckBox::toggled, this, [this](const bool checked) { wordplay->args.allowRephrased = checked; });

    // Terminal option x
    auto *noGenerate = new QCheckBox();
    addTranslatedWidget(noGenerate, QT_TR_NOOP("No anagrams"));
    addTranslatedTooltip(noGenerate, QT_TR_NOOP("Useful for inspecting candidates of very long input words"));
    optionsLayout->addWidget(noGenerate);

    connect(noGenerate, &QCheckBox::toggled, this, [this](const bool checked) { wordplay->args.recursive = !checked; });

    // Terminal option z
    auto *sort = new QCheckBox();
    addTranslatedWidget(sort, QT_TR_NOOP("Sort results"));
    addTranslatedTooltip(sort, QT_TR_NOOP("Useful for organizing the results, but increases generation time"));
    optionsLayout->addWidget(sort);

    connect(sort, &QCheckBox::toggled, this, [this](const bool checked) { wordplay->args.sort = checked; });
    sort->toggle();

    optionsLayout->addStretch();

    // Terminal option f
    auto *wordListLabel = new QLabel();
    addTranslatedWidget(wordListLabel, QT_TR_NOOP("Word list:"));

    wordList = new QComboBox();
    const auto wordFiles = QStringList() << QDir(QStringLiteral(":/words")).entryList() << QStringLiteral("Custom");
    wordList->addItems(wordFiles);
    const qint32 customIndex = static_cast<qint32>(wordFiles.size()) - 1;
    wordList->setItemText(customIndex, tr("Custom")); // Remember to do this after setting up the items
    wordList->setCurrentIndex(static_cast<qint32>(wordFiles.indexOf("en-US.txt")));
    for (const auto &file : wordFiles)
    {
        wordListFiles.append(QStringLiteral(":/words/%1").arg(file));
    }

    wordListBrowse = new QPushButton();
    addTranslatedWidget(wordListBrowse, QT_TR_NOOP("Browse..."));
    wordListBrowse->setVisible(false);
    connect(wordListBrowse, &QPushButton::pressed, this, [=, this]() {
        const auto caption = tr("Open word list");
        const auto home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
        const auto filter = tr("Text Files (*.txt)");
        const auto file = QFileDialog::getOpenFileName(this, caption, home, filter);
        if (file.isEmpty())
        {
            return;
        }

        wordplay->args.file = file;
        wordListFiles[customIndex] = file;
        wordListBrowse->setText(QFileInfo(file).fileName());
        generate->setEnabled(QFile::exists(file));
    });

    connect(wordList, &QComboBox::currentIndexChanged, this, [=, this](const qint32 index) {
        wordListBrowse->setVisible(index == customIndex);
        const auto file = wordListFiles.value(index, DEFAULT_WORD_FILE);
        wordplay->args.file = file;
        generate->setEnabled(QFile::exists(file));
    });

    auto *wordListLayout = new QVBoxLayout();
    auto *wordControlLayout = new QHBoxLayout();
    wordControlLayout->addWidget(wordListLabel);
    wordControlLayout->addWidget(wordList);
    wordListLayout->addLayout(wordControlLayout);
    wordListLayout->addWidget(wordListBrowse);
    optionsLayout->addLayout(wordListLayout);

    // Translated languages
    auto *languageLabel = new QLabel();
    addTranslatedWidget(languageLabel, QT_TR_NOOP("Language:"));

    auto *language = new QComboBox();
    const auto languageList = QStringLiteral(I18N_TRANSLATED_LANGUAGES).split(QStringLiteral(","));
    translatedLanguages.reserve(languageList.size());
    for (const auto &translated : languageList)
    {
        translatedLanguages.append(translated);
        language->addItem(QLocale(translated).nativeLanguageName());
    }
    for (const QString &locale : QLocale::system().uiLanguages())
    {
        const auto index = languageList.indexOf(locale);
        if (index > -1)
        {
            language->setCurrentIndex(static_cast<qint32>(index));
            break;
        }
    }

    connect(language, &QComboBox::currentIndexChanged, this, [this](const qint32 index) {
        const auto locale = translatedLanguages.value(index, QStringLiteral("en-US"));

        if (wordplay->translator.load(QStringLiteral(":/i18n/wordplay_%1").arg(locale)))
        {
            return;
        }

        if (wordplay->translator.load(QStringLiteral(":/i18n/wordplay_en-US")))
        {
            return;
        }

        QMessageBox::warning(this, tr("Warning"), tr("Failed to load translations for %1").arg(QLocale(locale).nativeLanguageName()), QMessageBox::Ok);
    });

    auto *languageLayout = new QHBoxLayout(); // NOLINT: False positive about a memory leak
    languageLayout->addWidget(languageLabel);
    languageLayout->addWidget(language);
    optionsLayout->addLayout(languageLayout);

    auto *optionsGroup = new QGroupBox();
    addTranslatedWidget(optionsGroup, QT_TR_NOOP("Options"));
    optionsGroup->setLayout(optionsLayout);

    auto *outputLayout = new QHBoxLayout(); // NOLINT: False positive about a memory leak

    output = new QTableWidget(0, 1);
    output->horizontalHeader()->setVisible(false);
    output->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    output->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    output->setEditTriggers(QTableWidget::NoEditTriggers);
    output->setAlternatingRowColors(true);
    outputLayout->addWidget(output);

    candidates = new QTableWidget(0, 1);
    candidates->horizontalHeader()->setVisible(false);
    candidates->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    candidates->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    candidates->setEditTriggers(QTableWidget::NoEditTriggers);
    candidates->setAlternatingRowColors(true);
    candidates->setVisible(false);
    outputLayout->addWidget(candidates);

    auto *centralHLayout = new QHBoxLayout(); // NOLINT: False positive about a memory leak
    centralHLayout->addWidget(optionsGroup);
    centralHLayout->addWidget(controlsGroup);

    auto *centralVLayout = new QVBoxLayout(); // NOLINT: False positive about a memory leak
    centralVLayout->addLayout(centralHLayout);
    centralVLayout->addLayout(inputLayout);
    centralVLayout->addLayout(outputLayout);
    centralVLayout->setStretchFactor(outputLayout, 1);

    centralWidget->setLayout(centralVLayout);

    setCentralWidget(centralWidget);
    setMinimumSize(768, 576);
    resize(768, 576);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for (auto iter = translatableWidgets.begin(); iter != translatableWidgets.end(); ++iter)
        {
            assignTranslation(iter.key(), iter.value());
        }

        wordList->setItemText(wordList->count() - 1, tr("Custom"));

        const auto file = wordListFiles.value(wordListFiles.size() - 1);
        if (QFile::exists(file))
        {
            wordListBrowse->setText(QFileInfo(file).fileName());
        }

        for (auto iter = translatableTooltips.begin(); iter != translatableTooltips.end(); ++iter)
        {
            assignTooltip(iter.key(), iter.value());
        }
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::process() const
{
    auto word = input->text().trimmed();
    wordplay->processWord(word);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    wordplay->args.letters = letterFilter->text();
    wordplay->args.word = wordFilter->text();
    wordplay->initialWord = word;
    wordplay->process();

    candidates->clearContents();
    if (wordplay->args.listCandidates)
    {
        const auto rowCount = static_cast<qint32>(wordplay->candidateWords.size());
        candidates->setRowCount(rowCount);
        std::ranges::sort(wordplay->candidateWords, [](const StrPair &left, const StrPair &right) { return (left.first.size() != right.first.size()) ? (left.first.size() < right.first.size()) : (left.first < right.first); });

        for (qint32 i = 0; i < rowCount; ++i)
        {
            candidates->setItem(i, 0, new QTableWidgetItem(wordplay->candidateWords.at(i).first));
        }

        if (rowCount == 0)
        {
            candidates->setRowCount(1);
            candidates->setItem(0, 0, new QTableWidgetItem(tr("No candidates...")));
        }
    }

    const auto rowCount = static_cast<qint32>(wordplay->finalResult.size());
    output->clearContents();
    output->setRowCount(rowCount);

    for (qint32 i = 0; i < rowCount; ++i)
    {
        output->setItem(i, 0, new QTableWidgetItem(wordplay->finalResult.at(i)));
    }

    if (rowCount == 0)
    {
        output->setRowCount(1);
        output->setItem(0, 0, new QTableWidgetItem(tr("No results...")));
    }

    QApplication::restoreOverrideCursor();
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
