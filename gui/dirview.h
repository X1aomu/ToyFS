#ifndef DIRVIEW_H
#define DIRVIEW_H

#include <QWidget>

#include "filesystem.h"

#include <QStringListModel>
#include <QStandardItemModel>

namespace Ui {
class DirView;
}

class DirView : public QWidget
{
    Q_OBJECT

public:
    explicit DirView(QWidget *parent = nullptr);
    ~DirView();

    void setFileSystem(FileSystem *fs);
    void cd(const std::string& dir);
    void updateOpenedFileList();
    void updateDirectoryView();
    void reset();

signals:
    void message(const QString& msg);

private slots:
    void on_listViewOpenedFiles_doubleClicked(const QModelIndex &index);

    void on_treeViewBrowsingFiles_doubleClicked(const QModelIndex &index);

    void on_toolButton_up_clicked();

private:
    Ui::DirView *ui;

    FileSystem *m_fs;
    std::string m_pwd;
    QStringListModel* m_openedFileListModel;
    QStandardItemModel* m_directoryModel;
};

#endif // DIRVIEW_H
