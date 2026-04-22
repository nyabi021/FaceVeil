#include "faceveil/MainWindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    faceveil::MainWindow window;
    window.show();
    return QApplication::exec();
}
