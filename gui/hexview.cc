#include "hexview.h"
#include "ui_hexview.h"

HexView::HexView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HexView)
{
    ui->setupUi(this);
}

HexView::~HexView()
{
    delete ui;
}
