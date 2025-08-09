#include <QApplication>
#include "platformer.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    Platformer w;
    w.setWindowTitle("Simple Platformer");
    w.show();

    return a.exec();
}
