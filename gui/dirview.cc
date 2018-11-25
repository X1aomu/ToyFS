#include "dirview.h"
#include "ui_dirview.h"

#include <QtDebug>
#include <QStringListModel>
#include <QTimer>
#include <QInputDialog>

#include "gui/filepropertiesdialog.h"
#include "gui/readandwritedialog.h"

DirView::DirView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DirView),
    m_fs(nullptr),
    m_openedFileListModel(new QStringListModel(this)),
    m_directoryModel(new QStandardItemModel(this))
{
    ui->setupUi(this);

    ui->listViewOpenedFiles->setModel(m_openedFileListModel);
    ui->treeViewBrowsingFiles->setModel(m_directoryModel);
    // 禁用双击编辑
    ui->listViewOpenedFiles->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeViewBrowsingFiles->setEditTriggers(QAbstractItemView::NoEditTriggers);

    initActionsAndContextMenu();
    reset();
}

DirView::~DirView()
{
    delete ui;
}

void DirView::setFileSystem(FileSystem *fs)
{
    m_fs = fs;
}

void DirView::updateOpenedFileList()
{
    QStringList list;
    if (m_fs != nullptr)
    {
        for(auto const& str : m_fs->getOpenedFiles())
        {
            list.append(QString::fromStdString(str));
        }
    }
    m_openedFileListModel->setStringList(list);
}

void DirView::cd(const std::string &dir)
{
    if (m_fs == nullptr)
        return;
    auto entry = m_fs->getEntry(dir);
    if (!entry->isDir())
        return;
    m_pwd = dir;
    ui->label_dirPath->setText(QString::fromStdString(m_pwd));
    updateDirectoryView();
}

void DirView::updateDirectoryView()
{
    m_directoryModel->clear();
    m_directoryModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Read Only" << "System");
    ui->treeViewBrowsingFiles->setColumnWidth(0, 250);
    if (m_fs == nullptr)
        return;
    auto children = m_fs->getEntry(m_pwd)->getChildren();
    QStandardItem* rootItem = m_directoryModel->invisibleRootItem();
    for(const auto& child : children)
    {
        QString name = QString::fromStdString(child->name());
        QString size = QString::number(child->size());
        QString readOnly = child->isReadOnly() ? "Yes" : "No";
        QString system = child->isSystem() ? "Yes" : "No";
        QList<QStandardItem *> rowItems;
        rowItems << new QStandardItem(name);
        rowItems << new QStandardItem(size);
        rowItems << new QStandardItem(readOnly);
        rowItems << new QStandardItem(system);
        rootItem->appendRow(rowItems);
        QIcon icon;
        if (child->isDir())
        {
            icon.addPixmap(style()->standardPixmap(QStyle::SP_DirIcon));
        }
        else
        {
            icon.addPixmap(style()->standardPixmap(QStyle::SP_FileIcon));
        }
        rowItems.first()->setIcon(icon);
    }
}

void DirView::reset()
{
    m_pwd = "/";
    ui->label_dirPath->setText("Path");
    updateDirectoryView();
    updateOpenedFileList();
}

void DirView::initActionsAndContextMenu()
{
    m_fileEntryMenu->addAction(actionOpenFile);
    m_fileEntryMenu->addSeparator();
    m_fileEntryMenu->addAction(actionFileProperties);
    m_fileEntryMenu->addSeparator();
    m_fileEntryMenu->addAction(actionDeleteEntry);

    m_dirEntryMenu->addAction(actionDeleteEntry);

    m_inDirMenu->addAction(actionAddNewFile);
    m_inDirMenu->addAction(actionAddNewDir);

    m_openedFileMenu->addAction(actionReadOrWrite);
    m_openedFileMenu->addSeparator();
    m_openedFileMenu->addAction(actionClose);

    connect(actionOpenFile, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        QModelIndex index = ui->treeViewBrowsingFiles->currentIndex();
        // 保证永远获取的是最左边的文件名
        QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
        std::string name = mostLeftIndex.data().toString().toStdString();
        auto targetEntry = m_fs->getEntry(m_pwd)->findChild(name);
        std::string targetPath = targetEntry->fullpath();
        if (targetEntry->isDir())
        {
            cd(targetPath);
        }
        else
        {
            if (!m_fs->openFile(targetPath, FileSystem::Read | FileSystem::Write))
            {
                if (!m_fs->openFile(targetPath, FileSystem::Read))
                {
                    emit message("Failed to open file: " +targetPath);
                    return;
                }
            }
            emit message("Opened file: " + targetPath);
            updateOpenedFileList(); // 更新界面
        }
    });

    connect(actionFileProperties, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        QModelIndex index = ui->treeViewBrowsingFiles->currentIndex();
        // 保证永远获取的是最左边的文件名
        QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
        std::string name = mostLeftIndex.data().toString().toStdString();
        std::string filePath = m_fs->getEntry(m_pwd)->findChild(name)->fullpath();
        FilePropertiesDialog* dialog = new FilePropertiesDialog(m_fs, filePath);
        dialog->show();
        connect(dialog, &QDialog::finished, this, &DirView::updateDirectoryView); // 读写操作后更新界面
    });

    connect(actionDeleteEntry, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        QModelIndex index = ui->treeViewBrowsingFiles->currentIndex();
        // 保证永远获取的是最左边的文件名
        QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
        std::string name = mostLeftIndex.data().toString().toStdString();
        auto targetEntry = m_fs->getEntry(m_pwd)->findChild(name);
        std::string targetPath = targetEntry->fullpath();
        if (m_fs->deleteEntry(targetEntry))
        {
            emit message("Deleted " + targetPath);
            updateDirectoryView(); // 更新界面
        }
        else
        {
            emit message("Failed to delete " + targetPath);
        }
    });

    connect(actionAddNewFile, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        QString fileName = QInputDialog::getText(this, "Input File Name", "File Name:");
        if (m_fs->createFile(m_fs->getEntry(m_pwd), fileName.toStdString(), FileSystem::File))
        {
            emit message("Created file " + fileName.toStdString());
            updateDirectoryView(); // 更新界面
            updateOpenedFileList();
        }
        else
        {
            emit message("Failed to create file " + fileName.toStdString());
        }
    });

    connect(actionAddNewDir, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        QString dirName = QInputDialog::getText(this, "Input Directory Name", "Directory Name:");
        if (m_fs->createDir(m_fs->getEntry(m_pwd), dirName.toStdString()))
        {
            emit message("Created directory " + dirName.toStdString());
            updateDirectoryView(); // 更新界面
        }
        else
        {
            emit message("Failed to create directory " + dirName.toStdString());
        }
    });

    connect(actionReadOrWrite, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        std::string filePath = ui->listViewOpenedFiles->currentIndex().data().toString().toStdString();
        ReadAndWriteDialog* dialog = new ReadAndWriteDialog(m_fs, filePath);
        dialog->show();
        connect(dialog, &QDialog::finished, this, &DirView::updateDirectoryView); // 读写操作后更新界面
    });

    connect(actionClose, &QAction::triggered, this, [&](){
        if (m_fs == nullptr) return;
        std::string filePath = ui->listViewOpenedFiles->currentIndex().data().toString().toStdString();
        m_fs->closeFile(filePath);
        updateOpenedFileList();
        emit message("Closed file: " + filePath);
    });
}

void DirView::on_listViewOpenedFiles_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    actionReadOrWrite->trigger();
}

void DirView::on_treeViewBrowsingFiles_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    actionOpenFile->trigger();
}

void DirView::on_toolButton_up_clicked()
{
    if (m_fs == nullptr)
        return;
    cd(m_fs->getEntry(m_pwd)->parent()->fullpath());
}

void DirView::on_treeViewBrowsingFiles_customContextMenuRequested(const QPoint &pos)
{
    if (m_fs == nullptr) return;
    QModelIndex index = ui->treeViewBrowsingFiles->indexAt(pos);
    if (!index.isValid()) // 没有选中文件或目录
    {
        m_inDirMenu->exec(ui->treeViewBrowsingFiles->viewport()->mapToGlobal(pos));
    }
    else
    {
        QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
        std::string name = m_directoryModel->data(mostLeftIndex).toString().toStdString();
        auto targetEntry = m_fs->getEntry(m_pwd)->findChild(name);
        std::string targetPath = targetEntry->fullpath();
        if (targetEntry->isDir())
        {
            m_dirEntryMenu->exec(ui->treeViewBrowsingFiles->viewport()->mapToGlobal(pos));
        }
        else
        {
            m_fileEntryMenu->exec(ui->treeViewBrowsingFiles->viewport()->mapToGlobal(pos));
        }
    }
}

void DirView::on_listViewOpenedFiles_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->listViewOpenedFiles->indexAt(pos);
    if (!index.isValid()) // 没有选中文件或目录
    {
        return;
    }
    m_openedFileMenu->exec(ui->listViewOpenedFiles->viewport()->mapToGlobal(pos));
}
