#include "hexview.h"
#include "ui_hexview.h"

HexView::HexView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HexView),
    m_disk(nullptr)
{
    ui->setupUi(this);
}

HexView::~HexView()
{
    delete ui;
}

void HexView::setDisk(Disk *disk)
{
    m_disk = disk;
}
