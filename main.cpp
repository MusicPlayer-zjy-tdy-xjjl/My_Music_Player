#include "MainWidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/image/image/image/systemTrayIcon.png"));
    MainWidget w;
    w.show();
    return a.exec();
}
