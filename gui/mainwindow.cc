#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Toy File System Explorer");

    emit setViewMode(ViewMode::BROWSE_MODE);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setViewMode(ViewMode mode)
{

}

void MainWindow::loadDisk()
{

}

void MainWindow::formatDisk()
{

}
