#ifndef LOCATIONBAR_H
#define LOCATIONBAR_H

#include <QWidget>

namespace Ui {
class LocationBar;
}

class LocationBar : public QWidget
{
    Q_OBJECT

public:
    explicit LocationBar(QWidget *parent = nullptr);
    ~LocationBar();

private:
    Ui::LocationBar *ui;
};

#endif // LOCATIONBAR_H
