#include "dirview.h"
#include "ui_dirview.h"

DirView::DirView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirView)
{
    ui->setupUi(this);
}

DirView::~DirView()
{
    delete ui;
}
