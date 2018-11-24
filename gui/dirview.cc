#include "dirview.h"
#include "ui_dirview.h"

#include <QtDebug>
#include <QStringListModel>
#include <QTimer>

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
//    QTimer *updateOpenedFileListTimer = new QTimer(this);
//    connect(updateOpenedFileListTimer, &QTimer::timeout, this, &DirView::updateOpenedFileList);
//    updateOpenedFileListTimer->start(500);
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
    m_fileEntryMenu->addAction(actionDeleteEntry);
    m_dirEntryMenu->addAction(actionDeleteEntry);
    m_inDirMenu->addAction(actionAddNewFile);
    m_inDirMenu->addAction(actionAddNewDir);
    m_openedFileMenu->addAction(actionRead);
    m_openedFileMenu->addAction(actionWrite);

    connect(actionDeleteEntry, &QAction::triggered, this, [&](){
        std::string name = ui->treeViewBrowsingFiles->currentIndex().data().toString().toStdString();
        auto targetEntry = m_fs->getEntry(m_pwd)->findChild(name);
        std::string targetPath = targetEntry->fullpath();
        if (m_fs->deleteEntry(targetEntry))
        {
            emit message("Deleted " + targetPath);
            updateDirectoryView();
        }
        else
        {
            emit message("Failed to delete " + targetPath);
        }
    });
}

void DirView::on_listViewOpenedFiles_doubleClicked(const QModelIndex &index)
{
    std::string file = m_openedFileListModel->data(index).toString().toStdString();
    m_fs->closeFile(file);
    updateOpenedFileList();
    emit message("Closed file: " + file);
}

void DirView::on_treeViewBrowsingFiles_doubleClicked(const QModelIndex &index)
{
    // 保证永远获取的是最左边的文件名
    QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
    std::string name = m_directoryModel->data(mostLeftIndex).toString().toStdString();
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
        updateOpenedFileList();
    }
}

void DirView::on_toolButton_up_clicked()
{
    if (m_fs == nullptr)
        return;
    cd(m_fs->getEntry(m_pwd)->parent()->fullpath());
}

void DirView::on_treeViewBrowsingFiles_customContextMenuRequested(const QPoint &pos)
{
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
