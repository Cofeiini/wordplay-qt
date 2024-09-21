#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "wordplay.h"

#include <QMainWindow>
#include <QLineEdit>
#include <QTableWidget>

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Wordplay &core, QWidget *parent = nullptr);

private:
    void changeEvent(QEvent *event) override;

    void process() const;
    template<typename T>
    static void assignTranslation(T *widget, const char *text);
    template<typename T>
    static void assignTooltip(T *widget, const char *text);
    template<typename T>
    void addTranslatedWidget(T *widget, const char *text);
    template<typename T>
    void addTranslatedTooltip(T *widget, const char *text);

    QStringList translatedLanguages;
    QHash<QWidget *, const char *> translatableWidgets;
    QHash<QWidget *, const char *> translatableTooltips;

    QLineEdit *input = nullptr;
    QLineEdit *letterFilter = nullptr;
    QLineEdit *wordFilter = nullptr;
    QTableWidget *output = nullptr;
    QTableWidget *candidates = nullptr;
    Wordplay *wordplay = nullptr;
};

#endif
