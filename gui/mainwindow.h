#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

enum ViewMode
{
    BROWSE_MODE,
    HEX_MODE
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void setViewMode(ViewMode mode);
    void loadDisk();
    void formatDisk();
};

#endif // MAINWINDOW_H
