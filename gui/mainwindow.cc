#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QtDebug>

#include "disk.h"
#include "filesystem.h"

#include "gui/dirview.h"
#include "gui/hexview.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_dirView(new DirView),
    m_hexView(new HexView),
    m_disk(nullptr),
    m_fs(nullptr)
{
    ui->setupUi(this);

    setWindowTitle(m_baseWindowTitle);

    connect(m_dirView, &DirView::message, this, &MainWindow::showMessage);

    emit setViewMode(ViewMode::BROWSE_MODE);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_disk;
    delete m_fs;
}

void MainWindow::showMessage(const std::string& msg)
{
    ui->statusBar->showMessage(QString::fromStdString(msg), 5000);
}

void MainWindow::setViewMode(ViewMode mode)
{
    switch (mode) {
    case BROWSE_MODE:
        setCentralWidget(m_dirView);
        break;
    case HEX_MODE:
        setCentralWidget(m_hexView);
        break;
    }
}

void MainWindow::openFile(const QString& filePath)
{
    Disk* newDisk = new Disk(filePath.toStdString());
    qDebug() << filePath;
    qDebug() << newDisk->isValid();
    if (newDisk->isValid())
    {
        closeFile();
        m_disk = newDisk;
        m_fs = new FileSystem(*m_disk);
        updateViews();
        setWindowTitle(filePath + " - " + m_baseWindowTitle);
    }
    m_dirView->reset();
    m_dirView->cd("/");
}

void MainWindow::closeFile()
{
    delete m_fs;
    delete m_disk;
    m_disk = nullptr;
    m_fs = nullptr;
    updateViews();
    setWindowTitle(m_baseWindowTitle);
    m_dirView->reset();
}

void MainWindow::saveFile()
{
    if (m_disk == nullptr) return;
    m_disk->sync();
}

void MainWindow::updateViews()
{
    m_dirView->setFileSystem(m_fs);
    m_hexView->setDisk(m_disk);
}

void MainWindow::on_actionNew_Disk_triggered()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Create New File", QDir::currentPath());
    Disk::CreateDisk(filePath.toStdString());
}

void MainWindow::on_actionOpen_Disk_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath());
    openFile(filePath);
}

void MainWindow::on_actionExit_triggered()
{
    saveFile();
    QApplication::quit();
}

void MainWindow::on_actionSave_triggered()
{
    saveFile();
}

void MainWindow::on_actionClose_Disk_triggered()
{
    closeFile();
}

void MainWindow::on_actionCreate_File_System_triggered()
{
    if (m_fs == nullptr)
        return;
    m_fs->initFileSystem();
    m_dirView->cd(m_fs->rootEntry()->fullpath());
}

void MainWindow::on_actionInitializie_File_System_triggered()
{
    if (m_fs == nullptr)
        return;
    using std::string;
    string d1 = "/d1";
    string d2 = "/d2";
    string d3 = "/d3";
    string d4 = "/d4";
    string d5 = "/d1/d5";
    string d6 = "/d1/d5/d6";
    string d7 = "/d1/d5/d6/d7";
    string f1 = "/f1";
    string f2 = "/f2";
    string f3 = "/f3";
    string f4 = "/f4";
    string f5 = "/d1/f5";
    string f6 = "/d1/f6";
    string f7 = "/d1/f777";
    string f8 = "/d1/d5/f8";
    string f9 = "/d1/d5/d6/f9";
    string f10 = "/d1/d5/d6/d7/f10";

    // create dirs and files
    m_fs->createDir(d1);
    m_fs->createDir(d2);
    m_fs->createDir(d3);
    m_fs->createDir(d4);
    m_fs->createDir(d5);
    m_fs->createDir(d6);
    m_fs->createDir(d7);
    m_fs->createFile(f1, FileSystem::File | FileSystem::System);
    m_fs->createFile(f2, FileSystem::File);
    m_fs->createFile(f3, FileSystem::File);
    m_fs->createFile(f4, FileSystem::File);
    m_fs->createFile(f5, FileSystem::File);
    m_fs->createFile(f6, FileSystem::File);
    m_fs->createFile(f7, FileSystem::File);
    m_fs->createFile(f8, FileSystem::File);
    m_fs->createFile(f9, FileSystem::File);
    m_fs->createFile(f10, FileSystem::File);
    m_fs->setFileAttributes(f6, FileSystem::File | FileSystem::ReadOnly);

    qDebug() << "opened files " << m_fs->getOpenedFiles().size();
    m_dirView->updateOpenedFileList();
    m_dirView->updateDirectoryView();
}
