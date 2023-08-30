#include "qfmainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFMainWindow w;
    w.show();

    return a.exec();
}
