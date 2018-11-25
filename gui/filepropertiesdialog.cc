#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"

FilePropertiesDialog::FilePropertiesDialog(FileSystem *fs, const std::string &filePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilePropertiesDialog),
    m_fs(fs),
    m_filePath(filePath)
{
    ui->setupUi(this);
    setWindowTitle("File Properties");
    setModal(true);

    auto fileEntry = m_fs->getEntry(filePath);

    ui->label_path->setText(QString::fromStdString(fileEntry->fullpath()));

    FileSystem::Attributes attrs = m_fs->getEntry(filePath)->attributes();
    if (attrs & FileSystem::ReadOnly)
    {
        ui->checkBox_readOnly->setChecked(true);
    }
    if (attrs & FileSystem::System)
    {
        ui->checkBox_system->setChecked(true);
    }

    if (m_fs->isOpened(filePath)) // 如果文件已经打开，设置属性为只读
    {
        ui->checkBox_readOnly->setEnabled(false);
        ui->checkBox_system->setEnabled(false);
        ui->pushButton_ok->setEnabled(false);
    }
}

FilePropertiesDialog::~FilePropertiesDialog()
{
    delete ui;
}

void FilePropertiesDialog::on_pushButton_ok_clicked()
{
    FileSystem::Attributes newAttrs = FileSystem::File;
    if (ui->checkBox_readOnly->isChecked())
    {
        newAttrs |= FileSystem::ReadOnly;
    }
    if (ui->checkBox_system->isChecked())
    {
        newAttrs |= FileSystem::System;
    }
    if (newAttrs == m_oldAttrs) return; // 没变，什么也不用干了
    m_fs->setFileAttributes(m_filePath, newAttrs);
    close();
}

void FilePropertiesDialog::on_pushButton_cancel_clicked()
{
    close();
}
