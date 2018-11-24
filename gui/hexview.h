#ifndef HEXVIEW_H
#define HEXVIEW_H

#include <QWidget>

#include "disk.h"

namespace Ui {
class HexView;
}

class HexView : public QWidget
{
    Q_OBJECT

public:
    explicit HexView(QWidget *parent = nullptr);
    ~HexView();

    void setDisk(Disk *disk);

private:
    Ui::HexView *ui;

    Disk* m_disk;
};

#endif // HEXVIEW_H
