#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <QMainWindow>

namespace Ui {
class PropertiesWindow;
}

class PropertiesWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PropertiesWindow(QWidget *parent = nullptr);
    ~PropertiesWindow();

private:
    Ui::PropertiesWindow *ui;
};

#endif // PROPERTIESWINDOW_H
