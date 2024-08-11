#include "mainwindow.h"

#include <QCheckBox>
#include <QCursor>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(Wordplay &core, QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 (%2)").arg(QApplication::applicationName(), QApplication::applicationVersion()));

    wordplay = &core;
    wordplay->args.silent = SilenceLevel::ALL;
    wordplay->args.gui = true;

    auto *centralWidget = new QWidget(this);

    auto *inputLayout = new QHBoxLayout();

    input = new QLineEdit();
    input->setPlaceholderText(tr("The word to anagram"));
    input->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    inputLayout->addWidget(input);

    auto *button = new QPushButton();
    button->setText(tr("Generate"));
    inputLayout->addWidget(button);

    connect(input, &QLineEdit::returnPressed, button, &QPushButton::click);
    connect(button, &QPushButton::clicked, this, &MainWindow::process);

    auto *controlsLayout = new QVBoxLayout();

    auto *minimum = new QSpinBox();
    minimum->setRange(1, MAX_WORD_LENGTH);
    minimum->setValue(1);
    auto *minimumLayout = new QHBoxLayout();
    minimumLayout->addWidget(minimum);
    auto *minimumGroup = new QGroupBox(tr("Minimum characters per word"));
    minimumGroup->setToolTip(tr("Useful for removing short words from the results"));
    minimumGroup->setLayout(minimumLayout);
    controlsLayout->addWidget(minimumGroup);

    auto *maximum = new QSpinBox();
    maximum->setRange(1, MAX_WORD_LENGTH);
    maximum->setValue(MAX_WORD_LENGTH);
    auto *maximumLayout = new QHBoxLayout();
    maximumLayout->addWidget(maximum);
    auto *maximumGroup = new QGroupBox(tr("Maximum characters per word"));
    maximumGroup->setToolTip(tr("Useful for removing long words from the results"));
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
    auto *depthWarning = new QLabel(tr("Big word limit will impact performance while giving poor results"));
    depthWarning->setStyleSheet("color: red; font: bold 12px;");
    depthWarning->setVisible(false);
    auto *depthLayout = new QVBoxLayout();
    depthLayout->addWidget(depth);
    depthLayout->addWidget(depthWarning);
    auto *depthGroup = new QGroupBox(tr("Word limit per result"));
    depthGroup->setToolTip(tr("Useful for removing long sentences from the results"));
    depthGroup->setLayout(depthLayout);
    controlsLayout->addWidget(depthGroup);

    connect(depth, &QSpinBox::valueChanged, this, [=, this](const qint32 value) {
        wordplay->args.depth = value;
        depthWarning->setVisible(value > 4);
    });

    letterFilter = new QLineEdit();
    letterFilter->setPlaceholderText(tr("Optional letters for filtering the results"));
    letterFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    controlsLayout->addWidget(letterFilter);

    connect(letterFilter, &QLineEdit::returnPressed, button, &QPushButton::click);

    wordFilter = new QLineEdit();
    wordFilter->setPlaceholderText(tr("Optional word for filtering the results"));
    wordFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    controlsLayout->addWidget(wordFilter);

    connect(wordFilter, &QLineEdit::returnPressed, button, &QPushButton::click);

    controlsLayout->addStretch();
    auto *controlsGroup = new QGroupBox(tr("Controls"));
    controlsGroup->setLayout(controlsLayout);

    auto *optionsLayout = new QVBoxLayout();

    // Terminal option a
    auto *allowDuplicates = new QCheckBox(tr("Allow duplicates in anagrams"));
    allowDuplicates->setToolTip(tr("Useful for adding unusual results, but often produces nonsense"));
    optionsLayout->addWidget(allowDuplicates);

    connect(allowDuplicates, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.allowDuplicates = checked; });

    // Terminal option i
    auto *includeInput = new QCheckBox(tr("Include input word"));
    includeInput->setToolTip(tr("Useful for making a more complete list of results"));
    optionsLayout->addWidget(includeInput);

    connect(includeInput, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.includeInput = checked; });
    includeInput->toggle();

    // Terminal option l
    auto *listCandidates = new QCheckBox(tr("List candidates"));
    listCandidates->setToolTip(tr("Useful for visualizing the candidate words"));
    optionsLayout->addWidget(listCandidates);

    connect(listCandidates, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.listCandidates = checked; });

    // Terminal option r
    auto *rephrase = new QCheckBox(tr("Allow rephrased"));
    rephrase->setToolTip(tr("Useful for getting many results, but exponentially increases resource usage"));
    optionsLayout->addWidget(rephrase);

    connect(rephrase, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.allowRephrased = checked; });

    // Terminal option x
    auto *noGenerate = new QCheckBox(tr("No anagrams"));
    noGenerate->setToolTip(tr("Useful for inspecting candidates of very long input words"));
    optionsLayout->addWidget(noGenerate);

    connect(noGenerate, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.recursive = !checked; });

    // Terminal option z
    auto *sort = new QCheckBox(tr("Sort results"));
    sort->setToolTip(tr("Useful for organizing the results, but increases generation time"));
    optionsLayout->addWidget(sort);

    connect(sort, &QCheckBox::toggled, this, [&](const bool checked) { wordplay->args.sort = checked; });
    sort->toggle();

    optionsLayout->addStretch();
    auto *optionsGroup = new QGroupBox(tr("Options"));
    optionsGroup->setLayout(optionsLayout);

    auto *outputLayout = new QHBoxLayout();

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

    auto *centralHLayout = new QHBoxLayout();
    centralHLayout->addWidget(optionsGroup);
    centralHLayout->addWidget(controlsGroup);

    auto *centralVLayout = new QVBoxLayout();
    centralVLayout->addLayout(centralHLayout);
    centralVLayout->addLayout(inputLayout);
    centralVLayout->addLayout(outputLayout);
    centralVLayout->setStretchFactor(outputLayout, 1);

    centralWidget->setLayout(centralVLayout);

    setCentralWidget(centralWidget);
    setMinimumSize(768, 576);
    resize(768, 576);
}

void MainWindow::process()
{
    auto word = input->text().trimmed();
    wordplay->processWord(word);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    wordplay->args.letters = letterFilter->text();
    wordplay->args.word = wordFilter->text();
    wordplay->initialWord = word;
    wordplay->process();

    candidates->clearContents();
    candidates->setVisible(wordplay->args.listCandidates);
    if (wordplay->args.listCandidates)
    {
        const auto rowCount = static_cast<qint32>(wordplay->candidateWords.size());
        candidates->setRowCount(rowCount);
        std::sort(wordplay->candidateWords.begin(), wordplay->candidateWords.end(), [](const StrPair &a, const StrPair &b) { return (a.first.size() != b.first.size()) ? (a.first.size() < b.first.size()) : (a.first < b.first); });

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
