#include "gui/mainwindow.h"
#include <QApplication>
#include <QStyle>

using namespace std;
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QIcon appIcon;
    appIcon.addPixmap(app.style()->standardPixmap(QStyle::SP_DirOpenIcon));
    app.setWindowIcon(appIcon);

    MainWindow w;
    w.show();

    return app.exec();
}
