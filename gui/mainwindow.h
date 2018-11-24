#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "disk.h"
#include "filesystem.h"

#include "gui/dirview.h"
#include "gui/hexview.h"

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

public slots:
    void showMessage(const QString& msg);

private slots:
    void setViewMode(ViewMode mode);

    void openFile(const QString& filePath);
    void closeFile();
    void saveFile();

    void updateViews();

    void on_actionNew_Disk_triggered();
    void on_actionOpen_Disk_triggered();
    void on_actionExit_triggered();
    void on_actionSave_triggered();
    void on_actionClose_Disk_triggered();
    void on_actionCreate_File_System_triggered();
    void on_actionInitializie_File_System_triggered();

private:
    Ui::MainWindow *ui;
    DirView *m_dirView;
    HexView *m_hexView;
    QString m_baseWindowTitle = "Toy File System Demo";

    Disk* m_disk;
    FileSystem* m_fs;

};

#endif // MAINWINDOW_H
