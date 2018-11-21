#ifndef HEXVIEW_H
#define HEXVIEW_H

#include <QWidget>

namespace Ui {
class HexView;
}

class HexView : public QWidget
{
    Q_OBJECT

public:
    explicit HexView(QWidget *parent = nullptr);
    ~HexView();

private:
    Ui::HexView *ui;
};

#endif // HEXVIEW_H
