#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "wordplay.h"

#include <QComboBox>
#include <QJsonObject>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Wordplay &core, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void setupConfig();
    void process();

    template<typename T>
    static void assignTranslation(T *widget, const char *text);
    template<typename T>
    static void assignTooltip(T *widget, const char *text);
    template<typename T>
    void addTranslatedWidget(T *widget, const char *text);
    template<typename T>
    void addTranslatedTooltip(T *widget, const char *text);

    struct
    {
        bool hasOutput = false;
        bool hasResults = false;
        bool hasCandidates = false;

        [[nodiscard]] auto canSave() const -> bool
        {
            return hasResults && hasOutput;
        }
    } checks;

    struct
    {
        QString language;
        QString wordlist;
        struct
        {
            qint64 depth = 0;
            qint64 minimum = 0;
            qint64 maximum = 0;
            bool allowDuplicates = false;
            bool includeInput = true;
            bool listCandidates = false;
            bool rephrase = false;
            bool noGenerate = false;
            bool sort = true;
        } options;
    } config;

    QStringList translatedLanguages;
    QHash<QString, QString> wordlistFiles;
    QHash<QWidget *, const char *> translatableWidgets;
    QHash<QWidget *, const char *> translatableTooltips;

    QLineEdit *input = nullptr;
    QLineEdit *letterFilter = nullptr;
    QLineEdit *wordFilter = nullptr;
    QComboBox *wordlist = nullptr;
    QPushButton *outputSave = nullptr;
    QTableWidget *output = nullptr;
    QTableWidget *candidates = nullptr;
    std::shared_ptr<Wordplay> wordplay = nullptr;
};

#endif
