#include "locationbar.h"
#include "ui_locationbar.h"

LocationBar::LocationBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LocationBar)
{
    ui->setupUi(this);
}

LocationBar::~LocationBar()
{
    delete ui;
}
