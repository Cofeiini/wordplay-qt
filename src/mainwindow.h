#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "wordplay.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QTableWidget>

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Wordplay &core, QWidget *parent = nullptr);

private:
    void changeEvent(QEvent *event) override;

    void process();
    template<typename T>
    static void assignTranslation(T *widget, const char *text);
    template<typename T>
    static void assignTooltip(T *widget, const char *text);
    template<typename T>
    void addTranslatedWidget(T *widget, const char *text);
    template<typename T>
    void addTranslatedTooltip(T *widget, const char *text);

    union
    {
        struct
        {
            quint8 result : 1;
            quint8 output : 1;
            quint8 : 6;
        };
        quint8 value = 0; // Remember to initialize this specific member to set up the union
    } canSave;

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
    Wordplay *wordplay = nullptr;
};

#endif
