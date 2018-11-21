#include "texteditingwindow.h"
#include "ui_texteditingwindow.h"

TextEditingWindow::TextEditingWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TextEditingWindow)
{
    ui->setupUi(this);
}

TextEditingWindow::~TextEditingWindow()
{
    delete ui;
}
