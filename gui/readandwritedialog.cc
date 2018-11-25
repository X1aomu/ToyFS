#include "readandwritedialog.h"
#include "ui_readandwritedialog.h"

ReadAndWriteDialog::ReadAndWriteDialog(FileSystem *fs, const std::string &filePath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReadAndWriteDialog),
    m_fs(fs),
    m_filePath(filePath)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle(QString::fromStdString(filePath));
}

ReadAndWriteDialog::~ReadAndWriteDialog()
{
    delete ui;
}

void ReadAndWriteDialog::on_radioButton_read_clicked()
{
    ui->pushButton->setText("Read");
    ui->label_readOrWrite->setText("bytes read");
    ui->lineEdit->clear();
    ui->lineEdit->setReadOnly(false);
    ui->textEdit->setReadOnly(true);
}

void ReadAndWriteDialog::on_radioButton_write_clicked()
{
    ui->pushButton->setText("Write");
    ui->label_readOrWrite->setText("bytes written");
    ui->lineEdit->setText("Input text to write below");
    ui->lineEdit->setReadOnly(true);
    ui->textEdit->setReadOnly(false);
}

void ReadAndWriteDialog::on_pushButton_clicked()
{
    if (ui->radioButton_read->isChecked())
    {
        int numOfBytesToRead = ui->lineEdit->text().toInt();
        char* buffer = new char[numOfBytesToRead];
        int bytesRead = m_fs->readFile(m_filePath, buffer, numOfBytesToRead);
        ui->textEdit->setText(QString::fromStdString(std::string(buffer, buffer + bytesRead)));
        ui->label_numOfBytes->setText(QString::number(bytesRead));
        delete[] buffer;
    }
    else
    {
        int numOfBytesToWrite = ui->textEdit->toPlainText().length();
        const char* output = ui->textEdit->toPlainText().toStdString().c_str();
        bool success = m_fs->writeFile(m_filePath, output, numOfBytesToWrite);
        int bytesWritten = success ? numOfBytesToWrite : 0;
        ui->label_numOfBytes->setText(QString::number(bytesWritten));
    }
}
