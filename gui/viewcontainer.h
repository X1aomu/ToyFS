#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include <QWidget>

namespace Ui {
class ViewContainer;
}

class ViewContainer : public QWidget
{
    Q_OBJECT

public:
    explicit ViewContainer(QWidget *parent = nullptr);
    ~ViewContainer();

private:
    Ui::ViewContainer *ui;
};

#endif // VIEWCONTAINER_H
