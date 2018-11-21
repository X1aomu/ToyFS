#include "propertieswindow.h"
#include "ui_propertieswindow.h"

PropertiesWindow::PropertiesWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PropertiesWindow)
{
    ui->setupUi(this);
}

PropertiesWindow::~PropertiesWindow()
{
    delete ui;
}
