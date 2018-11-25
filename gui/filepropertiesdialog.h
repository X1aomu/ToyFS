#ifndef TOYFS_GUI_FILEPROPERTIESDIALOG_H_
#define TOYFS_GUI_FILEPROPERTIESDIALOG_H_

#include "filesystem.h"

#include <QDialog>

namespace Ui
{
class FilePropertiesDialog;
}

class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(FileSystem* fs, const std::string& filePath, QWidget* parent = nullptr);
    ~FilePropertiesDialog();

private slots:
    void on_pushButton_ok_clicked();
    void on_pushButton_cancel_clicked();

private:
    Ui::FilePropertiesDialog* ui;

    FileSystem* m_fs;
    std::string m_filePath;
    FileSystem::Attributes m_oldAttrs;
};

#endif // TOYFS_GUI_FILEPROPERTIESDIALOG_H_
