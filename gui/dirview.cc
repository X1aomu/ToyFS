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

void DirView::on_listViewOpenedFiles_doubleClicked(const QModelIndex &index)
{
    std::string file = m_openedFileListModel->data(index).toString().toStdString();
    m_fs->closeFile(file);
    updateOpenedFileList();
    emit message("Closed file: " + QString::fromStdString(file));
}

void DirView::on_treeViewBrowsingFiles_doubleClicked(const QModelIndex &index)
{
    // 保证永远获取的是最左边的文件名
    QModelIndex mostLeftIndex = m_directoryModel->index(index.row(), 0);
    std::string name = m_directoryModel->data(mostLeftIndex).toString().toStdString();
    std::string target;
    if (m_pwd != "/")
    {
        target = m_pwd + "/" + name;
    }
    else
    {
        target = m_pwd + name;
    }
    if (m_fs->getEntry(target)->isDir())
    {
        cd(target);
    }
    else
    {
        if (!m_fs->openFile(target, FileSystem::Read | FileSystem::Write))
        {
            if (!m_fs->openFile(target, FileSystem::Read))
            {
                emit message("Failed to open file: " + QString::fromStdString(target));
                return;
            }
        }
        emit message("Opened file: " + QString::fromStdString(target));
        updateOpenedFileList();
    }
}

void DirView::on_toolButton_up_clicked()
{
    if (m_fs == nullptr)
        return;
    cd(m_fs->getEntry(m_pwd)->parent()->fullpath());
}
