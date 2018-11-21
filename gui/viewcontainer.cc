#include "viewcontainer.h"
#include "ui_viewcontainer.h"

ViewContainer::ViewContainer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewContainer)
{
    ui->setupUi(this);
}

ViewContainer::~ViewContainer()
{
    delete ui;
}
