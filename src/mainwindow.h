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
    void process() const;

    QLineEdit *input = nullptr;
    QLineEdit *letterFilter = nullptr;
    QLineEdit *wordFilter = nullptr;
    QTableWidget *output = nullptr;
    QTableWidget *candidates = nullptr;
    Wordplay *wordplay = nullptr;
};

#endif
