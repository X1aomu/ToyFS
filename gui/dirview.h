#ifndef TOYFS_GUI_DIRVIEW_H_
#define TOYFS_GUI_DIRVIEW_H_

#include "filesystem.h"

#include <QAction>
#include <QMenu>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QWidget>

namespace Ui
{
class DirView;
}

class DirView : public QWidget
{
    Q_OBJECT

public:
    explicit DirView(QWidget* parent = nullptr);
    ~DirView();

    void setFileSystem(FileSystem* fs);
    void cd(const std::string& dir);
    void updateOpenedFileList();
    void updateDirectoryView();
    void reset();

signals:
    void message(const std::string& msg);

private:
    void initActionsAndContextMenu();

private slots:
    void on_listViewOpenedFiles_doubleClicked(const QModelIndex& index);
    void on_treeViewBrowsingFiles_doubleClicked(const QModelIndex& index);
    void on_toolButton_up_clicked();
    void on_treeViewBrowsingFiles_customContextMenuRequested(const QPoint& pos);
    void on_listViewOpenedFiles_customContextMenuRequested(const QPoint& pos);

private:
    Ui::DirView* ui;

    FileSystem* m_fs;
    std::string m_pwd;
    QStringListModel* m_openedFileListModel;
    QStandardItemModel* m_directoryModel;

    QMenu* m_fileEntryMenu = new QMenu(this);  // 文件项的菜单
    QMenu* m_dirEntryMenu = new QMenu(this);   // 目录项的菜单
    QMenu* m_inDirMenu = new QMenu(this);      // 在目录空白处点击出现的菜单
    QMenu* m_openedFileMenu = new QMenu(this); // 打开文件列表项的菜单

    QAction* actionOpenFile = new QAction("Open", this);
    QAction* actionFileProperties = new QAction("Properties", this);
    QAction* actionDeleteEntry = new QAction("Delete", this);
    QAction* actionAddNewFile = new QAction("Add New File", this);
    QAction* actionAddNewDir = new QAction("Add New Directory", this);
    QAction* actionReadOrWrite = new QAction("Read or Write", this);
    QAction* actionClose = new QAction("Close", this);
};

#endif // TOYFS_GUI_DIRVIEW_H_
