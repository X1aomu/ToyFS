#ifndef DIRVIEW_H
#define DIRVIEW_H

#include <QWidget>

namespace Ui {
class DirView;
}

class DirView : public QWidget
{
    Q_OBJECT

public:
    explicit DirView(QWidget *parent = nullptr);
    ~DirView();

private:
    Ui::DirView *ui;
};

#endif // DIRVIEW_H
