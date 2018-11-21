#ifndef TEXTEDITINGWINDOW_H
#define TEXTEDITINGWINDOW_H

#include <QMainWindow>

namespace Ui {
class TextEditingWindow;
}

class TextEditingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextEditingWindow(QWidget *parent = nullptr);
    ~TextEditingWindow();

private:
    Ui::TextEditingWindow *ui;
};

#endif // TEXTEDITINGWINDOW_H
