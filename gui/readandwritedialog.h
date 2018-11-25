#ifndef READANDWRITEDIALOG_H
#define READANDWRITEDIALOG_H

#include "filesystem.h"

#include <QDialog>

namespace Ui {
class ReadAndWriteDialog;
}

class ReadAndWriteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReadAndWriteDialog(FileSystem* fs, const std::string& filePath, QWidget *parent = nullptr);
    ~ReadAndWriteDialog();

private slots:
    void on_radioButton_read_clicked();

    void on_radioButton_write_clicked();

    void on_pushButton_clicked();

private:
    Ui::ReadAndWriteDialog *ui;

    FileSystem* m_fs;
    std::string m_filePath;
};

#endif // READANDWRITEDIALOG_H
